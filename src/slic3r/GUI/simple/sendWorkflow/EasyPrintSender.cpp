#include "EasyPrintSender.hpp"
#include "CxCloudPrintExecutor.hpp"
#include "../../print_manage/RemotePrinterManager.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cstdint>
#include <utility>
#include <thread>

#include "../../print_manage/App/SendToPrinter.hpp"
#include "slic3r/GUI/I18N.hpp"
#include "slic3r/GUI/wxExtensions.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "libslic3r_version.h"
#include "libslic3r/Utils.hpp"
#include "../../print_manage/data/DataCenter.hpp"
#include <fstream>
#include <string>
#include "slic3r/GUI/Notebook.hpp"
#include "slic3r/GUI/simple/DeviceListSimple.hpp"
#include "slic3r/GUI/print_manage/Utils.hpp"
#include "libslic3r/Time.hpp"

EasyPrintSender::EasyPrintSender()
{
    m_cloud_print_executor = std::make_shared<CxCloudPrintExecutor>();
}

EasyPrintSender::~EasyPrintSender()
{
}



void EasyPrintSender::setCloudClosedLoopEnabled(bool enabled)
{
    m_cloud_closed_loop_enabled = enabled;
}

void EasyPrintSender::setCloudPrintCallbacks(CloudPrintCallbacks callbacks)
{
    m_cloud_print_callbacks = std::move(callbacks);
}

bool EasyPrintSender::tryStartPrintCloudClosedLoop(const std::string&    deviceId,
                                                   const std::string&    fileKey,
                                                   const nlohmann::json& printData,
                                                   const std::string&    uploadResult)
{
    (void) fileKey;
    (void) deviceId;

    if (!m_cloud_closed_loop_enabled || !m_cloud_print_executor)
        return false;

    CloudPrintRequest request;
    request.device_name = printData.value("device_address", std::string());
    if (request.device_name.empty())
        request.device_name = printData.value("printer_name", std::string());
    request.printer_name = printData.value("printer_name", std::string());
    request.tb_id = printData.value("tb_id", std::string());
    request.upload_name = printData.value("upload_gcode_name", std::string());
    request.upload_result_body = uploadResult;
    request.is_multi_color_device = printData.value("is_multi_color_device", false);
    request.open_cfs = printData.value("open_cfs", 0);
    request.print_calibration = printData.value("print_calibration", 1);

    if (printData.contains("color_match_info") && printData["color_match_info"].is_array()) {
        for (const auto& item : printData["color_match_info"]) {
            CloudPrintMaterialItem material;
            material.extruder_id = item.value("extruderId", item.value("extruder_id", 0));
            material.box_id = item.value("boxId", item.value("box_id", -1));
            material.box_type = item.value("boxType", item.value("box_type", -1));
            material.material_id = item.value("materialId", item.value("material_id", -1));
            material.filament_type = item.value("extruderFilamentType", item.value("filamentType", item.value("filament_type", std::string())));
            material.extruder_color = item.value("extruderColor", item.value("extruder_color", std::string()));
            material.match_color = item.value("matchColor", item.value("match_color", std::string()));
            material.c_id = item.value("cId", item.value("c_id", std::string()));
            material.slot_label = item.value("slotLabel", item.value("slot_label", std::string()));
            request.materials.push_back(std::move(material));
        }
    }

    if (!request.is_multi_color_device)
        request.is_multi_color_device = request.materials.size() > 1;

    m_cloud_print_executor->start(request, {
        [callbacks = m_cloud_print_callbacks](int progress, const std::string& stage, const std::string& message) {
            if (callbacks.onProgress)
                callbacks.onProgress(progress, stage, message);
        },
        [callbacks = m_cloud_print_callbacks](const nlohmann::json& result) {
            if (callbacks.onSuccess)
                callbacks.onSuccess(result);
        },
        [callbacks = m_cloud_print_callbacks](const std::string& code, const std::string& message) {
            if (callbacks.onError)
                callbacks.onError(code, message);
        }
    });

    return true;
}
void EasyPrintSender::logMessage(const std::string& tag, const std::string& content)
{
#define _DEBUG1
#ifdef _DEBUG1
    std::ofstream logFile("D:/log.txt", std::ios::app);
    if (logFile.is_open()) {
        std::time_t now = std::time(nullptr);
        char        buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

        logFile << "[" << buf << "][" << tag << "]" << content << "\n ";

        logFile.close();
    }
#endif
}

static std::string baseName(const std::string& fullPath)
{
    try {
        return std::filesystem::path(fullPath).filename().string();
    } catch (...) {
        return fullPath;
    }
}

