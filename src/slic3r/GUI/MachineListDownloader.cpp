#include "MachineListDownloader.hpp"

#include "libslic3r/Utils.hpp"
#include "slic3r/Utils/Http.hpp"

#include <iomanip>

#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <nlohmann/json.hpp>

namespace Slic3r {
namespace GUI {

namespace {

// Writes to a unique temporary file and renames it over the target so a reader (or a
// second app instance reacting to the same region change) never observes a partially
// written list. boost::filesystem::rename replaces an existing destination.
bool write_machine_list_atomically(const nlohmann::json& printer_list, const boost::filesystem::path& target)
{
    boost::system::error_code ec;
    boost::filesystem::create_directories(target.parent_path(), ec);
    if (ec) {
        BOOST_LOG_TRIVIAL(warning) << "download_official_machine_list: cannot create "
                                   << target.parent_path().string() << ", reason=" << ec.message();
        return false;
    }

    const boost::uuids::uuid    uuid     = boost::uuids::random_generator()();
    boost::filesystem::path     tmp_path = target;
    tmp_path += "." + to_string(uuid) + ".tmp";

    {
        boost::nowide::ofstream out;
        out.open(tmp_path.string(), std::ios::out | std::ios::trunc);
        if (!out.is_open()) {
            BOOST_LOG_TRIVIAL(warning) << "download_official_machine_list: cannot open " << tmp_path.string();
            return false;
        }
        out << std::setw(4) << printer_list << std::endl;
        out.close();
        if (out.fail()) {
            BOOST_LOG_TRIVIAL(warning) << "download_official_machine_list: write failed for " << tmp_path.string();
            boost::filesystem::remove(tmp_path, ec);
            return false;
        }
    }

    boost::filesystem::rename(tmp_path, target, ec);
    if (ec) {
        BOOST_LOG_TRIVIAL(warning) << "download_official_machine_list: cannot move " << tmp_path.string() << " onto "
                                   << target.string() << ", reason=" << ec.message();
        boost::system::error_code remove_ec;
        boost::filesystem::remove(tmp_path, remove_ec);
        return false;
    }
    return true;
}

} // namespace

bool MachineListDownloader::download_official_machine_list(const std::string& base_url,
                                                           std::map<std::string, std::string> extra_headers,
                                                           long connect_timeout,
                                                           long response_timeout)
{
    using nlohmann::json;

    try {
        // Matches the existing behaviour of GUI_App::check_machine_list(): the printer
        // catalogue is requested in English so model names stay stable across UI locales.
        extra_headers["__CXY_OS_LANG_"] = "0";

        const std::string printer_list_url = "/api/cxy/v2/slice/profile/official/printerList";
        Http              http             = Http::post(base_url + printer_list_url);

        json request_body;
        request_body["engineVersion"] = "3.0.0";

        const boost::filesystem::path target = (boost::filesystem::path(data_dir()) / "system" / "Creality" /
                                                "machineList.json")
                                                   .make_preferred();

        bool                     result = false;
        const boost::uuids::uuid uuid   = boost::uuids::random_generator()();
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
                    json response     = json::parse(body);
                    json printer_list = response["result"];
                    if (!printer_list.contains("printerList") || printer_list["printerList"].empty()) {
                        BOOST_LOG_TRIVIAL(warning) << "download_official_machine_list: empty printerList, "
                                                      "keeping the file already on disk";
                        return;
                    }
                    result = write_machine_list_atomically(printer_list, target);
                } catch (...) {
                    result = false;
                }
            })
            .on_error([&](std::string, std::string error, unsigned status) {
                BOOST_LOG_TRIVIAL(warning) << "download_official_machine_list failed, status=" << status
                                           << ", error=" << error;
            })
            .perform_sync();
        return result;
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(warning) << "download_official_machine_list exception: " << e.what();
    } catch (...) {
        BOOST_LOG_TRIVIAL(warning) << "download_official_machine_list unknown exception";
    }
    return false;
}

} // namespace GUI
} // namespace Slic3r
