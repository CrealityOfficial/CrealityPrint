#pragma once

#include "nlohmann/json.hpp"

#include <string>
#include <utility>
#include <vector>

namespace Slic3r {
namespace GUI {

class CxCloudPrintClient
{
public:
    struct Result {
        bool           ok = false;
        int            http_status = 0;
        std::string    error;
        nlohmann::json body = nlohmann::json::object();
    };

public:
    CxCloudPrintClient() = default;
    ~CxCloudPrintClient() = default;

    Result set_print_calibration(const std::string& tb_id, int enable_self_test) const;
    Result get_gcode_detail(const std::string& gcode_id) const;
    Result parse_gcode(const std::string& tb_id, const std::string& download_link) const;
    Result query_parse_gcode(const std::string& tb_id, const std::string& download_link) const;
    Result add_single_task(const nlohmann::json& payload) const;

    std::string build_base_url() const;
    std::vector<std::pair<std::string, std::string>> build_common_headers() const;

private:
    Result post_json(const std::string& path,
                     const nlohmann::json& payload,
                     long connect_timeout = 5,
                     long max_timeout = 15) const;
    static bool is_success_result(const Result& result);
    static nlohmann::json parse_response_body(const std::string& body);
};

} // namespace GUI
} // namespace Slic3r