std::string EasyPrintSender::getDeviceIp()
{
    // Prefer the real-time device data from DataCenter to resolve the current device address.
    // The same physical device may have both a LAN connection (deviceType=0) and a Creality Cloud
    // bound connection (deviceType=1), sharing the same mac. DataCenter's current device
    // (_get_acive_device) already selects the address by the following priority:
    //   - When both LAN and cloud exist: return LAN if it is online, otherwise return cloud;
    //   - When only cloud exists: return cloud.
    // So when the LAN connection is offline (or removed) while the cloud connection is still online,
    // it returns the cloud device address, avoiding jumping to an offline, non-interactive LAN detail page.
    try {
        const DM::Device& cur_dev = DM::DataCenter::Ins().get_current_device_data();
        if (cur_dev.valid && !cur_dev.address.empty()) {
            return cur_dev.address;
        }
    } catch (std::exception& e) {
        logMessage("EasyPrintSender::getDeviceIp", std::string("read DataCenter current device failed: ") + e.what());
    }

    // Fallback: when DataCenter has no valid real-time data, read the current device address
    // from the persisted deviceInfo.json.
    try {

        boost::filesystem::path device_file = boost::filesystem::path(Slic3r::data_dir()) / "deviceInfo.json";
        if (!boost::filesystem::exists(device_file)) {
            device_file = boost::filesystem::path(Slic3r::data_dir()).parent_path().parent_path() / "Creative3D/deviceInfo.json";
        }
        std::ifstream  ifs(device_file.string());

        nlohmann::json j = nlohmann::json::parse(ifs);

        if (j.contains("current_device") && j["current_device"].contains("mac")) {
            std::string curMac = j["current_device"]["mac"];
            for (auto& group : j["groups"]) {
                if (!group.contains("list") || group["list"].is_null())
                    continue;
                for (auto& dev : group["list"]) {
                    if (dev["mac"] == curMac) {
                        if (dev.contains("address") && !dev["address"].is_null() && !dev["address"].get<std::string>().empty()) {
                            return dev["address"];
                        } else {
                            return curMac; // fallback to MAC when no valid address
                        }
                    }
                }
            }
        }
        logMessage("EasyPrintSender::getDeviceIp", "闂佸搫鐗滄禍婵囩珶濮椻偓瀹曟岸骞忓畝濠傛櫗婵?IP");
        logMessage("EasyPrintSender::getDeviceIp", "no current device ip found");
    } catch (std::exception& e) {
        logMessage("EasyPrintSender::getDeviceIp", std::string("闁荤喐鐟辩徊楣冩�?deviceInfo.json 闂佸憡鍨跺浠嬪�? ") + e.what());
        logMessage("EasyPrintSender::getDeviceIp", std::string("read deviceInfo.json failed: ") + e.what());
    }

    return "";
}

#include <algorithm>

static bool equalsIgnoreCase(const std::string& a, const std::string& b)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

static bool uploadResultContainsCloudTaskMetadata(const std::string& result)
{
    try {
        auto j = nlohmann::json::parse(result);
        return j.contains("result") && j["result"].contains("list") && !j["result"]["list"].empty();
    } catch (...) {
    }
    return false;
}

static std::string buildUploadFileNotReadyBody(const std::string& uploadName)
{
    return nlohmann::json{
        {"code", 500},
        {"message", "Uploaded file was not confirmed in the DMgr file list."},
        {"msg", "file_not_ready"},
        {"upload_gcode_name", uploadName}
    }.dump();
}

