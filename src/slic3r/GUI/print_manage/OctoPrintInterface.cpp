#include "OctoPrintInterface.hpp"
#include "RemotePrinterManager.hpp"
#include <curl/curl.h>
#include <fstream>
#include <iostream>

namespace RemotePrint {
OctoPrintInterface::OctoPrintInterface() { curl_global_init(CURL_GLOBAL_DEFAULT); }

OctoPrintInterface::~OctoPrintInterface() { curl_global_cleanup(); }

void OctoPrintInterface::sendFileToDevice(const std::string& strServerIp, const std::string& strToken, const std::string& filePath)
{
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize CURL" << std::endl;
        return;
    }

    std::string strUrl = "http://" + strServerIp + ":" + std::to_string(cServerPort) + cUrlSuffixFile;

    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        curl_easy_cleanup(curl);
        return;
    }

    curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READDATA, &file);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, RemotePrint::read_callback);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(file.tellg()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_slist_append(nullptr, ("X-Api-Key: " + strToken).c_str()));

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
    }

    curl_easy_cleanup(curl);
}
} // namespace RemotePrint