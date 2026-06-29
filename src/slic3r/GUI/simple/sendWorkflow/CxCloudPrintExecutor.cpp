#include "CxCloudPrintExecutor.hpp"

#include "../../I18N.hpp"

#include <boost/log/trivial.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <fstream>
#include <thread>
#include <utility>

namespace Slic3r {
namespace GUI {

namespace {

using json = nlohmann::json;

constexpr int kWaitReadyMaxRetry = 60;
constexpr int kParseMaxRetry = 60;
constexpr auto kPollInterval = std::chrono::seconds(5);

std::string dump_json_for_log(const json& value)
{
    try {
        return value.dump();
    } catch (...) {
        return "<json_dump_failed>";
    }
}


std::string build_material_label(const CloudPrintMaterialItem& item)
{
    if (!item.slot_label.empty())
        return item.slot_label;
    if (!item.filament_type.empty())
        return item.filament_type;
    return "material";
}

std::string build_filament_id(int extruder_id)
{
    if (extruder_id <= 0)
        return {};

    const int group = (extruder_id - 1) / 4 + 1;
    const char slot = static_cast<char>('A' + ((extruder_id - 1) % 4));
    return "T" + std::to_string(group) + std::string(1, slot);
}

std::string normalize_filaments_color(const std::string& color)
{
    if (color.empty())
        return {};

    std::string normalized = color;
    if (!normalized.empty() && normalized.front() != '#')
        normalized.insert(normalized.begin(), '#');

    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });

    if (normalized.size() == 7)
        return "#FF" + normalized.substr(1);
    return normalized;
}

std::string normalize_cloud_cid(std::string cid)
{
    cid.erase(std::remove_if(cid.begin(), cid.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }), cid.end());

    std::transform(cid.begin(), cid.end(), cid.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });

    return cid;
}

std::string build_cloud_cid_from_slot_label(const std::string& slot_label)
{
    const std::string normalized = normalize_cloud_cid(slot_label);
    if (normalized.size() < 3)
        return {};

    size_t digit_begin = 0;
    if (normalized.front() == 'T')
        digit_begin = 1;

    size_t digit_end = digit_begin;
    while (digit_end < normalized.size() && std::isdigit(static_cast<unsigned char>(normalized[digit_end])) != 0)
        ++digit_end;

    if (digit_end == digit_begin || digit_end != normalized.size() - 1)
        return {};

    if (std::isalpha(static_cast<unsigned char>(normalized.back())) == 0)
        return {};

    return normalized.front() == 'T' ? normalized : ("T" + normalized);
}

std::string find_parsed_filament_cid(const CloudGcodeDetail& detail, const std::string& filament_id)
{
    if (!detail.parsed_filaments.is_array() || filament_id.empty())
        return {};

    for (const auto& item : detail.parsed_filaments) {
        if (!item.is_object())
            continue;

        const std::string parsed_filament_id = normalize_cloud_cid(item.value("filamentId", std::string()));
        if (!parsed_filament_id.empty() && parsed_filament_id != filament_id)
            continue;

        const std::string parsed_cid = normalize_cloud_cid(item.value("cId", std::string()));
        if (!parsed_cid.empty())
            return parsed_cid;
    }

    return {};
}

std::string resolve_material_cid_for_task(const CloudPrintMaterialItem& material,
                                          const CloudGcodeDetail& detail)
{
    const std::string existing_cid = normalize_cloud_cid(material.c_id);
    if (!existing_cid.empty())
        return existing_cid;

    const std::string filament_id = normalize_cloud_cid(build_filament_id(material.extruder_id));
    const std::string parsed_cid = find_parsed_filament_cid(detail, filament_id);
    if (!parsed_cid.empty())
        return parsed_cid;

    return build_cloud_cid_from_slot_label(material.slot_label);
}
bool is_cancelled_waiting(const std::atomic_bool& cancelled)
{
    const auto step = std::chrono::milliseconds(100);
    for (auto waited = std::chrono::milliseconds(0); waited < kPollInterval; waited += step) {
        if (cancelled.load())
            return true;
        std::this_thread::sleep_for(step);
    }
    return cancelled.load();
}

