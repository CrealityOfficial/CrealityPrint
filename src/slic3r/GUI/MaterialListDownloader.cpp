#include "MaterialListDownloader.hpp"

#include "libslic3r/MaterialListManager.hpp"
#include "slic3r/Utils/Http.hpp"

#include <boost/log/trivial.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <nlohmann/json.hpp>

namespace Slic3r {
namespace GUI {

bool MaterialListDownloader::download_official_material_list(const std::string& base_url,
                                                              std::map<std::string, std::string> extra_headers,
                                                              long connect_timeout,
                                                              long response_timeout)
{
    using nlohmann::json;

    try {
        extra_headers["__CXY_OS_LANG_"] = "0";

        const std::string material_profile_url = "/api/cxy/v2/slice/profile/official/materialList";
        Http http = Http::post(base_url + material_profile_url);
        json request_body;
        request_body["engineVersion"] = "3.0.0";
        request_body["pageSize"] = 1000;

        bool result = false;
        const boost::uuids::uuid uuid = boost::uuids::random_generator()();
        for (const auto& header : extra_headers)
            http.header(header.first, header.second);
        http.header("Content-Type", "application/json")
            .header("__CXY_REQUESTID_", to_string(uuid))
            .timeout_connect(connect_timeout)
            .timeout_max(response_timeout)
            .set_post_body(request_body.dump())
            .on_complete([&](std::string body, unsigned status) {
                if (status != 200)
                    return;
                try {
                    json response = json::parse(body);
                    json materials = response["result"]["list"];
                    if (materials.empty())
                        return;
                    result = MaterialListManager::instance().save_official_material_list(materials);
                } catch (...) {
                    result = false;
                }
            })
            .on_error([&](std::string, std::string error, unsigned status) {
                BOOST_LOG_TRIVIAL(warning) << "download_official_material_list failed, status=" << status
                                           << ", error=" << error;
            })
            .perform_sync();
        return result;
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(warning) << "download_official_material_list exception: " << e.what();
    } catch (...) {
        BOOST_LOG_TRIVIAL(warning) << "download_official_material_list unknown exception";
    }
    return false;
}

} // namespace GUI
} // namespace Slic3r
