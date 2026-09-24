#ifndef slic3r_GCode_ZaaIntervalProtocol_hpp_
#define slic3r_GCode_ZaaIntervalProtocol_hpp_

#include "../Exception.hpp"

#include <string_view>

namespace Slic3r {

enum class ZaaIntervalMarker : unsigned char
{
    None,
    Begin,
    End,
};

// Per-filter streaming state for the DEC-035 ZAA_BEGIN / ZAA_END protocol.
// Marker payload is deliberately opaque: filters validate transitions only.
class ZaaIntervalTracker
{
public:
    ZaaIntervalMarker consume_line(std::string_view line);
    void              consume_gcode(std::string_view gcode);
    void              finish_layer() const;
    void              reset() { m_inside = false; }

    bool inside() const { return m_inside; }

private:
    bool m_inside{ false };
};

} // namespace Slic3r

#endif /* slic3r_GCode_ZaaIntervalProtocol_hpp_ */
