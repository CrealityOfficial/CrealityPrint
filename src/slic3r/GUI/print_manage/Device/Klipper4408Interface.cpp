#include "Klipper4408Interface.hpp"
#include "slic3r/Utils/Http.hpp"
#include <boost/log/trivial.hpp>
#include <curl/curl.h>
#include <string>
#include "slic3r/GUI/I18N.hpp"
#include "../AppUtils.hpp"
#include <wx/string.h>
namespace RemotePrint {
Klipper4408Interface::Klipper4408Interface() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

Klipper4408Interface::~Klipper4408Interface() {
    curl_global_cleanup();
}
bool isGCodeFile(const boost::filesystem::path& filePath) {
    // 获取文件扩展名
    boost::filesystem::path extension = filePath.extension();
    // 将扩展名转换为小写并检查是否为 .gcode
    std::string extStr = extension.string();
    for (char& c : extStr) {
        c = std::tolower(c);
    }
    return extStr == ".gcode";
}
std::future<void> Klipper4408Interface::sendFileToDevice(const std::string& serverIp, int port, const std::string& uploadFileName, const std::string& localFilePath, std::function<void(float,double)> progressCallback, std::function<void(int)> uploadStatusCallback, std::function<void(std::string)> onCompleteCallback, UploadCancelToken cancelToken) {
    return std::async(std::launch::async, [=]() {
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " start";
    bool res = false;
    if (is_upload_cancelled(cancelToken)) {
        if (uploadStatusCallback)
            uploadStatusCallback(601);
        return;
    }

    std::string urlUpload = "http://" + serverIp + ":" + std::to_string(80) + "/upload/" + Slic3r::Http::url_encode(uploadFileName);

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
    progressCallback(0.1f,0.0f);
    //Disable MD5
    //     if(isGCodeFile(localFilePath)) {
    //         std::string md5 = DM::AppUtils::MD5(localFilePath); 
    //         http.header("MD5", md5);
    //     }
    progressCallback(1.0f,0.0f);
    std::string filePath =  wxString::FromUTF8(localFilePath.c_str()).ToStdString();
    //static time_t last_time = 0;
    time_t last_time = time(NULL);
    int percent = 0;
    http.header("Content-Type", "multipart/form-data")
        .mime_form_add_file(temp_upload_name, filePath.c_str()).timeout_connect(5)
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
            if (uploadStatusCallback && !is_upload_cancelled(cancelToken)) {
                uploadStatusCallback(status == 0 ? CURLE_HTTP_RETURNED_ERROR : status);
            }
            res = false;
        })
        .on_progress([&](Slic3r::Http::Progress progress, bool& cancel) {
             cancel = is_upload_cancelled(cancelToken);
             if (cancel) {
                // Upload was canceled
                BOOST_LOG_TRIVIAL(info) << boost::format("%1%: Upload canceled") % uploadFileName;
                res = false;
                return;
            }
            if (progressCallback) {
                
                time_t now = time(NULL);
                double speed = 0;
                if(now != last_time) {
                    curl_off_t bytes_sent = progress.ulnow;
                    double time_elapsed = difftime(now, last_time);
                    speed = bytes_sent / time_elapsed / 1024; // 字节/秒
                }
                if(progress.ultotal > 0) {
                    float tpercent = static_cast<float>(progress.ulnow) / progress.ultotal * 100.0f;
                    if(percent == round(tpercent)) {
                        return ;
                    }
                    percent = round(tpercent);
                    progressCallback(percent<1.0f?1.0f:percent, progress.upload_spd/1024);
                }
            }
           
        })
        .perform_sync();
        if (!res && uploadStatusCallback) {
            if (http.is_cancelled() || is_upload_cancelled(cancelToken)) {
                uploadStatusCallback(601); // 601 表示取消成功
            } else {
                //uploadStatusCallback(CURLE_HTTP_RETURNED_ERROR);
            }
        }
    });

    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " end!!!";
}

void Klipper4408Interface::cancelSendFileToDevice(const UploadCancelToken& cancelToken)
{
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " start";
    request_upload_cancel(cancelToken);
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " end";
}

}
