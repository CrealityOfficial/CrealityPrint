#ifndef slic3r_UploadFile_hpp_
#define slic3r_UploadFile_hpp_
#include <string>
#include <functional>
#include <exception>
#include <utility>
#include "nlohmann/json.hpp"
#include "print_manage/UploadCancellation.hpp"

#include "alibabacloud/oss/OssClient.h"
using namespace nlohmann;
namespace Slic3r {
namespace GUI {
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
        ~UploadFile();

        int getAliyunInfo();
        int getOssInfo();
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
        bool isCancelled() const {
            return RemotePrint::is_upload_cancelled(m_cancel_token);
        }
    private:
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
    };

}
}

#endif
