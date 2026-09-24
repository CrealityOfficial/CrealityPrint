#ifndef slic3r_GUI_MachineListDownloader_hpp_
#define slic3r_GUI_MachineListDownloader_hpp_

#include <map>
#include <string>

namespace Slic3r {
namespace GUI {

// Downloads the official printer list for the cloud endpoint of the current login
// region and stores it as <data_dir>/system/Creality/machineList.json.
//
// The catalogue differs per region (the CN and overseas hosts publish different
// series), so this file has to be refreshed whenever the login region changes.
// Callers are expected to follow a successful download with
// ProfileFamilyLoader::reload_machine_list() so the running process picks it up.
class MachineListDownloader
{
public:
    // Blocking. Safe to call from a worker thread: all headers are attached per
    // request instead of through Http::set_extra_headers().
    // Returns true only when a non-empty list was written to disk.
    static bool download_official_machine_list(const std::string& base_url,
                                               std::map<std::string, std::string> extra_headers,
                                               long connect_timeout = 2,
                                               long response_timeout = 5);
};

} // namespace GUI
} // namespace Slic3r

#endif