void EasyPrintSender::handleUploadComplete(const std::string&                            ip,
                                           const std::string&                            result,
                                           const nlohmann::json&                         printData,
                                           std::function<void(std::string, std::string)> onComplete,
                                           bool                                          start_print)
{
    logMessage("EasyPrintSender::handleUploadComplete", "Upload completion callback triggered. ip=" + ip + ", body=" + result);

    bool success = false;
    try {
        auto j = nlohmann::json::parse(result);

        // 闁诲繒鍋愰崑鎾绘煕閳哄啫鏋旂紓宥咁槹濞煎寮幐搴ｎ槬闂佸搫绉堕崢褏妲愰敓鐘虫櫖婵繃绫峝e=200, message=OK (婵犮垹鐖㈤崘顏呯槗闂佸憡鍔栭悷銈囩箔婢舵劕鏋侀煫鍥ㄦ煥�?
        if (j.contains("code") && j["code"] == 200 && j.contains("message")) {
            std::string msgVal = j["message"].get<std::string>();
            if (equalsIgnoreCase(msgVal, "OK")) {
                success = true;
            }
        }

        // 婵炲瓨绮嶉崹褰掝敂椤掍焦浜ら柡鍌涘缁€鈧梺鍝勭Ф閸樠呮閿熺姵鏅慨婵囩睄de=0, msg=ok (婵犮垹鐖㈤崘顏呯槗闂佸憡鍔栭悷銈囩箔婢舵劕鏋侀煫鍥ㄦ煥�?
        else if (j.contains("code") && j["code"] == 0 && j.contains("msg")) {
            std::string msgVal = j["msg"].get<std::string>();
            if (equalsIgnoreCase(msgVal, "ok")) {
                success = true;
            }
        }
    } catch (const std::exception& e) {
        logMessage("EasyPrintSender::handleUploadComplete", std::string("Failed to parse JSON: ") + e.what() + ", raw body=" + result);
    }

    if (success) {
        if (start_print) {
            logMessage(
                "EasyPrintSender::handleUploadComplete",
                "Upload successful. Calling startPrint... device_type=" +
                    std::to_string(printData.value("device_type", -1)) +
                    ", upload_device_key=" + printData.value("upload_device_key", std::string()) +
                    ", force_cloud_for_test=" + (printData.value("force_cloud_for_test", false) ? std::string("true") : std::string("false")) +
                    ", target_device_reason=" + printData.value("target_device_reason", std::string()));
            if (uploadResultContainsCloudTaskMetadata(result)) {
                if (onComplete)
                    onComplete(ip, result);
                startPrint(ip, printData, result);
            } else {
                const bool print_dispatched = startPrintLan(ip, printData);
                if (onComplete) {
                    onComplete(
                        ip,
                        print_dispatched ? result : buildUploadFileNotReadyBody(printData.value("upload_gcode_name", std::string())));
                }
            }
        } else {
            if (onComplete) {
                onComplete(ip, result);
            }
            logMessage("EasyPrintSender::handleUploadComplete", "Upload successful. Start print skipped by workflow.");
        }
    } else {
        if (onComplete) {
            onComplete(ip, result);
        }
        logMessage("EasyPrintSender::handleUploadComplete", "Upload completed but response did not indicate success. Print not started.");
    }
}

/*

void EasyPrintSender::handleUploadComplete(const std::string&                            ip,
                                           const std::string&                            result,
                                           const nlohmann::json&                         printData,
                                           std::function<void(std::string, std::string)> onComplete,
                                           bool                                          start_print)
{
    logMessage("EasyPrintSender::handleUploadComplete", "Upload completion callback triggered. ip=" + ip + ", body=" + result);

    if (onComplete) {
        onComplete(ip, result);
    }

    bool success = false;
    try {
        auto j = nlohmann::json::parse(result);
        if (j.contains("code") && j["code"] == 200 && j.contains("message") && j["message"] == "OK") {
            success = true;
        }
    } catch (const std::exception& e) {
        logMessage("EasyPrintSender::handleUploadComplete", std::string("Failed to parse JSON: ") + e.what() + ", raw body=" + result);
    }

    if (success) {
        logMessage("EasyPrintSender::handleUploadComplete", "Upload successful. Calling startPrint...");
        startPrint(ip, printData);
    } else {
        logMessage("EasyPrintSender::handleUploadComplete", "Upload completed but response did not indicate success. Print not started.");
    }
}
*/

void EasyPrintSender::cancelUpload()
{
    std::string ip = getDeviceIp();
    if (ip.empty())
        return;

    logMessage("EasyPrintSender::cancelUpload", "Cancelling upload for ip=" + ip);
    RemotePrint::RemotePrinterManager::getInstance().cancelUpload(ip);
}

std::string EasyPrintSender::getDeviceAddress()
{
    try {
        const DM::Device& cur_dev = DM::DataCenter::Ins().get_current_device_data();
        if (cur_dev.valid&& !cur_dev.address.empty()) {
            return cur_dev.address;
        }
        logMessage("EasyPrintSender::getDeviceAddress", "闂佸搫鐗滄禍婵囩珶濮椻偓瀹曟岸骞忓畝濠傛櫗婵?Address");
        return "";
    } catch (std::exception& e) {
        logMessage("EasyPrintSender::getDeviceAddress", std::string("闂佸吋鍎抽崲鑼躲亹閸モ晜濯奸柟顖嗗本�?Address 闂佸憡鍨跺浠嬪�? ") + e.what());
        return "";
    }
}


