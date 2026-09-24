#ifndef slic3r_UploadFile_hpp_
#define slic3r_UploadFile_hpp_
#include <string>
#include <functional>
#include <exception>
#include <map>
#include <mutex>
#include <utility>
#include "nlohmann/json.hpp"
#include "print_manage/UploadCancellation.hpp"

#include "alibabacloud/oss/OssClient.h"
using namespace nlohmann;
namespace Slic3r {
namespace GUI {
class OssSdkGuard {
public:
    OssSdkGuard() {
        std::lock_guard<std::mutex> lock(mutex());
        if (users()++ == 0) {
            owns_sdk() = !AlibabaCloud::OSS::IsSdkInitialized();
            if (owns_sdk())
                AlibabaCloud::OSS::InitializeSdk();
        }
    }

    ~OssSdkGuard() {
        std::lock_guard<std::mutex> lock(mutex());
        if (users() > 0 && --users() == 0 && owns_sdk()) {
            AlibabaCloud::OSS::ShutdownSdk();
            owns_sdk() = false;
        }
    }

    OssSdkGuard(const OssSdkGuard&) = delete;
    OssSdkGuard& operator=(const OssSdkGuard&) = delete;

private:
    static std::mutex& mutex() {
        static std::mutex value;
        return value;
    }
    static unsigned int& users() {
        static unsigned int value = 0;
        return value;
    }
    static bool& owns_sdk() {
        static bool value = false;
        return value;
    }
};

struct CloudOssConfig {
    std::string token;
    std::string access_key_id;
    std::string secret_access_key;
    std::string endpoint;
    std::string file_bucket;
    std::string video_bucket;
    std::string cdn_host;

    bool valid_for_video() const {
        return !token.empty() && !access_key_id.empty() && !secret_access_key.empty() &&
               !endpoint.empty() && !video_bucket.empty();
    }
};

class ErrorCodeException : public std::exception {
    private:
        int errorCode;
        std::string funName;
        std::string message;
    public:
        ErrorCodeException(std::string fn,int code,std::string msg) : funName(fn),errorCode(code), message(msg){}
        int code() const {
            return errorCode;
        }
        std::string msg() const {
            return message;
        }
        const char* what() const noexcept override {
            return message.c_str();
        }
};
    using ProgressCallback = std::function<void(int partNumber, int totalParts, double percentage)>;
    class UploadFile
    {
    public:
        struct LastError {
            std::string code;
            std::string message;
            std::string requestId;
        };
        UploadFile();
        explicit UploadFile(std::map<std::string, std::string> request_headers);
        ~UploadFile();
        UploadFile(const UploadFile&) = delete;
        UploadFile& operator=(const UploadFile&) = delete;

        int getAliyunInfo();
        int getOssInfo();
        int getCloudUploadInfo(CloudOssConfig& info) {
            const json value = getCloudUploadInfo();
            auto read_string = [&value](const char* key, std::string& out) {
                const auto it = value.find(key);
                if (it == value.end() || !it->is_string())
                    return false;
                out = it->get<std::string>();
                return !out.empty();
            };
            info = CloudOssConfig{};
            const bool token_ok = read_string("token", info.token);
            const bool access_ok = read_string("accessKeyId", info.access_key_id);
            const bool secret_ok = read_string("secretAccessKey", info.secret_access_key);
            const bool endpoint_ok = read_string("endPoint", info.endpoint);
            read_string("bucket", info.file_bucket);
            const bool video_bucket_ok = read_string("video_bucket", info.video_bucket);
            read_string("cdnHost", info.cdn_host);
            return token_ok && access_ok && secret_ok && endpoint_ok && video_bucket_ok ? 0 : -1;
        }
        json getCloudUploadInfo();
        int uploadGcodeToCXCloud(const std::string& name, const std::string&fileName, std::function<void(std::string)> onCompleteCallback=nullptr);
        void setProcessCallback(std::function<void(int,double)> funcProcessCb);
        void UploadProgressCallback(int partNumber, int totalParts, double percentage);
        const LastError& getLastError() { return m_lastError; }
        int uploadFileToAliyun(const std::string& local_path, const std::string& target_path, const std::string& fileName);
        int downloadFileFromAliyun(const std::string& target_path, const std::string& local_path);
        void setCancel(bool cancel) {
            RemotePrint::set_upload_cancelled(m_cancel_token, cancel);
        }
        void setCancelToken(RemotePrint::UploadCancelToken cancel_token) {
            m_cancel_token = cancel_token ? std::move(cancel_token) : RemotePrint::make_upload_cancel_token();
        }
        vector<AlibabaCloud::OSS::Part> uploadParts(AlibabaCloud::OSS::OssClient& client,
                       const std::string& bucketName,
                       const std::string& objectName,
                       const std::string& uploadId,
                       const std::string& filePath,
                       ProgressCallback callback = nullptr);
    private:
        void ProgressCallback(size_t increment, int64_t transfered, int64_t total, void* userData);
        std::map<std::string, std::string> requestHeaders() const;
        bool isCancelled() const {
            return RemotePrint::is_upload_cancelled(m_cancel_token);
        }
    private:
        OssSdkGuard m_oss_sdk_guard;
        std::string m_token = "";
        std::string m_accessKeyId = "";
        std::string m_secretAccessKey = "";
        std::string m_endPoint = "";
        std::string m_bucket = "";
        std::string m_video_bucket = "";
        std::string m_cdnHost = "";
        std::function<void(int,double)> m_funcProcessCb = nullptr;
        LastError m_lastError;
        RemotePrint::UploadCancelToken m_cancel_token = RemotePrint::make_upload_cancel_token();
        std::map<std::string, std::string> m_request_headers;
        bool m_has_request_headers {false};
    };

}
}

#endif
