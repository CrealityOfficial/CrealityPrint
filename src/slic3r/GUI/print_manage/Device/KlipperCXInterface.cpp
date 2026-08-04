#include "KlipperCXInterface.hpp"
#include "slic3r/Utils/Http.hpp"
#include <boost/log/trivial.hpp>
#include <curl/curl.h>
#include <exception>
#include <stdexcept>
#include <string>
#include "nlohmann/json_fwd.hpp"
#include "slic3r/GUI/GUI.hpp"
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <vector>

namespace {
std::string get_file_md5_cancellable(const std::string& path,
                                     const RemotePrint::UploadCancelToken& cancel_token)
{
    RemotePrint::UploadFileUseGuard file_use(cancel_token);
    if (!file_use)
        return {};

    boost::nowide::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Failed to open file: " + path);

    using boost::uuids::detail::md5;
    md5 md5_hash;
    md5::digest_type digest {};
    std::vector<char> buffer(1024 * 1024);

    while (input) {
        if (RemotePrint::is_upload_cancelled(cancel_token))
            return {};

        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize read_count = input.gcount();
        if (read_count > 0)
            md5_hash.process_bytes(buffer.data(), static_cast<std::size_t>(read_count));
    }

    md5_hash.get_digest(digest);
    std::string result;
    boost::algorithm::hex(digest, digest + std::size(digest), std::back_inserter(result));
    return result;
}

std::string build_gcode_upload_path(const std::string& local_file_path,
                                    const std::string& display_name,
                                    const RemotePrint::UploadCancelToken& cancel_token)
{
    std::string normalized_name = display_name.empty() ? "unnamed" : display_name;
    if (boost::iends_with(normalized_name, ".gcode"))
        normalized_name.resize(normalized_name.size() - 6);

    const std::string content_md5 = get_file_md5_cancellable(local_file_path, cancel_token);
    if (content_md5.empty() && RemotePrint::is_upload_cancelled(cancel_token))
        return {};
    const std::string hash_input  = normalized_name + ":" + content_md5;

    using boost::uuids::detail::md5;
    md5              md5_hash;
    md5::digest_type md5_digest{};
    std::string      md5_digest_str;

    md5_hash.process_bytes(hash_input.data(), hash_input.size());
    md5_hash.get_digest(md5_digest);
    boost::algorithm::hex(md5_digest, md5_digest + std::size(md5_digest), std::back_inserter(md5_digest_str));

    return "model/slice/" + md5_digest_str + ".gcode.gz";
}
}

namespace RemotePrint {
KlipperCXInterface::KlipperCXInterface() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

KlipperCXInterface::~KlipperCXInterface() {
    curl_global_cleanup();
}
void KlipperCXInterface::cancelSendFileToDevice(const UploadCancelToken& cancelToken)
{
    request_upload_cancel(cancelToken);
}
std::future<void> KlipperCXInterface::sendFileToDevice(const std::string& serverIp, int port, const std::string& uploadFileName, const std::string& localFilePath, std::function<void(float,double)> progressCallback, std::function<void(int)> uploadStatusCallback, std::function<void(std::string)> onCompleteCallback, UploadCancelToken cancelToken) {
    auto upload_file = std::make_shared<Slic3r::GUI::UploadFile>();
    upload_file->setCancelToken(cancelToken);
    return std::async(std::launch::async, [=]() {
        try{
            BOOST_LOG_TRIVIAL(warning) << "[CloudUpload] stage=get_aliyun_info begin, device=" << serverIp;
            upload_file->setProcessCallback(progressCallback);
            int nRet = upload_file->getAliyunInfo();
            BOOST_LOG_TRIVIAL(warning) << "[CloudUpload] stage=get_aliyun_info end, result=" << nRet;
            if (nRet != 0)
            {
                if (uploadStatusCallback)
                    uploadStatusCallback(nRet == 601 ? 601 : 1);
                return;
            }
                
            BOOST_LOG_TRIVIAL(warning) << "[CloudUpload] stage=get_oss_info begin";
            nRet = upload_file->getOssInfo();
            BOOST_LOG_TRIVIAL(warning) << "[CloudUpload] stage=get_oss_info end, result=" << nRet;
            if (nRet != 0)
            {
                if (uploadStatusCallback)
                    uploadStatusCallback(nRet == 601 ? 601 : 2);
                return;
            }

            std::string target_name = uploadFileName;
            if (boost::iends_with(uploadFileName, ".gcode")) {
                target_name = uploadFileName.substr(0, uploadFileName.size() - 6);
            }
                ;
            //std::string local_target_path = wxString(localFilePath).utf8_str().data();
            BOOST_LOG_TRIVIAL(warning) << "[CloudUpload] stage=build_upload_path begin";
            std::string target_path = build_gcode_upload_path(localFilePath, target_name, cancelToken);
            BOOST_LOG_TRIVIAL(warning) << "[CloudUpload] stage=build_upload_path end, cancelled="
                                       << is_upload_cancelled(cancelToken);
            if (is_upload_cancelled(cancelToken)) {
                if (uploadStatusCallback)
                    uploadStatusCallback(601);
                return;
            }
            BOOST_LOG_TRIVIAL(warning) << "[CloudUpload] stage=oss_upload begin";
            nRet = upload_file->uploadFileToAliyun(localFilePath, target_path, target_name);
            BOOST_LOG_TRIVIAL(warning) << "[CloudUpload] stage=oss_upload end, result=" << nRet;
            if (nRet != 0)
            {
                if (uploadStatusCallback)
                    uploadStatusCallback(nRet);
                return;
            }

            if (is_upload_cancelled(cancelToken)) {
                if (uploadStatusCallback)
                    uploadStatusCallback(601);
                return;
            }

            BOOST_LOG_TRIVIAL(warning) << "[CloudUpload] stage=cloud_register begin";
            nRet = upload_file->uploadGcodeToCXCloud(target_name, target_path, onCompleteCallback);
            BOOST_LOG_TRIVIAL(warning) << "[CloudUpload] stage=cloud_register end, result=" << nRet;
            if (uploadStatusCallback)
                uploadStatusCallback(nRet);
        } catch (const Slic3r::GUI::ErrorCodeException& e) {
            BOOST_LOG_TRIVIAL(error) << "[CloudUpload] ErrorCodeException, code=" << e.code()
                                     << ", message=" << e.msg();
            if (uploadStatusCallback)
                uploadStatusCallback(e.code());
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "[CloudUpload] exception: " << e.what();
            if (uploadStatusCallback)
                uploadStatusCallback(1000);
        }
    });
}
}