void EasyPrintSender::sendGcode(const std::string&                              gcodePath,
                                const nlohmann::json&                           printData, 
                                std::function<void(std::string, float, double)> onProgress,
                                std::function<void(std::string, int)>           onStatus,
                                std::function<void(std::string, std::string)>   onComplete,
                                bool                                            start_print)
{
    std::string key = printData.value("upload_device_key", std::string());
    if (!key.empty()) {
        logMessage("EasyPrintSender::sendGcode", "Using explicit upload_device_key=" + key +
            ", device_type=" + std::to_string(printData.value("device_type", -1)) +
            ", force_cloud_for_test=" + (printData.value("force_cloud_for_test", false) ? std::string("true") : std::string("false")) +
            ", target_device_reason=" + printData.value("target_device_reason", std::string()));
    }

    if (key.empty()) {
        key = getDeviceIp();
    }
    if (key.empty()) {
        key = getDeviceAddress();
    }

    if (key.empty()) {
        logMessage("EasyPrintSender::sendGcode", "No device IP or address found. Upload aborted.");
        return;
    }

    nlohmann::json printDataWithSize = printData;
    std::error_code file_size_ec;
    const auto upload_file_size = std::filesystem::file_size(gcodePath, file_size_ec);
    if (!file_size_ec) {
        printDataWithSize["upload_file_size"] = static_cast<std::uint64_t>(upload_file_size);
    } else {
        logMessage("EasyPrintSender::sendGcode", "Failed to get upload file size. path=" + gcodePath + ", error=" + file_size_ec.message());
    }

    logMessage("EasyPrintSender::sendGcode", "Uploading gcode. key=" + key +
        ", upload_name=" + printDataWithSize.value("upload_gcode_name", std::string()) +
        ", upload_file_size=" + std::to_string(printDataWithSize.value("upload_file_size", static_cast<std::uint64_t>(0))) +
        ", device_type=" + std::to_string(printDataWithSize.value("device_type", -1)) +
        ", tb_id=" + printDataWithSize.value("tb_id", std::string()));

    if (auto* view = wxGetApp().mainframe->get_printer_mgr_view()) {
        view->request_close_detail_page();
    }

     RemotePrint::RemotePrinterManager::getInstance().pushUploadTasks(key, printDataWithSize["upload_gcode_name"],
                  gcodePath, onProgress, onStatus,
                  std::bind(&EasyPrintSender::handleUploadComplete, this,
                  std::placeholders::_1, std::placeholders::_2, printDataWithSize,
                      onComplete, start_print));

}

void EasyPrintSender::send3mf(const std::string& filePath,
                              const nlohmann::json& printData,
                              std::function<void(std::string, float, double)> onProgress,
                              std::function<void(std::string, int)>           onStatus,
                              std::function<void(std::string, std::string)>   onComplete)
{
    std::string ip = getDeviceIp();
    if (ip.empty())
        return;

    RemotePrint::RemotePrinterManager::getInstance().pushUploadTasks(ip, printData["upload_3mf_name"], filePath, onProgress, onStatus,
                                                                     onComplete);
}

