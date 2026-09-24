#ifndef slic3r_GUI_PrinterCoverRoute_hpp_
#define slic3r_GUI_PrinterCoverRoute_hpp_

#include "libslic3r/PrinterCover.hpp"

namespace Slic3r { namespace GUI {

// Hex-encoded UTF-8 components survive both the frontend's encodeURI and the
// HTTP server's URL decoding, including names containing spaces, #, % or &.
inline std::string printer_cover_hex(const std::string& value)
{
    constexpr char digits[] = "0123456789abcdef";
    std::string encoded;
    for (unsigned char c : value) {
        encoded += digits[c >> 4];
        encoded += digits[c & 15];
    }
    return encoded;
}

inline std::string printer_cover_unhex(const std::string& value)
{
    if (value.empty() || value.size() % 2 != 0)
        return {};
    const auto digit = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
    };
    std::string decoded;
    for (size_t i = 0; i < value.size(); i += 2) {
        const int high = digit(value[i]), low = digit(value[i + 1]);
        if (high < 0 || low < 0) return {};
        decoded += static_cast<char>((high << 4) | low);
    }
    return decoded;
}

inline std::string printer_cover_url(int port, const std::string& vendor, const std::string& model)
{
    return "http://localhost:" + std::to_string(port) + "/printer-cover/" +
           printer_cover_hex(vendor) + "/" + printer_cover_hex(model);
}

// Resolve only model covers, never a client-provided filesystem path.
// Empty means an invalid route; a valid missing cover resolves to the default.
inline std::string printer_cover_route_path(const std::string& request_path,
                                           const std::string& data_root, const std::string& resource_root)
{
    const std::string prefix = "/printer-cover/";
    if (request_path.compare(0, prefix.size(), prefix) != 0)
        return {};
    const size_t slash = request_path.find('/', prefix.size());
    if (slash == std::string::npos)
        return {};
    const std::string vendor = printer_cover_unhex(request_path.substr(prefix.size(), slash - prefix.size()));
    const std::string model = printer_cover_unhex(request_path.substr(slash + 1));
    if (!valid_printer_cover_component(vendor) || !valid_printer_cover_component(model))
        return {};
    std::string cover = find_printer_cover(data_root, resource_root, vendor, model);
    if (cover.empty())
        cover = (std::filesystem::u8path(resource_root) / "images" / "printer_default.png").u8string();
    return cover;
}

}} // namespace Slic3r::GUI
#endif
