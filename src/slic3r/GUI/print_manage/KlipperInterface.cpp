#include "KlipperInterface.hpp"
#include "RemotePrinterManager.hpp"
#include <curl/curl.h>
#include <fstream>
#include <iostream>

namespace RemotePrint {
KlipperInterface::KlipperInterface() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

KlipperInterface::~KlipperInterface() {
    curl_global_cleanup();
}

std::future<void> KlipperInterface::sendFileToDevice(const std::string& serverIp, int port, const std::string& fileName, const std::string& filePath, std::function<void(float)> progressCallback, std::function<void(int)> errorCallback, std::function<void(std::string)> onCompleteCallback) {
    return std::async(std::launch::async, [=]() {
        CURL* curl = curl_easy_init();
        if (!curl) {
            if (errorCallback) {
                errorCallback(CURLE_FAILED_INIT);
            }
            return;
        }

        std::string url = "http://" + serverIp + ":" + std::to_string(port) + urlSuffixUpload + fileName;

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
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progressCallback);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            if (errorCallback) {
                errorCallback(res);
            }
        } else {
            if (progressCallback) {
                progressCallback(100.0f); // Assuming the upload is complete
            }
        }

        curl_easy_cleanup(curl);
    });
}
}