void EasyPrintSender::startPrintCloud(const std::string&    deviceId,
                                      const std::string&    fileKey,
                                      const nlohmann::json& printData,
                                      const std::string&    uploadResult)
{
    logMessage("EasyPrintSender::startPrintCloud", "Start cloud print for id=" + deviceId + ", fileKey=" + fileKey);
    logMessage("EasyPrintSender::startPrintCloud",
        "device_type=" + std::to_string(printData.value("device_type", -1)) +
        ", tb_id=" + printData.value("tb_id", std::string()) +
        ", upload_device_key=" + printData.value("upload_device_key", std::string()) +
        ", force_cloud_for_test=" + (printData.value("force_cloud_for_test", false) ? std::string("true") : std::string("false")) +
        ", target_device_reason=" + printData.value("target_device_reason", std::string()));

    if (tryStartPrintCloudClosedLoop(deviceId, fileKey, printData, uploadResult)) {
        logMessage("EasyPrintSender::startPrintCloud", "Cloud closed-loop executor accepted the request.");
        return;
    }

    // 闂佺懓鐏氶幐绋跨暤閸愵喖鍌ㄩ柣鏂款殠�?uploadResult
    logMessage("EasyPrintSender::startPrintCloud", "uploadResult raw=" + uploadResult);

    int            statusCode = 0;
    std::string    status_msg;
    nlohmann::json top_level_json;

    top_level_json["status_code"] = statusCode;
    top_level_json["id"] = "";
    top_level_json["name"] = "";
    top_level_json["type"] = "";
    top_level_json["filekey"] = "";
    top_level_json["open_cfs"] = 0;
    top_level_json["print_calibration"] = 1;
    top_level_json["color_match_info"] = nlohmann::json::array();
    top_level_json["upload_gcode_name"] = printData.value("upload_gcode_name", std::string());

    if (printData.contains("open_cfs")) {
        if (printData["open_cfs"].is_boolean())
            top_level_json["open_cfs"] = printData["open_cfs"].get<bool>() ? 1 : 0;
        else if (printData["open_cfs"].is_number_integer())
            top_level_json["open_cfs"] = printData["open_cfs"].get<int>() == 1 ? 1 : 0;
    }

    if (printData.contains("print_calibration")) {
        if (printData["print_calibration"].is_boolean())
            top_level_json["print_calibration"] = printData["print_calibration"].get<bool>() ? 1 : 0;
        else if (printData["print_calibration"].is_number_integer())
            top_level_json["print_calibration"] = printData["print_calibration"].get<int>() == 1 ? 1 : 0;
    }

    if (printData.contains("color_match_info") && printData["color_match_info"].is_array())
        top_level_json["color_match_info"] = printData["color_match_info"];

    try {
        nlohmann::json jBody = nlohmann::json::parse(uploadResult);

        if (jBody.contains("code") && jBody["code"].is_number_integer()) {
            statusCode = jBody["code"];
        }
        if (jBody.contains("message") && jBody["message"].is_string()) {
            status_msg = jBody["message"];
        }

        top_level_json["status_code"] = statusCode;

        if (jBody.contains("result") && jBody["result"].contains("list") && jBody["result"]["list"].size() > 0) {
            auto dev = jBody["result"]["list"][0];
            if (dev.contains("id"))
                top_level_json["id"] = dev["id"];
            if (dev.contains("name"))
                top_level_json["name"] = dev["name"];
            if (dev.contains("type"))
                top_level_json["type"] = dev["type"];
            if (dev.contains("filekey"))
                top_level_json["filekey"] = dev["filekey"];
        }
    } catch (const std::exception& e) {
        logMessage("EasyPrintSender::startPrintCloud", std::string("Failed to parse uploadResult: ") + e.what());
    }

    // 闂佺懓鐏氶幐绋跨暤閸愵亝鍠嗛柨婵嗘閳ь兛绮欏畷銉︽償閿濆棛鏆?JSON
    logMessage("EasyPrintSender::startPrintCloud", "Parsed top_level_json=" + top_level_json.dump(-1, ' ', true));

    // 闂佸搫顑呯€氫即鍩€?commandJson
    nlohmann::json commandJson;
    commandJson["command"] = "notify_send_complete";
    commandJson["data"]    = RemotePrint::Utils::url_encode(top_level_json.dump(-1, ' ', true));

    wxString strJS = wxString::Format("window.handleStudioCmd('%s');", RemotePrint::Utils::url_encode(commandJson.dump(-1, ' ', true)));

    wxGetApp().CallAfter([strJS]() {
        try {
            Plater* plater = wxGetApp().plater();
            if (!plater)
                return;

//#define _DEBUG2
#ifdef _DEBUG2
            //logMessage("EasyPrintSender::startPrintCloud", "test skip print");
            return;
#endif
            plater->send_script_to_printer_dialog(strJS.ToStdString());
        } catch (...) {
            // swallow
        }
    });
}

bool EasyPrintSender::startPrint(const std::string& ip, const nlohmann::json& printData, const std::string& uploadResult)
{
    logMessage("EasyPrintSender::startPrint",
        "Dispatching startPrint. ip=" + ip +
        ", device_type=" + std::to_string(printData.value("device_type", -1)) +
        ", upload_device_key=" + printData.value("upload_device_key", std::string()) +
        ", force_cloud_for_test=" + (printData.value("force_cloud_for_test", false) ? std::string("true") : std::string("false")) +
        ", target_device_reason=" + printData.value("target_device_reason", std::string()));

    try {
        auto j = nlohmann::json::parse(uploadResult);
        if (j.contains("result") && j["result"].contains("list") && !j["result"]["list"].empty()) {
            auto        dev     = j["result"]["list"][0];
            std::string id      = dev.value("id", "");
            std::string filekey = dev.value("filekey", "");
            logMessage("EasyPrintSender::startPrint",
                "Cloud upload result detected. id=" + id +
                ", filekey=" + filekey +
                ", entering startPrintCloud");
            startPrintCloud(id, filekey, printData, uploadResult);
            return true;
        }
        logMessage("EasyPrintSender::startPrint", "Upload result does not contain cloud task metadata. Falling back to LAN print.");
    } catch (const std::exception& e) {
        logMessage("EasyPrintSender::startPrint", std::string("JSON parse error, fallback to LAN print: ") + e.what());
    }

    return startPrintLan(ip, printData);
}


