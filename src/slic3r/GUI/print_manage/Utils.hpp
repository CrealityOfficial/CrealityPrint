#ifndef UTILS_202410111635_H
#define UTILS_202410111635_H

#include <wx/colour.h>
#include <string>

namespace RemotePrint {
    class Utils {
    public:
        static std::string url_encode(const std::string &value);
        static wxColour hex_string_to_wxcolour(const std::string& hex);
        static std::string wxcolour_to_hex_string(const wxColour& colour);
        static bool should_dark(const wxColour& bgColor);
    };
}

#endif // UTILS_202410111635_H