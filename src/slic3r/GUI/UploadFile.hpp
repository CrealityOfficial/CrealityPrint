#ifndef slic3r_UploadFile_hpp_
#define slic3r_UploadFile_hpp_
#include <string>
#include <functional>
#include <exception>
#include "alibabacloud/oss/OssClient.h"
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
        int uploadGcodeToCXCloud(const std::string& name, const std::string&fileName, std::function<void(std::string)> onCompleteCallback=nullptr);
        void setProcessCallback(std::function<void(int,double)> funcProcessCb);
        void UploadProgressCallback(int partNumber, int totalParts, double percentage);
        const LastError& getLastError() { return m_lastError; }
        int uploadFileToAliyun(const std::string& local_path, const std::string& target_path, const std::string& fileName);
        int downloadFileFromAliyun(const std::string& target_path, const std::string& local_path);
        void setCancel(bool cancel) {m_cancel=cancel;}
        vector<AlibabaCloud::OSS::Part> uploadParts(AlibabaCloud::OSS::OssClient& client,
                       const std::string& bucketName,
                       const std::string& objectName,
                       const std::string& uploadId,
                       const std::string& filePath,
                       ProgressCallback callback = nullptr);
    private:
        void ProgressCallback(size_t increment, int64_t transfered, int64_t total, void* userData);
    private:
        std::string m_token = "";
        std::string m_accessKeyId = "";
        std::string m_secretAccessKey = "";
        std::string m_endPoint = "";
        std::string m_bucket = "";
        std::function<void(int,double)> m_funcProcessCb = nullptr;
        LastError m_lastError;
        bool m_cancel = false;
    };

}
}

#endif
