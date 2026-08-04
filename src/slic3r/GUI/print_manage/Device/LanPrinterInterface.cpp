#include "LanPrinterInterface.hpp"
#include "../RemotePrinterManager.hpp"
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <filesystem>

namespace RemotePrint {
LanPrinterInterface::LanPrinterInterface() { curl_global_init(CURL_GLOBAL_DEFAULT); }

LanPrinterInterface::~LanPrinterInterface() { curl_global_cleanup(); }

std::future<void> LanPrinterInterface::sendFileToDevice(const std::string&         strIp,
                                                        const std::string&         fileName,
                                                        const std::string&         filePath,
                                                        std::function<void(float,double)> callback,
                                                        std::function<void(int)>   errorCallback,
                                                        std::function<void(std::string)> onCompleteCallback,
                                                        UploadCancelToken cancelToken)
{
    return std::async(std::launch::async, [=]() {
        if (is_upload_cancelled(cancelToken)) {
            if (errorCallback)
                errorCallback(601);
            return;
        }

        // 1. 初始化CURL

        CURL* curl = curl_easy_init();
        if (!curl)
        {
            if (errorCallback)
                errorCallback(CURLE_FAILED_INIT);
            return;
        }

        // 2. RAII保护CURL句柄
        struct CurlGuard
        {
            CURL* handle;
            ~CurlGuard()
            {
                if (handle) curl_easy_cleanup(handle);
            }
        } guard{ curl };

        UploadFileUseGuard file_use(cancelToken);
        if (!file_use) {
            if (errorCallback)
                errorCallback(601);
            return;
        }

        FILE* fd = nullptr;
#if defined(_MSC_VER) || defined(__MINGW64__)
        fd = boost::nowide::fopen(filePath.c_str(), "rb");
#elif defined(__GNUC__) && defined(_LARGEFILE64_SOURCE)
        fd = fopen64(filePath.c_str(), "rb");
#else
        fd = fopen(filePath.c_str(), "rb");
#endif

        if (fd == nullptr)
        {
            if (errorCallback)
                errorCallback(CURLE_READ_ERROR);
            return;
        }

        struct FileGuard
        {
            FILE* fd;
            ~FileGuard()
            {
                if (fd) fclose(fd);
            }
        } fileGuard{ fd };

        fseek(fd, 0, SEEK_END);  
        curl_off_t fileSize = ftell(fd);  
        fseek(fd, 0, SEEK_SET);

        if (callback)
            callback(10, 0);

        std::string sFileName = fileName;
        sFileName = std::regex_replace(sFileName, std::regex("[\\\\/:*?\"'<>|#&=+]"), "");
        // 4. 配置基础FTP选项
        const std::string url = "ftp://" + strIp + "/mmcblk0p1/creality/gztemp/" + sFileName;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_READDATA, fd);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, fileSize);

        struct ProgressContext
        {
            UploadCancelToken token;
            std::function<void(float, double)> callback;
        } progressContext {cancelToken, callback};

        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION,
            +[](void* data, curl_off_t, curl_off_t, curl_off_t uploadTotal, curl_off_t uploaded) -> int {
                auto* context = static_cast<ProgressContext*>(data);
                if (is_upload_cancelled(context->token))
                    return 1;

                if (context->callback && uploadTotal > 0) {
                    const float percent = static_cast<float>(uploaded) /
                                          static_cast<float>(uploadTotal) * 100.0f;
                    context->callback(percent, 0);
                }
                return 0;
            });
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progressContext);

        //设置超时
        curl_easy_setopt(curl, CURLOPT_FTP_RESPONSE_TIMEOUT, 30L);

        // 6. 执行传输
        CURLcode res = curl_easy_perform(curl);


        if (res != CURLE_OK)
        {
            if (errorCallback)
            {
                errorCallback(is_upload_cancelled(cancelToken) ? 601 : res);
            }
        }
        else
        {
            if (callback) 
                callback(100.0f,0.0f); // Assuming the upload is complete

            std::string body = "{ \"code\": 200, \"message\" : \"OK\" }";
            if(onCompleteCallback)
                onCompleteCallback(body);
        }
     });
}

} // namespace RemotePrint