bool EasyPrintSender::startPrintLan(const std::string& ip, const nlohmann::json& printData)
{   
    nlohmann::json dataJson = printData;

    // [17140] Target device name of this print job; used to jump and open the
    // device detail page after the print starts successfully.
    const std::string device_name = printData.value("printer_name", std::string());

    // 闂婎偄娲ら幊姗€濡磋箛鏇熷仒闁靛鍎辩敮鐘绘煟閵娿儱顏╅柣鈯欏啠�?
    dataJson["allPlate"]   = false;
    dataJson["printer_ip"] = ip;

    nlohmann::json commandJson;
    commandJson["command"] = "send_print_cmd";
    commandJson["data"]    = dataJson.dump(-1, ' ', true);

    const std::string jsStr = RemotePrint::Utils::url_encode(commandJson.dump(-1, ' ', true));

    PrinterMgrView* view = wxGetApp().mainframe->get_printer_mgr_view();
    logMessage("EasyPrintSender::startPrintLan", "jsStr = " + jsStr);
    logMessage("EasyPrintSender::startPrintLan", "view pointer = " + std::to_string(reinterpret_cast<uintptr_t>(view)));

    if (!view) {
        logMessage("EasyPrintSender::startPrintLan", "PrinterMgrView is null. Print command not dispatched.");
        return false;
    }

    if (!updatePrinterState(ip))
    {
        return false;
    }

    const std::string uploadName = printData.value("upload_gcode_name", std::string());
    if (uploadName.empty()) {
        logMessage("EasyPrintSender::startPrintLan", "upload_gcode_name is empty. Print command not dispatched.");
        return false;
    }

    const std::uint64_t uploadFileSize = printData.value("upload_file_size", static_cast<std::uint64_t>(0));
    if (uploadFileSize == 0) {
        logMessage("EasyPrintSender::startPrintLan", "upload_file_size is empty. Print command not dispatched.");
        return false;
    }

    bool file_ready = false;
    for (int attempt = 0; attempt < 10; ++attempt) {
        const auto attempt_started = std::chrono::steady_clock::now();
        file_ready = view->request_check_upload_file_ready(ip, uploadName, uploadFileSize, 900);
        logMessage(
            "EasyPrintSender::startPrintLan",
            "Check uploaded file ready attempt=" + std::to_string(attempt + 1) +
                ", ip=" + ip +
                ", upload_name=" + uploadName +
                ", upload_file_size=" + std::to_string(uploadFileSize) +
                ", ready=" + (file_ready ? std::string("true") : std::string("false")));
        if (file_ready)
            break;

        if (attempt + 1 < 10) {
            const auto elapsed = std::chrono::steady_clock::now() - attempt_started;
            const auto interval = std::chrono::seconds(1);
            if (elapsed < interval)
                std::this_thread::sleep_for(interval - elapsed);
        }
    }

    if (!file_ready) {
        logMessage("EasyPrintSender::startPrintLan", "Uploaded file was not found in DMgr file list. Print command not dispatched.");
        return false;
    }

//#define _DEBUG2
#ifdef _DEBUG2
        logMessage("EasyPrintSender::startPrintLan", "test skip print");
        return true;
#endif

    wxGetApp().mainframe->CallAfter([=]() {
        if (view) {
            // view->run_script(jsStr);
            view->ExecuteScriptCommand(jsStr, false);

            std::thread([ip, device_name]() {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                wxGetApp().CallAfter([ip, device_name]() {
                    if (auto* current_view = wxGetApp().mainframe->get_printer_mgr_view())
                    {
                        // [17140] Product requirement: after an AI send-print
                        // succeeds, always jump to and open the detail page of
                        // THIS print's device, with LAN and WAN behaving the
                        // same. The old "auto-open current device detail on
                        // entering the device page" logic has been removed (see
                        // PrinterMgrView::on_switch_to_device_page), so open the
                        // detail page here using this print's device address.
                        // jumpToDeviceDetail already switches to the device page,
                        // so there is no need to call switch_to_device_page again.
                        EasyPrintSender detail_sender;
                        detail_sender.jumpToDeviceDetail(ip, device_name);

                        // Reopen the video stream after the detail page is opened,
                        // reusing the LAN detail video reopen fix.
                        current_view->request_reopen_detail_video();
                    }
                });
            }).detach();
        }
    });
    return true;
}
void EasyPrintSender::jumpToDeviceDetail(const std::string& ip, const std::string& name)
{
    wxGetApp().mainframe->switch_to_device_page();

    // [17140] Removed the previous static lastIp/lastName de-duplication:
    // the product requires the detail page to open on EVERY successful AI
    // send-print. The old de-dup caused "after closing the detail page,
    // printing the same device again without switching model no longer opens
    // the detail page". That de-dup was originally a debounce for the
    // "auto-open current device detail on entering the device page" behavior,
    // which has been removed (see PrinterMgrView::on_switch_to_device_page);
    // all current callers are expected to open the detail every time.

    // Build the forward_device_detail command JSON.
    nlohmann::json commandJson;
    commandJson["command"] = "forward_device_detail";
    commandJson["ip"]      = ip;
    commandJson["name"]    = name;

    // 闂佺懓鍢查崥瀣�?JS 闁荤姴顑呴崯浼村极閵堝洠鍋撳☉娆樻畼妞ゆ垳鐒︾粙?
    std::string jsStr = "window.handleStudioCmd('" + commandJson.dump() + "');";

    PrinterMgrView* view = wxGetApp().mainframe->get_printer_mgr_view();

    if (view) {
        wxGetApp().mainframe->CallAfter([=]() {
            if (view) {
                view->run_script("console.log('forward_device_detail injected from C++');");
                view->run_script(jsStr);
            }
        });
    }
}

