#ifndef slic3r_PrinterCover_hpp_
#define slic3r_PrinterCover_hpp_

#include <filesystem>
#include <string>

namespace Slic3r {

inline bool valid_printer_cover_component(const std::string& name)
{
    return !name.empty() && name != "." && name != ".." &&
           name.find_first_of("/\\:") == std::string::npos && name.find('\0') == std::string::npos;
}

// An empty result lets callers fall back to another vendor or a web thumbnail.
inline std::string find_printer_cover(const std::string& data_root, const std::string& resource_root,
                                      const std::string& vendor, const std::string& model)
{
    if (!valid_printer_cover_component(vendor) || !valid_printer_cover_component(model))
        return {};
    namespace fs = std::filesystem;
    const auto filename = fs::u8path(model + "_cover.png");
    for (const auto& root : {fs::u8path(data_root) / "system", fs::u8path(resource_root) / "profiles"}) {
        const auto path = root / fs::u8path(vendor) / filename;
        std::error_code ec;
        if (fs::is_regular_file(path, ec) && fs::file_size(path, ec) > 0 && !ec)
            return path.u8string();
    }
    return {};
}

} // namespace Slic3r
#endif