bool body_code_is_auth_failure(const json& body)
{
    return body.is_object() && body.contains("code") && body["code"].is_number_integer() && body["code"].get<int>() == 4;
}

void append_cloud_executor_debug_log(const std::string& tag, const std::string& content)
{
    std::ofstream log_file("D:/log.txt", std::ios::app);
    if (!log_file.is_open())
        return;

    std::time_t now = std::time(nullptr);
    char        buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    log_file << "[" << buf << "][" << tag << "] " << content << "\n";
}

void log_cloud_executor_stage(const std::string& stage, const std::string& message)
{
    const std::string full_message = "[CxCloudPrintExecutor] stage=" + stage + " " + message;
    BOOST_LOG_TRIVIAL(info) << full_message;
    append_cloud_executor_debug_log("CxCloudPrintExecutor", full_message);
}
} // namespace

CxCloudPrintExecutor::CxCloudPrintExecutor(CxCloudPrintClient client)
    : m_client(std::move(client))
{
}

void CxCloudPrintExecutor::start(const CloudPrintRequest& request, Callbacks callbacks)
{
    m_cancelled.store(false);

    log_cloud_executor_stage(
        "start",
        "device_name=" + request.device_name +
        ", printer_name=" + request.printer_name +
        ", tb_id=" + request.tb_id +
        ", upload_name=" + request.upload_name +
        ", is_multi_color_device=" + std::string(request.is_multi_color_device ? "true" : "false") +
        ", open_cfs=" + std::to_string(request.open_cfs) +
        ", print_calibration=" + std::to_string(request.print_calibration) +
        ", material_count=" + std::to_string(request.materials.size()));

    emit_progress(callbacks, 0, "cloud_prepare", _u8L("Preparing cloud print request"));

    if (request.upload_result_body.empty()) {
        emit_error(callbacks, "missing_upload_result", _u8L("Cloud upload result is empty"));
        return;
    }

    if (request.device_name.empty()) {
        emit_error(callbacks, "missing_device_name", _u8L("Cloud device identifier is empty"));
        return;
    }

    if (request.tb_id.empty()) {
        emit_error(callbacks, "missing_tb_id", _u8L("Cloud device tbId is empty"));
        return;
    }

    CloudUploadedFileInfo uploaded;
    if (!load_uploaded_file_info(request, uploaded)) {
        BOOST_LOG_TRIVIAL(error)
            << "CxCloudPrintExecutor::start load_uploaded_file_info_failed upload_result="
            << request.upload_result_body;
        emit_error(callbacks, "invalid_upload_result", _u8L("Failed to parse uploaded cloud file info"));
        return;
    }

    BOOST_LOG_TRIVIAL(info)
        << "CxCloudPrintExecutor::start uploaded_file_info"
        << " status_code=" << uploaded.status_code
        << ", gcode_id=" << uploaded.gcode_id
        << ", file_key=" << uploaded.file_key
        << ", task_name=" << uploaded.task_name
        << ", cdn_gcode_file_path=" << uploaded.cdn_gcode_file_path;

    CloudGcodeDetail detail;
    std::string error_code;
    std::string error_message;

    if (!wait_cloud_gcode_ready(uploaded, detail, callbacks, error_code, error_message)) {
        emit_error(callbacks,
                   error_code.empty() ? "cloud_wait_gcode_failed" : error_code,
                   error_message.empty() ? _u8L("Waiting for cloud G-code preparation failed.") : error_message);
        return;
    }

    if (!ensure_cloud_parse_ready(request, uploaded, detail, callbacks, error_code, error_message)) {
        emit_error(callbacks,
                   error_code.empty() ? "cloud_parse_failed" : error_code,
                   error_message.empty() ? _u8L("Preparing device-side parse failed.") : error_message);
        return;
    }

    const auto payload = build_add_single_task_payload(request, uploaded, detail);
    emit_progress(callbacks, 92, "cloud_task_create", _u8L("Creating cloud print task"));

    const auto add_task_result = m_client.add_single_task(payload);
    BOOST_LOG_TRIVIAL(info)
        << "CxCloudPrintExecutor::start add_single_task_result"
        << " ok=" << add_task_result.ok
        << ", http_status=" << add_task_result.http_status
        << ", error=" << add_task_result.error
        << ", response=" << add_task_result.body.dump();
    if (!add_task_result.ok) {
        emit_error(callbacks,
                   "cloud_add_single_task_failed",
                   add_task_result.error.empty() ? _u8L("Creating cloud print task failed.") : add_task_result.error);
        return;
    }

    emit_success(callbacks, {
        {"stage", "cloud_task_created"},
        {"message", _u8L("Cloud print task created.")},
        {"payload", payload},
        {"response", add_task_result.body},
        {"gcode_id", uploaded.gcode_id},
        {"file_key", uploaded.file_key},
        {"gcode_file_path", uploaded.cdn_gcode_file_path},
        {"download_link", detail.download_link}
    });
}