void EasyPrintSender::sendConsumableMapping(int plateIndex)
{

    std::string deviceIp = getDeviceIp();
    if (deviceIp.empty()) {
        logMessage("EasyPrintSender::sendConsumableMapping", "No device IP found for consumable mapping request.");
        return;
    }

    nlohmann::json commandJson = {{"command", "req_match_color_info"}, {"data", buildConsumableMappingJson(plateIndex, deviceIp)}};

    logMessage("EasyPrintSender::sendConsumableMapping", "闂佸憡鐟﹂崹鍧楀焵椤戞寧绁伴柍褜鍓氶〃鍡楊焽濡ゅ懎鍙婇柣妯垮皺濞? " + commandJson.dump());

    std::string jsCall = "window.client.sendMsg(" + commandJson.dump() + ");";
    wxGetApp().mainframe->RunScript(jsCall);
}

nlohmann::json EasyPrintSender::buildConsumableMappingJson(int plateIndex, const std::string& deviceIp)
{
    nlohmann::json     plate_extruder_colors_json = nlohmann::json::array();
    std::ostringstream match_stream;

    const DM::Device& dev = DM::DataCenter::Ins().get_current_device_data();
    if (!dev.valid)
        return {};

    int extruder_id = 1;
    for (const auto& box : dev.materialBoxes) {
        for (const auto& mat : box.materials) {
            if (mat.color.empty())
                continue;

            plate_extruder_colors_json.push_back({{"extruder_id", extruder_id},
                                                  {"extruder_color", mat.color},
                                                  {"filament_type", mat.type},
                                                  {"match_slot", std::to_string(box.box_id) + char('A' + mat.material_id)}});

            match_stream << "T" << extruder_id << (std::to_string(box.box_id) + char('A' + mat.material_id)) << "="
                         << "T" << extruder_id << (std::to_string(box.box_id) + char('A' + mat.material_id)) << " ";

            ++extruder_id;
        }
    }

    return {{"printer_ip", deviceIp},
            {"plate_index", plateIndex},
            {"plate_extruder_colors", plate_extruder_colors_json},
            {"match", match_stream.str()}};
}

/*
bool EasyPrintSender::updatePrinterState() 
{ 
    std::string ip = getDeviceIp();
    return updatePrinterState(ip);
}

bool EasyPrintSender::updatePrinterState(const std::string& ip)
{
    auto* obj_list = wxGetApp().obj_list();
    if (!obj_list) {
        logMessage("updatePrinterState", "obj_list is null");
        return false;
    }

    // 闂佺儵鏅涢悺銊ф暜閹绢喖绠柛顭戝枛瀵潡姊洪幓鎺旂闁哄棛鍠栭�?
    auto data = Slic3r::GUI::SimpleDeviceMgr::instance().get_device_list_data_simple(true);

    // 闂備緡鍓欑粔鏉戭啅婵犳艾绠ラ柍褜鍓熷鍨緞鎼搭喖鏅繝銏ｆ硾濞村嫮妲愬┑瀣闁诡垎鍐帓闂佸憡鐗曠紞濠囧储閵堝鍎?IP
    for (const auto& kv : data.datas) {
        const auto& item = kv.second;
        if (item.address == ip) {
            // 闂佸憡甯囬崐鏍蓟閸ヮ剙鍙婃い鏍ㄧ閸庡﹦绱掔仦鐐仢婵?
            bool isIdle = item.online && item.state == 0;

            // 闁哄鐗婇幐鎼佸吹椤撱垹绫嶉柕澶堝劤�?
            std::ostringstream oss;
            oss << "Device IP=" << ip << ", Name=" << item.name << ", Model=" << item.model_name << ", MAC=" << item.mac
                << ", Online=" << (item.online ? "true" : "false") << ", State=" << item.state << ", Idle=" << (isIdle ? "true" : "false");
            logMessage("PrinterState", oss.str());

            return isIdle; // 缂備礁鏈钘壩涢懞銉︿氦闁哄倹瀵х粈�?true闂佹寧绋戦懟顖炲箚娓氣偓�?false
        }
    }

    // 濠电偛澶囬崜婵囩珶濮椻偓瀹曟岸骞忓畝濠傛櫗婵?
    logMessage("PrinterState", "No device found for IP=" + ip);
    return false;

}
*/

