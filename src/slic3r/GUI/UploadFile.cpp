#include "UploadFile.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <fstream>
#include <thread>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "GUI.hpp"
#include "GUI_App.hpp"
#include "../Utils/Http.hpp"
#include "../Utils/json_diff.hpp"


#include <iostream>
#include <mutex>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
namespace Slic3r { 
namespace GUI {

namespace {
std::mutex g_oss_sdk_mutex;
std::size_t g_oss_sdk_users = 0;

class TemporaryFileCleanup
{
public:
    explicit TemporaryFileCleanup(boost::filesystem::path path) : m_path(std::move(path)) {}
    ~TemporaryFileCleanup()
    {
        boost::system::error_code error;
        boost::filesystem::remove(m_path, error);
    }

private:
    boost::filesystem::path m_path;
};

class OssRequestCancelWatcher
{
public:
    OssRequestCancelWatcher(AlibabaCloud::OSS::OssClient& client,
                            RemotePrint::UploadCancelToken cancel_token)
        : m_client(client)
        , m_cancel_token(std::move(cancel_token))
        , m_thread([this] {
            while (!m_stopped.load(std::memory_order_acquire)) {
                if (RemotePrint::is_upload_cancelled(m_cancel_token)) {
                    // The vendored OSS patch also shuts down active sockets, so
                    // a stalled synchronous request does not wait for progress.
                    BOOST_LOG_TRIVIAL(warning) << "[CloudUpload] cancelling active OSS request";
                    m_client.DisableRequest();
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        })
    {
    }

    ~OssRequestCancelWatcher()
    {
        m_stopped.store(true, std::memory_order_release);
        if (m_thread.joinable())
            m_thread.join();
    }

private:
    AlibabaCloud::OSS::OssClient& m_client;
    RemotePrint::UploadCancelToken m_cancel_token;
    std::atomic_bool m_stopped {false};
    std::thread m_thread;
};
}

UploadFile::UploadFile() {
    std::lock_guard<std::mutex> lock(g_oss_sdk_mutex);
    if (g_oss_sdk_users++ == 0)
        AlibabaCloud::OSS::InitializeSdk();
}
UploadFile::~UploadFile(){
    std::lock_guard<std::mutex> lock(g_oss_sdk_mutex);
    if (g_oss_sdk_users > 0 && --g_oss_sdk_users == 0)
        AlibabaCloud::OSS::ShutdownSdk();
}
json UploadFile::getCloudUploadInfo()
{
    getAliyunInfo();
    getOssInfo();
    json j;
    j["token"] = m_token;
    j["accessKeyId"] = m_accessKeyId;
    j["secretAccessKey"] = m_secretAccessKey;
    j["endPoint"] = m_endPoint;
    j["bucket"] = m_bucket;
    j["video_bucket"]    = m_video_bucket;
    j["cdnHost"] = m_cdnHost;
    return j;
}
int UploadFile::getAliyunInfo()
{
    if (isCancelled())
        return 601;

    int nRet = -1;

    std::string base_url = get_cloud_api_url();
    auto               preupload_profile_url = "/api/cxy/account/v2/getAliyunInfo";
    std::string url                   = base_url + preupload_profile_url;

    std::map<std::string, std::string> mapHeader;
    wxGetApp().getExtraHeader(mapHeader);
    Http::set_extra_headers(mapHeader);
    Http               http                  = Http::post(url);
    http.enable_active_cancel();
    RemotePrint::UploadRequestCancelWatcher cancel_watcher(
        m_cancel_token, [&http] { http.cancel(); });
    boost::uuids::uuid uuid                  = boost::uuids::random_generator()();
    json               j = json::object();


    std::string body = j.dump();
    http.header("Content-Type", "application/json")
        .header("Connection", "keep-alive")
        .header("__CXY_REQUESTID_", to_string(uuid))
        .timeout_connect(5)
        .timeout_max(15)
        .set_post_body(body)
        .on_complete([&](std::string body, unsigned status) { 
            json jBody;
            try {
                jBody = json::parse(body);
            } catch (const json::parse_error& e) {
                BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " getOssInfo json parse error: " << e.what();
                nRet = -5;
                return;
            }
            if (jBody["code"].is_number_integer()) {
                nRet = jBody["code"];
                if (jBody["code"].get<int>() == 4) {
                    nRet = 4;
                    return;
                }
            }
            if (nRet != 0)
            {
                throw ErrorCodeException("getAliyunInfo", nRet,jBody["msg"].get<std::string>());
                return;
            }
            if (!jBody["result"].is_object()) {
                nRet = -2;
                return;
            }
            if (!jBody["result"]["aliyunInfo"].is_object()) {
                nRet = -3;
                return;
            }
            m_token           = jBody["result"]["aliyunInfo"]["sessionToken"];
            m_accessKeyId = jBody["result"]["aliyunInfo"]["accessKeyId"];
            m_secretAccessKey = jBody["result"]["aliyunInfo"]["secretAccessKey"];

            nRet = 0;
        })
        .on_error([&](std::string body, std::string error, unsigned status) { nRet = -4;
        })
        .on_progress([&](Http::Progress progress, bool& cancel) {
            cancel = isCancelled();
        })
        .perform_sync();

    if (isCancelled())
        return 601;
    return nRet;
}

int UploadFile::getOssInfo()
{
    if (isCancelled())
        return 601;

    int nRet = -1;

    std::string        base_url = get_cloud_api_url();
    auto               preupload_profile_url = "/api/cxy/v2/common/getOssInfo";
    std::string        url                   = base_url + preupload_profile_url;

    std::map<std::string, std::string> mapHeader;
    wxGetApp().getExtraHeader(mapHeader);
    Http::set_extra_headers(mapHeader);
    Http               http                  = Http::post(url);
    http.enable_active_cancel();
    RemotePrint::UploadRequestCancelWatcher cancel_watcher(
        m_cancel_token, [&http] { http.cancel(); });
    boost::uuids::uuid uuid                  = boost::uuids::random_generator()();
    json               j                     = json::object();


    std::string body = j.dump();
    http.header("Content-Type", "application/json")
        .header("Connection", "keep-alive")
        .header("__CXY_REQUESTID_", to_string(uuid))
        .timeout_connect(5)
        .timeout_max(15)
        .set_post_body(body)
        .on_complete([&](std::string body, unsigned status) {
            json jBody;
            try {
                jBody = json::parse(body);
            } catch (const json::parse_error& e) {
                BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " getOssInfo json parse error: " << e.what();
                nRet = -5;
                return;
            }
            if (jBody["code"].is_number_integer()) {
                nRet = jBody["code"];
                if (jBody["code"].get<int>() == 4) {
                    nRet = 4;
                    return;
                }
            }
            if (nRet != 0)
            {
                throw ErrorCodeException("getOssInfo", nRet,jBody["msg"].get<std::string>());
                return;
            }
               
            if (!jBody["result"].is_object()) {
                nRet = -2;
                return;
            }
            if (!jBody["result"]["info"].is_object()) {
                nRet = -3;
                return;
            }
            m_endPoint           = jBody["result"]["info"]["endpoint"];
            m_bucket             = jBody["result"]["info"]["file"]["bucket"];
            m_video_bucket       = jBody["result"]["info"]["video"]["bucket"];
            const auto& info     = jBody["result"]["info"];
            m_cdnHost.clear();
            auto videoIt = info.find("video");
            if (videoIt != info.end() && videoIt->is_object()) {
                auto cdnHostIt = videoIt->find("cdnHost");
                if (cdnHostIt != videoIt->end() && cdnHostIt->is_string()) {
                    m_cdnHost = cdnHostIt->get<std::string>();
                }
            }
            nRet = 0;
        })
        .on_error([&](std::string body, std::string error, unsigned status) { nRet = -4;
        })
        .on_progress([&](Http::Progress progress, bool& cancel) {
            cancel = isCancelled();
        })
        .perform_sync();

    if (isCancelled())
        return 601;
    return nRet;
}

int UploadFile::uploadGcodeToCXCloud(const std::string& name, const std::string& fileName, std::function<void(std::string)> onCompleteCallback) {
    if (isCancelled())
        return 601;

    int nRet = -1;

    std::string        base_url = get_cloud_api_url();
    auto               preupload_profile_url = "/api/cxy/v2/gcode/uploadGcode";
    std::string        url                   = base_url + preupload_profile_url;

    std::map<std::string, std::string> mapHeader;
    wxGetApp().getExtraHeader(mapHeader);
    Http::set_extra_headers(mapHeader);
    Http               http                  = Http::post(url);
    http.enable_active_cancel();
    RemotePrint::UploadRequestCancelWatcher cancel_watcher(
        m_cancel_token, [&http] { http.cancel(); });
    boost::uuids::uuid uuid                  = boost::uuids::random_generator()();
    json               j                     = json::object();
    j["list"]                                = json::array();
    j["list"][0]                             = json::object();
    j["list"][0]["name"]                     = name;
    j["list"][0]["fileKey"]                  = fileName;
    j["list"][0]["type"]                  = 1;

    std::string body = j.dump();
    http.header("Content-Type", "application/json")
        .header("Connection", "keep-alive")
        .header("__CXY_REQUESTID_", to_string(uuid))
        .timeout_connect(5)
        .timeout_max(15)
        .set_post_body(body)
        .on_complete([&](std::string body, unsigned status) {
            if (isCancelled()) {
                nRet = 601;
                return;
            }
            json jBody = json::parse(body);
             if (jBody["code"].is_number_integer()) {
                nRet = jBody["code"];
                if(nRet !=0)
                {
                    if (jBody["code"].get<int>() == 4) {
                        nRet = 4;
                        return;
                    }
                   BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << "uploadGcodeToCXCloud body: " << body;
                   std::string errMsg = jBody["msg"].get<std::string>();
                   throw ErrorCodeException("uploadGcodeToCXCloud", nRet,errMsg);
                }
            }

            if(onCompleteCallback)onCompleteCallback(body);
             if (nRet != 0)
                return;
            //if (!jBody["result"].is_object()) {
            //    nRet = -2;
            //    return;
           //}
            //if (!jBody["result"]["info"].is_object()) {
            //    nRet = -3;
            //    return;
            //}

            nRet = 0;
        })
        .on_error([&](std::string body, std::string error, unsigned status) { nRet = -4;
        })
        .on_progress([&](Http::Progress progress, bool& cancel) {
            cancel = isCancelled();
        })
        .perform_sync();

    if (isCancelled())
        return 601;
    return nRet;
}

void UploadFile::setProcessCallback(std::function<void(int,double)> funcProcessCb)
{
    m_funcProcessCb = funcProcessCb;
}
void UploadFile::UploadProgressCallback(int partNumber, int totalParts, double percentage) {
    int i = (int)(percentage * 100);
    if (m_funcProcessCb != nullptr) {
        m_funcProcessCb(i,0);
    }
}

void UploadFile::ProgressCallback(size_t increment, int64_t transfered, int64_t total, void* userData) {
    int i = (int)((transfered * 1.0 / total * 1.0) * 100);
    if (m_funcProcessCb != nullptr) {
        m_funcProcessCb(i,0);
    }
}

const int64_t PART_SIZE = 1024 * 1024; // 1MB per part
string initiateUploadWithMeta(AlibabaCloud::OSS::OssClient& client, 
                            const string& bucketName, 
                            const string& objectName,
                            const AlibabaCloud::OSS::ObjectMetaData& metaData) {
    AlibabaCloud::OSS::InitiateMultipartUploadRequest request(bucketName, objectName,metaData);
    

    
    auto outcome = client.InitiateMultipartUpload(request);
    
    if (!outcome.isSuccess()) {
        throw runtime_error("InitiateMultipartUpload fail: " + 
                          outcome.error().Code() + ", " + 
                          outcome.error().Message());
    }
    
    return outcome.result().UploadId();
}

vector<AlibabaCloud::OSS::Part> UploadFile::uploadParts(AlibabaCloud::OSS::OssClient& client,
                       const string& bucketName,
                       const string& objectName,
                       const string& uploadId,
                       const string& filePath,
                       Slic3r::GUI::ProgressCallback callback) {
    vector<AlibabaCloud::OSS::Part> partList;
    boost::nowide::ifstream file(filePath, ios::binary | ios::ate);
    
    if (!file.is_open()) {
        throw ErrorCodeException("uploadParts",1001,"Failed to open file: " + filePath);
    }
    
    int64_t fileSize = file.tellg();
    file.seekg(0, ios::beg);
    int partCount = static_cast<int>((fileSize + PART_SIZE - 1) / PART_SIZE);
    
    for (int i = 1; i <= partCount; i++) {
        if (isCancelled()) {
            file.close();
            throw ErrorCodeException("UploadPart", 601, "Upload canceled");
        }

        int64_t partSize = min(PART_SIZE, fileSize - (i - 1) * PART_SIZE);
    
        std::vector<char> buffer(static_cast<size_t>(partSize));
        file.read(buffer.data(), partSize);
        size_t bytesRead = file.gcount();
        shared_ptr<std::iostream> streambuffer =
            make_shared<std::stringstream>(std::string(buffer.data(), bytesRead));
        
        AlibabaCloud::OSS::UploadPartRequest request(bucketName, objectName,i, uploadId, streambuffer);
        
        auto outcome = client.UploadPart(request);

        if (isCancelled()) {
            file.close();
            throw ErrorCodeException("UploadPart", 601, "Upload canceled");
        }
        
        if (!outcome.isSuccess()) {
            file.close();
            throw ErrorCodeException("UploadPart",1002,"UploadPart fail: " + 
                              outcome.error().Code() + ", " + 
                              outcome.error().Message());
        }
        
        partList.emplace_back(i, outcome.result().ETag());
        
        if (callback) {
            double percentage = 70.0 * i / partCount/100;
            callback(i, partCount, 0.3+percentage);
        }
    }
    
    file.close();
    return partList;
}

void completeMultipartUpload(AlibabaCloud::OSS::OssClient& client,
                           const string& bucketName,
                           const string& objectName,
                           const string& uploadId,
                           const vector<AlibabaCloud::OSS::Part>& partList) {
    AlibabaCloud::OSS::CompleteMultipartUploadRequest request(bucketName, objectName, partList,uploadId);
    auto outcome = client.CompleteMultipartUpload(request);
    
    if (!outcome.isSuccess()) {
        throw ErrorCodeException("CompleteMultipartUpload",1003,"CompleteMultipartUpload fail: " + 
                          outcome.error().Code() + ", " + 
                          outcome.error().Message());
    }
    
    cout << "\nMultipart upload completed. ETag: " 
         << outcome.result().ETag() << endl;
}

int UploadFile::uploadFileToAliyun(const std::string& local_path, const std::string& target_path, const std::string& fileName) {
    if (isCancelled())
        return 601;

    RemotePrint::UploadFileUseGuard file_use(m_cancel_token);
    if (!file_use)
        return 601;

    int nRet = 0;
    boost::nowide::ifstream file_in(local_path, std::ios_base::binary);
        if (!file_in)
        {
            std::cerr << "无法打开输入文件。" << std::endl;
            return 1;
        }
        auto call_back = std::bind(&UploadFile::UploadProgressCallback, this,
        std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3);
        call_back(0, 0, 0.01);
        // 创建输出文件，文件名后缀为.gz
        boost::filesystem::path temp_path = boost::filesystem::temp_directory_path();
        boost::filesystem::path outputfile(
            "creality-upload-" + boost::uuids::to_string(boost::uuids::random_generator()()) + ".gz");
        boost::filesystem::path joined_path = temp_path / outputfile;
        TemporaryFileCleanup temp_file_cleanup(joined_path);
        boost::nowide::ofstream file_out(joined_path.string(), boost::nowide::ofstream::binary);
        if (!file_out)
        {
            std::cerr << "无法打开输出文件。" << std::endl;
            return 1;
        }
        // Compress in bounded chunks so a close/cancel request can release the
        // source GCode promptly, even for very large files.
        bool compression_cancelled = false;
        {
            boost::iostreams::filtering_ostream gzip_out;
            gzip_out.push(boost::iostreams::gzip_compressor());
            gzip_out.push(file_out);

            std::array<char, 256 * 1024> buffer;
            while (file_in) {
                if (isCancelled()) {
                    compression_cancelled = true;
                    break;
                }

                file_in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
                const std::streamsize read_count = file_in.gcount();
                if (read_count > 0)
                    gzip_out.write(buffer.data(), read_count);
            }

            if (!compression_cancelled)
                boost::iostreams::close(gzip_out);
        }
        file_in.close();
        file_out.close();
        if (compression_cancelled || isCancelled())
        {
            return 601;
        }
        std::string upload_path = target_path;// + ".gz";
    
    //AlibabaCloud::OSS::TransferProgress progress_callback = { call_back, nullptr };

    //std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(joined_path.string().c_str(), std::ios::in | std::ios::binary);
    //AlibabaCloud::OSS::PutObjectRequest request(m_bucket, upload_path, content);
    string uploadId;
    AlibabaCloud::OSS::ClientConfiguration conf;
    conf.connectTimeoutMs = 10 * 1000;
    conf.requestTimeoutMs = 20 * 1000;
    AlibabaCloud::OSS::OssClient oss_client(m_endPoint, m_accessKeyId, m_secretAccessKey, m_token, conf);
    OssRequestCancelWatcher cancel_watcher(oss_client, m_cancel_token);
    try{
    AlibabaCloud::OSS::ObjectMetaData metaData;
    metaData.addHeader("Content-Disposition", "attachment;filename=\"" + wxGetApp().url_encode(fileName+".gcode.gz") + "\"");
    metaData.addHeader("Content-Type", "application/x-www-form-urlencoded");
    metaData.addHeader("disabledMD5", "false");
    //request.setTransferProgress(progress_callback);


    call_back(0, 0, 0.3);
    if (isCancelled())
        return 601;

    uploadId = initiateUploadWithMeta(oss_client, m_bucket, upload_path, metaData);
    if (isCancelled())
        throw ErrorCodeException("InitiateMultipartUpload", 601, "Upload canceled");

    auto partList = uploadParts(oss_client, m_bucket, upload_path, uploadId, joined_path.string(),call_back);
        
    if (isCancelled())
        throw ErrorCodeException("CompleteMultipartUpload", 601, "Upload canceled");

    cout << "Completing upload..." << endl;
    completeMultipartUpload(oss_client, m_bucket, upload_path, uploadId, partList);
    }catch(const ErrorCodeException& e){
        if (!uploadId.empty() && !isCancelled()) {
            AlibabaCloud::OSS::AbortMultipartUploadRequest request(m_bucket, upload_path, uploadId);
            oss_client.AbortMultipartUpload(request);
        }
        nRet = isCancelled() ? 601 : e.code();
        m_lastError.code = std::to_string(nRet);
        m_lastError.message = e.msg();
    }catch(const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        if (!uploadId.empty() && !isCancelled()) {
            AlibabaCloud::OSS::AbortMultipartUploadRequest request(m_bucket, upload_path, uploadId);
            oss_client.AbortMultipartUpload(request);
        }
        nRet = isCancelled() ? 601 : 1000;
    }
    //auto outcome = oss_client.PutObject(request);
    if (isCancelled())
    {
        return 601;
    }
    

    return nRet; 
}

int UploadFile::downloadFileFromAliyun(const std::string& target_path, const std::string& local_path) {
    int nRet = 0;

    auto call_back = std::bind(&UploadFile::ProgressCallback, this,
        std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4);
    AlibabaCloud::OSS::TransferProgress progress_callback = { call_back, nullptr };

    AlibabaCloud::OSS::DownloadObjectRequest request(m_bucket, target_path, local_path);
    request.setTransferProgress(progress_callback);
    request.setPartSize(100 * 1024);
    request.setThreadNum(4);
    //request.setCheckpointDir("E:/999");

    AlibabaCloud::OSS::ClientConfiguration conf;
    AlibabaCloud::OSS::OssClient oss_client(m_endPoint, m_accessKeyId, m_secretAccessKey, m_token, conf);
    auto outcome = oss_client.ResumableDownloadObject(request);
    if (!outcome.isSuccess()) {
        m_lastError.code = outcome.error().Code();
        m_lastError.message = outcome.error().Message();
        m_lastError.requestId = outcome.error().RequestId();
        nRet = 1;
        return nRet;
    }

    return nRet;
}


}
} // namespace Slic3r::GUI