void CxCloudPrintExecutor::cancel()
{
    m_cancelled.store(true);
    log_cloud_executor_stage("cancel", "requested");
}

bool CxCloudPrintExecutor::load_uploaded_file_info(const CloudPrintRequest& request, CloudUploadedFileInfo& out) const
{
    try {
        const auto body = json::parse(request.upload_result_body);
        out.status_code = body.value("code", -1);

        const auto& list = body.at("result").at("list");
        if (!list.is_array() || list.empty())
            return false;

        const auto& first = list.front();
        out.gcode_id = first.value("id", std::string());
        out.file_key = first.value("filekey", std::string());
        out.task_name = first.value("name", request.upload_name);
        if (!out.file_key.empty())
            out.cdn_gcode_file_path = "https://file-cdn.creality.com/" + out.file_key;
        BOOST_LOG_TRIVIAL(info)
            << "CxCloudPrintExecutor::load_uploaded_file_info parsed"
            << " status_code=" << out.status_code
            << ", gcode_id=" << out.gcode_id
            << ", file_key=" << out.file_key
            << ", task_name=" << out.task_name
            << ", cdn_gcode_file_path=" << out.cdn_gcode_file_path;
        return !out.gcode_id.empty();
    } catch (const std::exception& ex) {
        BOOST_LOG_TRIVIAL(error)
            << "CxCloudPrintExecutor::load_uploaded_file_info exception=" << ex.what();
        return false;
    } catch (...) {
        BOOST_LOG_TRIVIAL(error)
            << "CxCloudPrintExecutor::load_uploaded_file_info unknown exception";
        return false;
    }
}

