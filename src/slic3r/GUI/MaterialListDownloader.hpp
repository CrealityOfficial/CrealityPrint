#ifndef slic3r_GUI_MaterialListDownloader_hpp_
#define slic3r_GUI_MaterialListDownloader_hpp_

#include <map>
#include <string>

namespace Slic3r {
namespace GUI {

class MaterialListDownloader
{
public:
    static bool download_official_material_list(const std::string& base_url,
                                                std::map<std::string, std::string> extra_headers,
                                                long connect_timeout = 2,
                                                long response_timeout = 5);
};

} // namespace GUI
} // namespace Slic3r

#endif
