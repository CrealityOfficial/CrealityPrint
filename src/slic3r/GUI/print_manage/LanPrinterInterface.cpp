#include "LanPrinterInterface.hpp"
#include "RemotePrinterManager.hpp"
#include <curl/curl.h>
#include <fstream>
#include <iostream>

namespace RemotePrint {
LanPrinterInterface::LanPrinterInterface() { curl_global_init(CURL_GLOBAL_DEFAULT); }

LanPrinterInterface::~LanPrinterInterface() { curl_global_cleanup(); }

std::future<void> LanPrinterInterface::sendFileToDevice(const std::string&         strIp,
                                                        const std::string&         fileName,
                                                        const std::string&         filePath,
                                                        std::function<void(float)> callback,
                                                        std::function<void(int)>   errorCallback, std::function<void(std::string)> onCompleteCallback)
{
    return std::async(std::launch::async, [=]() {
        CURL* curl = curl_easy_init();
        if (!curl) {
            if (errorCallback) {
                errorCallback(CURLE_FAILED_INIT);
            }
            return;
        }

        std::string url = "http://" + strIp + ":" + std::to_string(cServerPort) + cUrlSuffixUpload + fileName;

        std::ifstream file(filePath, std::ios::in | std::ios::binary);
        if (!file) {
            if (errorCallback) {
                errorCallback(CURLE_READ_ERROR);
            }
            curl_easy_cleanup(curl);
            return;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_READDATA, &file);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(file.tellg()));
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &callback);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            if (errorCallback) {
                errorCallback(res);
            }
        } else {
            if (callback) {
                callback(100.0f); // Assuming the upload is complete
            }
        }

        curl_easy_cleanup(curl);
    });
}
} // namespace RemotePrint