bool CxCloudPrintExecutor::wait_cloud_gcode_ready(const CloudUploadedFileInfo& uploaded,
                                                  CloudGcodeDetail& out_detail,
                                                  const Callbacks& callbacks,
                                                  std::string& error_code,
                                                  std::string& error_message) const
{
    CxCloudPrintClient::Result last_result;

    for (int attempt = 0; attempt < kWaitReadyMaxRetry; ++attempt) {
        if (m_cancelled.load()) {
            error_code = "cancelled";
            error_message = _u8L("Cloud print workflow cancelled");
            return false;
        }

        emit_progress(callbacks,
                      12 + std::min(attempt, 20),
                      "cloud_wait_gcode",
                      _u8L("Waiting for cloud G-code to become ready"));

        last_result = m_client.get_gcode_detail(uploaded.gcode_id);
        BOOST_LOG_TRIVIAL(info)
            << "CxCloudPrintExecutor::wait_cloud_gcode_ready attempt=" << (attempt + 1)
            << "/" << kWaitReadyMaxRetry
            << ", ok=" << last_result.ok
            << ", http_status=" << last_result.http_status
            << ", error=" << last_result.error
            << ", body=" << last_result.body.dump();
        append_cloud_executor_debug_log(
            "CxCloudPrintExecutor",
            "wait_cloud_gcode_ready attempt=" + std::to_string(attempt + 1) +
            "/" + std::to_string(kWaitReadyMaxRetry) +
            ", ok=" + std::string(last_result.ok ? "true" : "false") +
            ", http_status=" + std::to_string(last_result.http_status) +
            ", error=" + last_result.error +
            ", body=" + dump_json_for_log(last_result.body));
        if (last_result.ok && last_result.body.is_object()) {
            const auto result = last_result.body.value("result", json::object());
            const int parse_state = result.value("parseState", -1);
            const std::string download_link = result.value("downloadLink", std::string());

            BOOST_LOG_TRIVIAL(info)
                << "CxCloudPrintExecutor::wait_cloud_gcode_ready state"
                << " parse_state=" << parse_state
                << ", has_download_link=" << (!download_link.empty() ? "true" : "false");
            append_cloud_executor_debug_log(
                "CxCloudPrintExecutor",
                "wait_cloud_gcode_ready state attempt=" + std::to_string(attempt + 1) +
                ", parse_state=" + std::to_string(parse_state) +
                ", download_link=" + download_link);
            if(parse_state == 4 && attempt>2) {
                error_code = "cloud_parse_failed";
                error_message = _u8L("Cloud G-code parse failed.");
                return false;
            }
            if (parse_state == 3 && !download_link.empty()) {
                out_detail.parse_state = parse_state;
                out_detail.download_link = download_link;
                return true;
            }
        } else if (body_code_is_auth_failure(last_result.body)) {
            error_code = "cloud_auth_failed";
            error_message = last_result.error.empty() ? _u8L("Cloud authentication failed.") : last_result.error;
            return false;
        }

        if (attempt + 1 < kWaitReadyMaxRetry && is_cancelled_waiting(m_cancelled)) {
            error_code = "cancelled";
            error_message = _u8L("Cloud print workflow cancelled");
            return false;
        }
    }

    error_code = "cloud_gcode_ready_timeout";
    error_message = last_result.error.empty()
        ? _u8L("Timed out waiting for the cloud G-code file to become ready.")
        : last_result.error;
    append_cloud_executor_debug_log(
        "CxCloudPrintExecutor",
        "wait_cloud_gcode_ready timeout"
        ", error_code=" + error_code +
        ", error_message=" + error_message +
        ", last_body=" + dump_json_for_log(last_result.body));
    return false;
}