bool EasyPrintSender::updatePrinterState()
{
    const DM::Device& cur_dev = DM::DataCenter::Ins().get_current_device_data();
    std::string       mac     = cur_dev.mac;
    return updatePrinterState(mac);
}
    bool EasyPrintSender::updatePrinterState(const std::string& ipOrMac)
{
    auto* obj_list = wxGetApp().obj_list();
    if (!obj_list) {
        logMessage("updatePrinterState", "obj_list is null");
        return false;
    }

    // 闂佸吋鍎抽崲鑼躲亹閸モ晜濯奸柟顖嗗本校闂佸憡甯楅〃澶愬Υ閸愵喗鏅柛顐ｇ箓閻﹀爼鏌涘鐓庝簻闁活亶鍓熷畷娲偄閾忓湱效闂佸憡绮屾總鏃傝姳椤栨粎鍗氭い鏍ュ€楃粈?
    auto data = Slic3r::GUI::SimpleDeviceMgr::instance().get_device_list_data_simple(true);

    for (const auto& kv : data.datas) {
        const auto& item = kv.second;

        // 闂佸憡鐗曠紞濠囧储閵堝绾ч柍銉ュ级椤愪粙鏌ㄥ☉娆戠叝缂侀硸鍙冨畷?IP闂佹寧绋戦懟顖炲矗閹€�?MAC
        bool match = (item.address == ipOrMac) || (item.mac == ipOrMac);
        if (!match)
            continue;

        // 闂佸憡甯囬崐鏍蓟閸ヮ剙鍙婃い鏍ㄧ閸庡﹦绱掔仦鐐仢婵?
        bool isIdle = item.online && item.state == 0;

        // 闁哄鐗婇幐鎼佸吹椤撱垹绫嶉柕澶堝劤缁犲爼鏌ㄥ☉妯垮閻庡灚锕㈠畷銉╊敃閵夘喖鏅繝銏ｆ硾濞层劎鎮锕€鍨?
        std::ostringstream oss;
        oss << "DeviceType=" << item.device_type << ", IP=" << item.address << ", MAC=" << item.mac << ", Name=" << item.name
            << ", Model=" << item.model_name << ", Online=" << (item.online ? "true" : "false") << ", State=" << item.state
            << ", Idle=" << (isIdle ? "true" : "false");
        logMessage("PrinterState", oss.str());

        return isIdle;
    }

    // 濠电偛澶囬崜婵囩珶濮椻偓瀹曟岸骞忓畝濠傛櫗婵?
    logMessage("PrinterState", "No device found for key=" + ipOrMac);
    return false;
}



void EasyPrintSender::openLoginPage()
{
    logMessage("EasyPrintSender::openLoginPage", "start");

    {
        nlohmann::json dataJson;
        dataJson["deviceAddEnd"] = "0";
        dataJson["guideType"]    = "new";
        nlohmann::json commandJson;
        commandJson["command"] = "get_is_first_install";
        commandJson["data"]    = dataJson;
        wxString strJS         = wxString::Format("window.handleStudioCmd('%s');", commandJson.dump());
        wxGetApp().CallAfter([strJS] { wxGetApp().run_script(strJS.ToStdString()); });
    }

    {
        nlohmann::json commandJson;
        commandJson["command"] = "close_choose_area";
        commandJson["data"]    = {};
        wxString strJS         = wxString::Format("window.handleStudioCmd('%s');", commandJson.dump());
        wxGetApp().CallAfter([strJS] { wxGetApp().run_script(strJS.ToStdString()); });
    }

   // Sleep(5000);

    if (wxGetApp().mainframe)
        wxGetApp().mainframe->select_tab(static_cast<size_t>(Slic3r::GUI::MainFrame::tpHome));

    logMessage("EasyPrintSender::openLoginPage", "end");
}


