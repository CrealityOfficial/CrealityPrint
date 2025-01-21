#include "KlipperCXInterface.hpp"
#include "slic3r/Utils/Http.hpp"
#include <boost/log/trivial.hpp>
#include <curl/curl.h>
#include <string>
#include "nlohmann/json_fwd.hpp"
#include "slic3r/GUI/GUI.hpp"

namespace RemotePrint {
KlipperCXInterface::KlipperCXInterface() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

KlipperCXInterface::~KlipperCXInterface() {
    curl_global_cleanup();
}

std::future<void> KlipperCXInterface::sendFileToDevice(const std::string& serverIp, int port, const std::string& uploadFileName, const std::string& localFilePath, std::function<void(float)> progressCallback, std::function<void(int)> uploadStatusCallback, std::function<void(std::string)> onCompleteCallback) {
    return std::async(std::launch::async, [=]() {
        try{
        m_upload_file.setProcessCallback(progressCallback);
            int nRet = m_upload_file.getAliyunInfo();
            if (nRet != 0)
            {
                uploadStatusCallback(1);
                return;
            }
                
            nRet = m_upload_file.getOssInfo();
            if (nRet != 0)
            {
                uploadStatusCallback(2);
                return;
            }

            std::string target_name = uploadFileName;
            std::string local_target_path = wxString(localFilePath).utf8_str().data();
            std::string target_path = "model/slice/" + Slic3r::GUI::get_file_md5(local_target_path) + ".gcode.gz";
            nRet                    = m_upload_file.uploadFileToAliyun(local_target_path, target_path, target_name);
            if (nRet != 0)
            {
                uploadStatusCallback(3);
                return;
            }

            
            nRet = m_upload_file.uploadGcodeToCXCloud(target_name, target_path, onCompleteCallback);
            uploadStatusCallback(nRet);
        }catch(Slic3r::GUI::ErrorCodeException& e){
            uploadStatusCallback(e.code());
        }
    });
}
}