bool CxCloudPrintExecutor::ensure_cloud_parse_ready(const CloudPrintRequest& request,
                                                    const CloudUploadedFileInfo& uploaded,
                                                    CloudGcodeDetail& detail,
                                                    const Callbacks& callbacks,
                                                    std::string& error_code,
                                                    std::string& error_message) const
{
    (void) uploaded;

    emit_progress(callbacks, 40, "cloud_set_calibration", _u8L("Syncing print calibration setting"));
    const auto calibration_result = m_client.set_print_calibration(request.tb_id, request.print_calibration);
    BOOST_LOG_TRIVIAL(info)
        << "CxCloudPrintExecutor::ensure_cloud_parse_ready calibration_result"
        << " ok=" << calibration_result.ok
        << ", http_status=" << calibration_result.http_status
        << ", error=" << calibration_result.error
        << ", body=" << calibration_result.body.dump();
    append_cloud_executor_debug_log(
        "CxCloudPrintExecutor",
        "set_print_calibration"
        ", ok=" + std::string(calibration_result.ok ? "true" : "false") +
        ", http_status=" + std::to_string(calibration_result.http_status) +
        ", error=" + calibration_result.error +
        ", body=" + dump_json_for_log(calibration_result.body));
    if (!calibration_result.ok) {
        error_code = "cloud_set_print_calibration_failed";
        error_message = calibration_result.error.empty()
            ? _u8L("Failed to sync print calibration setting.")
            : calibration_result.error;
        return false;
    }

    if (!request.is_multi_color_device)
        return true;

    if (detail.download_link.empty()) {
        error_code = "cloud_missing_download_link";
        error_message = _u8L("Cloud G-code download link is empty.");
        return false;
    }

    emit_progress(callbacks, 55, "cloud_parse_start", _u8L("Starting device-side G-code parse"));
    const auto parse_result = m_client.parse_gcode(request.tb_id, detail.download_link);
    BOOST_LOG_TRIVIAL(info)
        << "CxCloudPrintExecutor::ensure_cloud_parse_ready parse_start_result"
        << " ok=" << parse_result.ok
        << ", http_status=" << parse_result.http_status
        << ", error=" << parse_result.error
        << ", body=" << parse_result.body.dump();
    append_cloud_executor_debug_log(
        "CxCloudPrintExecutor",
        "parse_gcode_start"
        ", download_link=" + detail.download_link +
        ", ok=" + std::string(parse_result.ok ? "true" : "false") +
        ", http_status=" + std::to_string(parse_result.http_status) +
        ", error=" + parse_result.error +
        ", body=" + dump_json_for_log(parse_result.body));
    if (!parse_result.ok) {
        error_code = "cloud_parse_start_failed";
        error_message = parse_result.error.empty()
            ? _u8L("Starting device-side G-code parse failed.")
            : parse_result.error;
        return false;
    }

    CxCloudPrintClient::Result last_query_result;
    for (int attempt = 0; attempt < kParseMaxRetry; ++attempt) {
        if (m_cancelled.load()) {
            error_code = "cancelled";
            error_message = _u8L("Cloud print workflow cancelled");
            return false;
        }

        emit_progress(callbacks,
                      60 + std::min(attempt, 25),
                      "cloud_parse_wait",
                      _u8L("Waiting for device-side G-code parse"));

        last_query_result = m_client.query_parse_gcode(request.tb_id, detail.download_link);
        BOOST_LOG_TRIVIAL(info)
            << "CxCloudPrintExecutor::ensure_cloud_parse_ready query_parse_gcode attempt=" << (attempt + 1)
            << "/" << kParseMaxRetry
            << ", ok=" << last_query_result.ok
            << ", http_status=" << last_query_result.http_status
            << ", error=" << last_query_result.error
            << ", body=" << last_query_result.body.dump();
        append_cloud_executor_debug_log(
            "CxCloudPrintExecutor",
            "query_parse_gcode attempt=" + std::to_string(attempt + 1) +
            "/" + std::to_string(kParseMaxRetry) +
            ", ok=" + std::string(last_query_result.ok ? "true" : "false") +
            ", http_status=" + std::to_string(last_query_result.http_status) +
            ", error=" + last_query_result.error +
            ", body=" + dump_json_for_log(last_query_result.body));
        if (last_query_result.ok && last_query_result.body.is_object()) {
            const auto result = last_query_result.body.value("result", json::object());
            const int state = result.value("state", -1);

            BOOST_LOG_TRIVIAL(info)
                << "CxCloudPrintExecutor::ensure_cloud_parse_ready parse_state=" << state;
            append_cloud_executor_debug_log(
                "CxCloudPrintExecutor",
                "query_parse_gcode state attempt=" + std::to_string(attempt + 1) +
                ", state=" + std::to_string(state) +
                ", result=" + dump_json_for_log(result));
            if (state == 2) {
                detail.parse_state = state;
                detail.parsed_filaments = result.value("filamentsList", json::array());
                append_cloud_executor_debug_log(
                    "CxCloudPrintExecutor",
                    "query_parse_gcode filamentsList=" + dump_json_for_log(detail.parsed_filaments));
                return true;
            }
            if (state == 3) {
                error_code = "cloud_parse_failed";
                error_message = _u8L("Device-side G-code parse failed.");
                return false;
            }
        } else if (body_code_is_auth_failure(last_query_result.body)) {
            error_code = "cloud_auth_failed";
            error_message = last_query_result.error.empty() ? _u8L("Cloud authentication failed.") : last_query_result.error;
            return false;
        }

        if (attempt + 1 < kParseMaxRetry && is_cancelled_waiting(m_cancelled)) {
            error_code = "cancelled";
            error_message = _u8L("Cloud print workflow cancelled");
            return false;
        }
    }

    error_code = "cloud_parse_timeout";
    error_message = last_query_result.error.empty()
        ? _u8L("Timed out waiting for device-side G-code parse.")
        : last_query_result.error;
    append_cloud_executor_debug_log(
        "CxCloudPrintExecutor",
        "query_parse_gcode timeout"
        ", error_code=" + error_code +
        ", error_message=" + error_message +
        ", last_body=" + dump_json_for_log(last_query_result.body));
    return false;
}

