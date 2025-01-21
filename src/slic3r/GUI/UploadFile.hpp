#ifndef slic3r_UploadFile_hpp_
#define slic3r_UploadFile_hpp_
#include <string>
#include <functional>
#include <exception>
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
        void setProcessCallback(std::function<void(int)> funcProcessCb);
        const LastError& getLastError() { return m_lastError; }
        int uploadFileToAliyun(const std::string& local_path, const std::string& target_path, const std::string& fileName);
        int downloadFileFromAliyun(const std::string& target_path, const std::string& local_path);

    private:
        void ProgressCallback(size_t increment, int64_t transfered, int64_t total, void* userData);
    private:
        std::string m_token = "";
        std::string m_accessKeyId = "";
        std::string m_secretAccessKey = "";
        std::string m_endPoint = "";
        std::string m_bucket = "";
        std::function<void(int)> m_funcProcessCb = nullptr;
        LastError m_lastError;
    };

}
}

#endif
