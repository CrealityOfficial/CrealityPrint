#include "string.h"

namespace cura52
{
    bool SplitString(const std::string& Src, std::vector<std::string>& Vctdest, const std::string& c)
    {
        std::string::size_type pos1, pos2;
        pos2 = Src.find(c);
        if (std::string::npos == pos2)
            return false;

        pos1 = 0;
        while (std::string::npos != pos2)
        {
            Vctdest.push_back(Src.substr(pos1, pos2 - pos1));

            pos1 = pos2 + c.size();
            pos2 = Src.find(c, pos1);
        }
        if (pos1 != Src.length())
        {
            Vctdest.push_back(Src.substr(pos1));
        }
        return true;
    }

    std::string transliterate(const std::string& text)
    {
        // For now, just replace all non-ascii characters with '?'.
        // This function can be expaned if we need more complex transliteration.
        std::ostringstream stream;
        for (const char& c : text)
        {
            stream << static_cast<char>((c >= 0) ? c : '?');
        }
        return stream.str();
    }
}
