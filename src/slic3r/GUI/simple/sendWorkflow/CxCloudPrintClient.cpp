#include "CxCloudPrintClient.hpp"

#include "../../GUI.hpp"
#include "../../GUI_App.hpp"
#include "slic3r/Utils/Http.hpp"

#include <boost/log/trivial.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <ctime>
#include <fstream>

namespace Slic3r {
namespace GUI {

namespace {

using json = nlohmann::json;

std::string to_string_body(const json& body)
{
    try {
        return body.dump();
    } catch (...) {
        return "{}";
    }
}

bool json_code_is_success(const json& body)
{
    if (!body.is_object())
        return false;

    if (body.contains("code") && body["code"].is_number_integer()) {
        const int code = body["code"].get<int>();
        return code == 0 || code == 200;
    }

    if (body.contains("message") && body["message"].is_string()) {
        const std::string message = body["message"].get<std::string>();
        if (message == "OK" || message == "ok")
            return true;
    }

    return body.contains("result");
}

std::string json_error_message(const json& body, unsigned http_status)
{
    if (body.is_object()) {
        if (body.contains("msg") && body["msg"].is_string() && !body["msg"].get<std::string>().empty())
            return body["msg"].get<std::string>();
        if (body.contains("message") && body["message"].is_string() && !body["message"].get<std::string>().empty())
            return body["message"].get<std::string>();
        if (body.contains("error") && body["error"].is_string() && !body["error"].get<std::string>().empty())
            return body["error"].get<std::string>();
        if (body.contains("code") && body["code"].is_number_integer())
            return "cloud api returned code " + std::to_string(body["code"].get<int>());
    }

    if (http_status != 0)
        return "cloud api request failed, http_status=" + std::to_string(http_status);
    return "cloud api request failed";
}

void append_cloud_client_debug_log(const std::string& tag, const std::string& content)
{
    std::ofstream log_file("D:/log.txt", std::ios::app);
    if (!log_file.is_open())
        return;

    std::time_t now = std::time(nullptr);
    char        buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    log_file << "[" << buf << "][" << tag << "] " << content << "\n";
}

} // namespace

CxCloudPrintClient::Result CxCloudPrintClient::set_print_calibration(const std::string& tb_id,
                                                                     int enable_self_test) const
{
    return post_json("/api/rest/iotrouter/rpc/twoway/" + tb_id,
                     {
                         {"method", "set"},
                         {"params", {
                             {"enableSelfTest", enable_self_test}
                         }}
                     });
}

CxCloudPrintClient::Result CxCloudPrintClient::get_gcode_detail(const std::string& gcode_id) const
{
    return post_json("/api/cxy/v2/gcodev2/detail",
                     {
                         {"id", gcode_id}
                     },
                     5,
                     8);
}

CxCloudPrintClient::Result CxCloudPrintClient::parse_gcode(const std::string& tb_id,
                                                           const std::string& download_link) const
{
    return post_json("/api/rest/iotrouter/rpc/twoway/" + tb_id,
                     {
                         {"method", "set"},
                         {"params", {
                             {"parseGCode", download_link}
                         }}
                     });
}

CxCloudPrintClient::Result CxCloudPrintClient::query_parse_gcode(const std::string& tb_id,
                                                                 const std::string& download_link) const
{
    return post_json("/api/rest/iotrouter/rpc/twoway/" + tb_id,
                     {
                         {"method", "get"},
                         {"params", {
                             {"queryParseGCode", download_link}
                         }}
                     });
}

CxCloudPrintClient::Result CxCloudPrintClient::add_single_task(const nlohmann::json& payload) const
{
    return post_json("/api/rest/print/cluster/addSingleTask", payload);
}

std::string CxCloudPrintClient::build_base_url() const
{
    return get_cloud_api_url();
}

std::vector<std::pair<std::string, std::string>> CxCloudPrintClient::build_common_headers() const
{
    std::vector<std::pair<std::string, std::string>> headers;

    try {
        const auto extra = wxGetApp().get_extra_header();
        for (const auto& item : extra)
            headers.emplace_back(item.first, item.second);
    } catch (...) {
    }

    return headers;
}

CxCloudPrintClient::Result CxCloudPrintClient::post_json(const std::string& path,
                                                         const nlohmann::json& payload,
                                                         long connect_timeout,
                                                         long max_timeout) const
{
    Result result;
    const std::string url = build_base_url() + path;

    BOOST_LOG_TRIVIAL(info)
        << "CxCloudPrintClient::post_json request"
        << " path=" << path
        << ", url=" << url
        << ", connect_timeout=" << connect_timeout
        << ", max_timeout=" << max_timeout
        << ", payload=" << to_string_body(payload);
    append_cloud_client_debug_log(
        "CxCloudPrintClient",
        "request path=" + path +
        ", url=" + url +
        ", connect_timeout=" + std::to_string(connect_timeout) +
        ", max_timeout=" + std::to_string(max_timeout) +
        ", payload=" + to_string_body(payload));

    try {
        Http::set_extra_headers(wxGetApp().get_extra_header());
        Http http = Http::post(url);
        const boost::uuids::uuid uuid = boost::uuids::random_generator()();

        http.header("Content-Type", "application/json")
            .header("__CXY_REQUESTID_", to_string(uuid))
            .timeout_connect(connect_timeout)
            .timeout_max(max_timeout)
            .set_post_body(payload.dump())
            .on_complete([&result](std::string body, unsigned status) {
                result.http_status = static_cast<int>(status);
                result.body = parse_response_body(body);
                result.ok = (status == 200) && is_success_result(result);
                if (!result.ok)
                    result.error = json_error_message(result.body, status);
            })
            .on_error([&result](std::string body, std::string error, unsigned status) {
                result.http_status = static_cast<int>(status);
                result.body = parse_response_body(body);
                result.ok = false;
                result.error = !error.empty() ? error : json_error_message(result.body, status);
            })
            .perform_sync();
    } catch (const std::exception& ex) {
        result.ok = false;
        result.http_status = 0;
        result.error = ex.what();
        result.body = json{
            {"path", path},
            {"payload", payload}
        };
    } catch (...) {
        result.ok = false;
        result.http_status = 0;
        result.error = "unknown cloud http exception";
        result.body = json{
            {"path", path},
            {"payload", payload}
        };
    }

    BOOST_LOG_TRIVIAL(info)
        << "CxCloudPrintClient request path=" << path
        << ", ok=" << result.ok
        << ", http_status=" << result.http_status
        << ", error=" << result.error
        << ", response=" << to_string_body(result.body);
    append_cloud_client_debug_log(
        "CxCloudPrintClient",
        "response path=" + path +
        ", ok=" + std::string(result.ok ? "true" : "false") +
        ", http_status=" + std::to_string(result.http_status) +
        ", error=" + result.error +
        ", response=" + to_string_body(result.body));
    return result;
}

bool CxCloudPrintClient::is_success_result(const Result& result)
{
    return json_code_is_success(result.body);
}

nlohmann::json CxCloudPrintClient::parse_response_body(const std::string& body)
{
    if (body.empty())
        return json::object();

    try {
        return json::parse(body);
    } catch (...) {
        return json{
            {"raw", body}
        };
    }
}

} // namespace GUI
} // namespace Slic3r


