#pragma once

#include "CxCloudPrintClient.hpp"

#include "nlohmann/json.hpp"

#include <atomic>
#include <functional>
#include <string>
#include <vector>

namespace Slic3r {
namespace GUI {

struct CloudPrintMaterialItem {
    int         extruder_id = 0;
    int         box_id = -1;
    int         box_type = -1;
    int         material_id = -1;
    std::string filament_type;
    std::string extruder_color;
    std::string match_color;
    std::string c_id;
    std::string slot_label;
};

struct CloudPrintRequest {
    std::string                     device_name;
    std::string                     printer_name;
    std::string                     tb_id;
    std::string                     upload_name;
    std::string                     upload_result_body;
    bool                            is_multi_color_device = false;
    int                             open_cfs = 0;
    int                             print_calibration = 1;
    std::vector<CloudPrintMaterialItem> materials;
};

struct CloudUploadedFileInfo {
    int         status_code = -1;
    std::string gcode_id;
    std::string file_key;
    std::string task_name;
    std::string cdn_gcode_file_path;
};

struct CloudGcodeDetail {
    int            parse_state = -1;
    std::string    download_link;
    nlohmann::json parsed_filaments = nlohmann::json::array();
};

class CxCloudPrintExecutor
{
public:
    struct Callbacks {
        std::function<void(int progress, const std::string& stage, const std::string& message)> on_progress;
        std::function<void(const nlohmann::json& result)> on_success;
        std::function<void(const std::string& code, const std::string& message)> on_error;
    };

public:
    explicit CxCloudPrintExecutor(CxCloudPrintClient client = {});
    ~CxCloudPrintExecutor() = default;

    void start(const CloudPrintRequest& request, Callbacks callbacks);
    void cancel();

private:
    bool load_uploaded_file_info(const CloudPrintRequest& request, CloudUploadedFileInfo& out) const;
    bool wait_cloud_gcode_ready(const CloudUploadedFileInfo& uploaded,
                                CloudGcodeDetail& out_detail,
                                const Callbacks& callbacks,
                                std::string& error_code,
                                std::string& error_message) const;
    bool ensure_cloud_parse_ready(const CloudPrintRequest& request,
                                  const CloudUploadedFileInfo& uploaded,
                                  CloudGcodeDetail& detail,
                                  const Callbacks& callbacks,
                                  std::string& error_code,
                                  std::string& error_message) const;
    nlohmann::json build_add_single_task_payload(const CloudPrintRequest& request,
                                                 const CloudUploadedFileInfo& uploaded,
                                                 const CloudGcodeDetail& detail) const;

    void emit_progress(const Callbacks& callbacks,
                       int progress,
                       const std::string& stage,
                       const std::string& message) const;
    void emit_error(const Callbacks& callbacks,
                    const std::string& code,
                    const std::string& message) const;
    void emit_success(const Callbacks& callbacks, const nlohmann::json& result) const;

private:
    CxCloudPrintClient m_client;
    std::atomic_bool   m_cancelled { false };
};

} // namespace GUI
} // namespace Slic3r
