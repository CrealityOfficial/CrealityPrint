#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <functional>
#include <map>
#include <chrono>
#include <memory>

namespace Slic3r {
namespace GUI {

enum class PRINTER_STATE {
    UNKNOWN        = -1,
    IDLE           = 0,
    PRINTING       = 1,
    PRINT_COMPLETE = 2,
    PRINT_FAILED   = 3,
    PRINT_ABORT    = 4,
    PAUSED         = 5,
    PAUSING        = 6,
    STOPPING       = 7,
    RESTORING      = 8,
    STARTING       = 9
};

inline const char* printerStateToString(PRINTER_STATE state)
{
    switch (state) {
    case PRINTER_STATE::IDLE: return "Idle";
    case PRINTER_STATE::PRINTING: return "Printing";
    case PRINTER_STATE::PRINT_COMPLETE: return "Print Complete";
    case PRINTER_STATE::PRINT_FAILED: return "Print Failed";
    case PRINTER_STATE::PRINT_ABORT: return "Print Abort";
    case PRINTER_STATE::PAUSED: return "Paused";
    case PRINTER_STATE::PAUSING: return "Pausing";
    case PRINTER_STATE::STOPPING: return "Stopping";
    case PRINTER_STATE::RESTORING: return "Restoring";
    case PRINTER_STATE::STARTING: return "Starting";
    default: return "Unknown";
    }
}

struct PrinterStatus
{
    PRINTER_STATE                         state;
    std::chrono::steady_clock::time_point lastUpdate;
};

class EasyPrintSender
{
public:
    struct CloudPrintCallbacks {
        std::function<void(int, const std::string&, const std::string&)> onProgress;
        std::function<void(const nlohmann::json&)>                        onSuccess;
        std::function<void(const std::string&, const std::string&)>       onError;
    };

    EasyPrintSender();
    ~EasyPrintSender();

    std::string getDeviceIp();
    void sendConsumableMapping(int plateIndex);

    void sendGcode(const std::string& gcodePath,
                   const nlohmann::json& printData,
                   std::function<void(std::string, float, double)> onProgress = {},
                   std::function<void(std::string, int)>           onStatus   = {},
                   std::function<void(std::string, std::string)>   onComplete = {},
                   bool                                            start_print = true);

    void send3mf(const std::string& filePath,
                 const nlohmann::json& printData,
                 std::function<void(std::string, float, double)> onProgress = {},
                 std::function<void(std::string, int)>           onStatus   = {},
                 std::function<void(std::string, std::string)>   onComplete = {});

    void handleUploadComplete(const std::string& ip,
                              const std::string& result,
                              const nlohmann::json& printData,
                              std::function<void(std::string, std::string)> onComplete,
                              bool start_print);

    bool startPrint(const std::string& ip, const nlohmann::json& printData, const std::string& uploadResult);
    bool startPrintLan(const std::string& ip, const nlohmann::json& printData);
    void startPrintCloud(const std::string& deviceId,
                         const std::string& fileKey,
                         const nlohmann::json& printData,
                         const std::string& uploadResult);

    void setCloudClosedLoopEnabled(bool enabled);
    void setCloudPrintCallbacks(CloudPrintCallbacks callbacks);

    void jumpToDeviceDetail(const std::string& ip, const std::string& name);
    void cancelUpload();

    bool updatePrinterState();
    bool updatePrinterState(const std::string& ipOrMac);

    void openLoginPage();
    std::string getDeviceAddress();

private:
    void logMessage(const std::string& tag, const std::string& content);
    bool tryStartPrintCloudClosedLoop(const std::string& deviceId,
                                      const std::string& fileKey,
                                      const nlohmann::json& printData,
                                      const std::string& uploadResult);

private:
    nlohmann::json buildConsumableMappingJson(int plateIndex, const std::string& deviceIp);
    bool                                        m_cloud_closed_loop_enabled = false;
    CloudPrintCallbacks                         m_cloud_print_callbacks;
    std::shared_ptr<class CxCloudPrintExecutor> m_cloud_print_executor;
};

} // namespace GUI
} // namespace Slic3r