nlohmann::json CxCloudPrintExecutor::build_add_single_task_payload(const CloudPrintRequest& request,
                                                                   const CloudUploadedFileInfo& uploaded,
                                                                   const CloudGcodeDetail& detail) const
{
    json payload = {
        {"deviceName", request.device_name},
        {"gcodeId", uploaded.gcode_id},
        {"taskName", uploaded.task_name.empty() ? request.upload_name : uploaded.task_name},
        {"gcodeFilePath", uploaded.cdn_gcode_file_path.empty() ? detail.download_link : uploaded.cdn_gcode_file_path},
        {"printCount", 1},
        {"modelName", uploaded.task_name.empty() ? request.upload_name : uploaded.task_name}
    };

    if (request.is_multi_color_device) {
        payload["enableCfs"] = request.open_cfs;
        if (request.open_cfs == 1) {
            json filaments = json::array();
            for (const auto& material : request.materials) {
                const std::string filament_id = build_filament_id(material.extruder_id);
                const std::string slot_label = build_material_label(material);
                const std::string resolved_cid = resolve_material_cid_for_task(material, detail);

                append_cloud_executor_debug_log(
                    "CxCloudPrintExecutor",
                    "resolve_material_cid extruder_id=" + std::to_string(material.extruder_id) +
                    ", filament_id=" + filament_id +
                    ", slot_label=" + slot_label +
                    ", raw_cid=" + material.c_id +
                    ", resolved_cid=" + resolved_cid);

                filaments.push_back({
                    {"cId", resolved_cid},
                    {"filamentId", filament_id},
                    {"filamentType", material.filament_type},
                    {"filamentsColor", normalize_filaments_color(material.extruder_color)},
                    {"slotLabel", slot_label}
                });
            }
            payload["filamentsList"] = filaments;
        }
    }

    BOOST_LOG_TRIVIAL(info) << "CxCloudPrintExecutor addSingleTask payload=" << payload.dump();
    append_cloud_executor_debug_log(
        "CxCloudPrintExecutor",
        "addSingleTask payload=" + dump_json_for_log(payload));
    return payload;
}

void CxCloudPrintExecutor::emit_progress(const Callbacks& callbacks,
                                         int progress,
                                         const std::string& stage,
                                         const std::string& message) const
{
    log_cloud_executor_stage(
        "progress",
        "progress=" + std::to_string(progress) +
        ", stage=" + stage +
        ", message=" + message);
    if (callbacks.on_progress)
        callbacks.on_progress(progress, stage, message);
}

void CxCloudPrintExecutor::emit_error(const Callbacks& callbacks,
                                      const std::string& code,
                                      const std::string& message) const
{
    log_cloud_executor_stage(
        "error",
        "code=" + code + ", message=" + message);
    if (callbacks.on_error)
        callbacks.on_error(code, message);
}

void CxCloudPrintExecutor::emit_success(const Callbacks& callbacks, const nlohmann::json& result) const
{
    log_cloud_executor_stage(
        "success",
        "result=" + result.dump());
    if (callbacks.on_success)
        callbacks.on_success(result);
}

} // namespace GUI
} // namespace Slic3r
