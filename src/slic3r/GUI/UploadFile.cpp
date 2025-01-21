#include "UploadFile.hpp"

#include <fstream>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "GUI.hpp"
#include "GUI_App.hpp"
#include "../Utils/Http.hpp"
#include "../Utils/json_diff.hpp"

#include "alibabacloud/oss/OssClient.h"
#include <iostream>
#include <fstream>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
namespace Slic3r { 
namespace GUI {
UploadFile::UploadFile() {
    AlibabaCloud::OSS::InitializeSdk();
}
UploadFile::~UploadFile(){
    AlibabaCloud::OSS::ShutdownSdk();
}
int UploadFile::getAliyunInfo()
{
    int nRet = -1;

    std::string base_url = get_cloud_api_url();
    auto               preupload_profile_url = "/api/cxy/account/v2/getAliyunInfo";
    std::string url                   = base_url + preupload_profile_url;

    std::map<std::string, std::string> mapHeader;
    wxGetApp().getExtraHeader(mapHeader);
    Http::set_extra_headers(mapHeader);
    Http               http                  = Http::post(url);
    boost::uuids::uuid uuid                  = boost::uuids::random_generator()();
    json               j = json::object();


    std::string body = j.dump();
    http.header("Content-Type", "application/json")
        .header("Connection", "keep-alive")
        .header("__CXY_REQUESTID_", to_string(uuid))
        .set_post_body(body)
        .on_complete([&](std::string body, unsigned status) { 
            json jBody = json::parse(body);
            if (jBody["code"].is_number_integer()) {
                nRet = jBody["code"];
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

        })
        .perform_sync();

    return nRet;
}

int UploadFile::getOssInfo()
{
    int nRet = -1;

    std::string        base_url = get_cloud_api_url();
    auto               preupload_profile_url = "/api/cxy/v2/common/getOssInfo";
    std::string        url                   = base_url + preupload_profile_url;

    std::map<std::string, std::string> mapHeader;
    wxGetApp().getExtraHeader(mapHeader);
    Http::set_extra_headers(mapHeader);
    Http               http                  = Http::post(url);
    boost::uuids::uuid uuid                  = boost::uuids::random_generator()();
    json               j                     = json::object();


    std::string body = j.dump();
    http.header("Content-Type", "application/json")
        .header("Connection", "keep-alive")
        .header("__CXY_REQUESTID_", to_string(uuid))
        .set_post_body(body)
        .on_complete([&](std::string body, unsigned status) {
            json jBody = json::parse(body);
            if (jBody["code"].is_number_integer()) {
                nRet = jBody["code"];
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
            m_bucket     = jBody["result"]["info"]["file"]["bucket"];

            nRet = 0;
        })
        .on_error([&](std::string body, std::string error, unsigned status) { nRet = -4;
        })
        .on_progress([&](Http::Progress progress, bool& cancel) {

        })
        .perform_sync();

    return nRet;
}

int UploadFile::uploadGcodeToCXCloud(const std::string& name, const std::string& fileName, std::function<void(std::string)> onCompleteCallback) {
    int nRet = -1;

    std::string        base_url = get_cloud_api_url();
    auto               preupload_profile_url = "/api/cxy/v2/gcode/uploadGcode";
    std::string        url                   = base_url + preupload_profile_url;

    std::map<std::string, std::string> mapHeader;
    wxGetApp().getExtraHeader(mapHeader);
    Http::set_extra_headers(mapHeader);
    Http               http                  = Http::post(url);
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
        .set_post_body(body)
        .on_complete([&](std::string body, unsigned status) {
            json jBody = json::parse(body);
             if (jBody["code"].is_number_integer()) {
                nRet = jBody["code"];
                if(nRet !=0)
                {
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

        })
        .perform_sync();

    return nRet;
}

void UploadFile::setProcessCallback(std::function<void(int)> funcProcessCb)
{
    m_funcProcessCb = funcProcessCb;
}
void UploadFile::ProgressCallback(size_t increment, int64_t transfered, int64_t total, void* userData) {
    int i = (int)((transfered * 1.0 / total * 1.0) * 100);
    if (m_funcProcessCb != nullptr) {
        m_funcProcessCb(i);
    }
}

int UploadFile::uploadFileToAliyun(const std::string& local_path, const std::string& target_path, const std::string& fileName) {
    int nRet = 0;
    boost::nowide::ifstream file_in(local_path, std::ios_base::binary);
        if (!file_in)
        {
            std::cerr << "无法打开输入文件。" << std::endl;
            return 1;
        }
        // 创建输出文件，文件名后缀为.gz
        boost::filesystem::path temp_path = boost::filesystem::temp_directory_path();
        boost::filesystem::path outputfile("output.gz");
        boost::filesystem::path joined_path = temp_path / outputfile;
        std::ofstream file_out(joined_path.string(), std::ios_base::binary);
        if (!file_out)
        {
            std::cerr << "无法打开输出文件。" << std::endl;
            return 1;
        }
        // 创建过滤流缓冲区，用于压缩
        boost::iostreams::filtering_streambuf<boost::iostreams::input> out;
        out.push(boost::iostreams::gzip_compressor());
        out.push(file_in);
        // 将压缩后的数据写入输出文件
        boost::iostreams::copy(out, file_out);
        file_out.close();
        std::string upload_path = target_path;// + ".gz";
    auto call_back = std::bind(&UploadFile::ProgressCallback, this,
        std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4);
    AlibabaCloud::OSS::TransferProgress progress_callback = { call_back, nullptr };

    std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(joined_path.string().c_str(), std::ios::in | std::ios::binary);
    AlibabaCloud::OSS::PutObjectRequest request(m_bucket, upload_path, content);
    request.MetaData().addHeader("Content-Disposition", "attachment;filename=\"" + wxGetApp().url_encode(fileName+".gcode.gz") + "\"");
    request.MetaData().addHeader("Content-Type", "application/x-www-form-urlencoded");
    request.MetaData().addHeader("disabledMD5", "false");
    request.setTransferProgress(progress_callback);

    AlibabaCloud::OSS::ClientConfiguration conf;
    AlibabaCloud::OSS::OssClient oss_client(m_endPoint, m_accessKeyId, m_secretAccessKey, m_token, conf);
    auto outcome = oss_client.PutObject(request);

    if (!outcome.isSuccess()) {
        m_lastError.code = outcome.error().Code();
        m_lastError.message = outcome.error().Message();
        m_lastError.requestId = outcome.error().RequestId();
        nRet = 1;
        return nRet;
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
