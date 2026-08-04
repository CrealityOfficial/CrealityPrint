#include "KlipperInterface.hpp"
#include "../RemotePrinterManager.hpp"
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

std::string KlipperInterface::extractIP(const std::string& str) 
{
    size_t pos = str.find('(');  
    if (pos != std::string::npos) {
        return str.substr(0, pos); 
    }
    return str;  
}

void KlipperInterface::cancelSendFileToDevice(const UploadCancelToken& cancelToken)
{
    request_upload_cancel(cancelToken);
}

std::future<void> KlipperInterface::sendFileToDevice(const std::string& serverIp, int port, const std::string& uploadFileName, const std::string& localFilePath, std::function<void(float, double)> progressCallback, std::function<void(int)> uploadStatusCallback, std::function<void(std::string)> onCompleteCallback, UploadCancelToken cancelToken)
{
    std::string ip = extractIP(serverIp);
    std::string urlLoad = "/server/files/upload";
    return std::async(std::launch::async, [=]() {
        bool res = false;
        if (is_upload_cancelled(cancelToken)) {
            if (uploadStatusCallback)
                uploadStatusCallback(601);
            return;
        }

        std::string urlUpload = "http://" + ip + ":" + std::to_string(port) + urlLoad;

        BOOST_LOG_TRIVIAL(info) << boost::format("%1%: Uploading file %2% to %3%") % uploadFileName % localFilePath % urlUpload;

        UploadFileUseGuard file_use(cancelToken);
        if (!file_use) {
            if (uploadStatusCallback)
                uploadStatusCallback(601);
            return;
        }
        auto http = Slic3r::Http::post(urlUpload);
        http.enable_active_cancel();
        UploadRequestCancelWatcher cancel_watcher(cancelToken, [&http] { http.cancel(); });

        std::string temp_upload_name = uploadFileName;

        http.clear_header();
        progressCallback(0.1f, 0.0f);

        progressCallback(1.0f, 0.0f);
        std::string filePath = wxString::FromUTF8(localFilePath.c_str()).ToStdString();
        time_t last_time = time(NULL);
        int percent = 0;
        http.header("Content-Type", "multipart/form-data")
            .mime_form_add_file(temp_upload_name, filePath.c_str()).timeout_connect(5)
            .on_complete([&](std::string body, unsigned status){
                if (status == 200 || status == 201)
                {
                    res = true;
                }
                if (!res)
                {
                    BOOST_LOG_TRIVIAL(error) << boost::format("%1%: Request completed but no SUCCESS message was received.") % uploadFileName;
                    if (uploadStatusCallback)
                    {
                        uploadStatusCallback(CURLE_HTTP_RETURNED_ERROR);
                    }
                }
                else
                {
                    if (uploadStatusCallback)
                    {
                        uploadStatusCallback(CURLE_OK); // the upload is complete
                    }
                    BOOST_LOG_TRIVIAL(info) << "File uploaded successfully";
                }
                if (onCompleteCallback)onCompleteCallback(body);
             })
            .on_error([&](std::string body, std::string error, unsigned status) {
                    BOOST_LOG_TRIVIAL(error) << boost::format("%1%: Error uploading file: %2%, HTTP %3%, body: `%4%`") % uploadFileName % error % status %
                        body;
                    if (uploadStatusCallback)
                    {
                        uploadStatusCallback(CURLE_HTTP_RETURNED_ERROR);
                    }
                    res = false;
            })
            .on_progress([&](Slic3r::Http::Progress progress, bool& cancel) {
             cancel = is_upload_cancelled(cancelToken);
             if (progressCallback)
             {
                 time_t now = time(NULL);
                 double speed = 0;
                 if (now != last_time)
                 {
                     curl_off_t bytes_sent = progress.ulnow;
                     double time_elapsed = difftime(now, last_time);
                     speed = bytes_sent / time_elapsed / 1024; // bytes per second
                 }
                 if (progress.ultotal > 0)
                 {
                     float tpercent = static_cast<float>(progress.ulnow) / progress.ultotal * 100.0f;
                     if (percent == round(tpercent))
                     {
                         return;
                     }
                     percent = round(tpercent);
                     progressCallback(percent < 1.0f ? 1.0f : percent, speed);
                 }
             }
             if (cancel)
             {
                 // Upload was canceled
                 BOOST_LOG_TRIVIAL(info) << boost::format("%1%: Upload canceled") % uploadFileName;
                 res = false;
             }
            })
             .perform_sync();
                  if (!res && uploadStatusCallback)
                  {
                      if (is_upload_cancelled(cancelToken))
                      {
                          uploadStatusCallback(601); // 601 means cancel succeeded
                      } else 
                      {
                          uploadStatusCallback(CURLE_HTTP_RETURNED_ERROR);
                      }
                  }
        });
}
}
