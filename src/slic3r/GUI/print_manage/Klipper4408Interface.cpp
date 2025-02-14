#include "Klipper4408Interface.hpp"
#include "slic3r/Utils/Http.hpp"
#include <boost/log/trivial.hpp>
#include <curl/curl.h>
#include <string>
#include "slic3r/GUI/I18N.hpp"
#include "AppUtils.hpp"
#include <wx/string.h>
namespace RemotePrint {
Klipper4408Interface::Klipper4408Interface() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

Klipper4408Interface::~Klipper4408Interface() {
    curl_global_cleanup();
}

std::future<void> Klipper4408Interface::sendFileToDevice(const std::string& serverIp, int port, const std::string& uploadFileName, const std::string& localFilePath, std::function<void(float)> progressCallback, std::function<void(int)> uploadStatusCallback, std::function<void(std::string)> onCompleteCallback) {
    return std::async(std::launch::async, [=]() {
    bool res = false;

    std::string urlUpload = "http://" + serverIp + ":" + std::to_string(80) + "/upload/" + Slic3r::Http::url_encode(uploadFileName);

    BOOST_LOG_TRIVIAL(info) << boost::format("%1%: Uploading file %2% to %3%") % uploadFileName % localFilePath % urlUpload;

    auto http = Slic3r::Http::post(urlUpload);
    m_pHttp   = &http;

    std::string temp_upload_name = uploadFileName;
    std::string md5 = DM::AppUtils::MD5(localFilePath); 
    http.clear_header();
    http.header("MD5", md5);
    std::string filePath =  wxString::FromUTF8(localFilePath.c_str()).ToStdString();
    http.header("Content-Type", "multipart/form-data")
        .mime_form_add_file(temp_upload_name, filePath.c_str())
        .on_complete([&](std::string body, unsigned status) {
            BOOST_LOG_TRIVIAL(debug) << boost::format("%1%: File uploaded: HTTP %2%: %3%") % uploadFileName % status % body;
            res = boost::icontains(body, "OK");
            if (!res) {
                BOOST_LOG_TRIVIAL(error) << boost::format("%1%: Request completed but no SUCCESS message was received.") % uploadFileName;
                if (uploadStatusCallback) {
                    uploadStatusCallback(CURLE_HTTP_RETURNED_ERROR);
                }
            } else {
                if (uploadStatusCallback) {
                    uploadStatusCallback(CURLE_OK); // the upload is complete
                }
                BOOST_LOG_TRIVIAL(info) << "File uploaded successfully";
            }
            if(onCompleteCallback)onCompleteCallback(body);
        })
        .on_error([&](std::string body, std::string error, unsigned status) {
            BOOST_LOG_TRIVIAL(error) << boost::format("%1%: Error uploading file: %2%, HTTP %3%, body: `%4%`") % uploadFileName % error % status %
                                            body;
            if (uploadStatusCallback) {
                uploadStatusCallback(CURLE_HTTP_RETURNED_ERROR);
            }
            res = false;
        })
        .on_progress([&](Slic3r::Http::Progress progress, bool& cancel) {
            if (progressCallback) {
                if(progress.ultotal > 0) {
                    progressCallback(static_cast<float>(progress.ulnow) / progress.ultotal * 100.0f);
                }
            }
            if (cancel) {
                // Upload was canceled
                BOOST_LOG_TRIVIAL(info) << boost::format("%1%: Upload canceled") % uploadFileName;
                res = false;
            }
        })
        .perform_sync();

        if (!res && uploadStatusCallback) {
            if (m_bCancelSend) {
                uploadStatusCallback(601); // 601 表示取消成功
            } else {
                uploadStatusCallback(CURLE_HTTP_RETURNED_ERROR);
            }
        }
    });
}

void Klipper4408Interface::cancelSendFileToDevice()
{
    m_bCancelSend = true;
    if (m_pHttp != nullptr) {
        m_pHttp->cancel();
    }
}

}