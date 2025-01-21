#include "Utils.hpp"
#include <sstream>
#include <iomanip>

namespace RemotePrint {
    // std::string Utils::url_encode(const std::string &value) 
    // {
    //     std::ostringstream escaped;
    //     escaped.fill('0');
    //     escaped << std::hex;

    //     for (char c : value) {
    //         // Keep alphanumeric and other accepted characters intact
    //         if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
    //             escaped << c;
    //         } else {
    //             // Any other characters are percent-encoded
    //             escaped << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
    //         }
    //     }

    //     return escaped.str();
    // }

std::string Utils::url_encode(const std::string& value)
{
    std::ostringstream escaped;
    escaped << std::hex << std::uppercase;

    for (unsigned char c : value) {
        // 保持字母数字和其他安全字符不变
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            // 其他字符进行百分比编码
            escaped << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(c);
        }
    }

    return escaped.str();
}

    // Helper function to convert hex color string to wxColour
    wxColour Utils::hex_string_to_wxcolour(const std::string& hex)
    {
        if(hex.empty()) {
            return wxColour(0, 0, 0);
        }
        
        if (hex[0] != '#' || hex.length() != 7) {
            return wxColour(0, 0, 0);
            throw std::invalid_argument("Invalid hex color format");
        }

        unsigned int      r, g, b;
        std::stringstream ss;
        ss << std::hex << hex.substr(1, 2);
        ss >> r;
        ss.clear();
        ss << std::hex << hex.substr(3, 2);
        ss >> g;
        ss.clear();
        ss << std::hex << hex.substr(5, 2);
        ss >> b;

        return wxColour(r, g, b);
    }

    std::string Utils::wxcolour_to_hex_string(const wxColour& colour)
    {
        std::stringstream ss;
        ss << "#" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(colour.Red()) << std::setw(2) << std::setfill('0')
           << static_cast<int>(colour.Green()) << std::setw(2) << std::setfill('0') << static_cast<int>(colour.Blue());
        return ss.str();
    }

    bool Utils::should_dark(const wxColour& bgColor)
    {
        int brightness = (bgColor.Red() * 299 + bgColor.Green() * 587 + bgColor.Blue() * 114) / 1000;
        return brightness > 50;
    }
}