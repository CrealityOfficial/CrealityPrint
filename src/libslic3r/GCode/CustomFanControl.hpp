#ifndef slic3r_GCode_CustomFanControl_hpp_
#define slic3r_GCode_CustomFanControl_hpp_

#include "../GCodeReader.hpp"

#include <algorithm>
#include <string>
#include <string_view>

namespace Slic3r {

// Tracks the commands that a filter actually emitted, not its planned fan state.
// The fallback remains executable when the later FanMover filter is disabled.
class CustomFanControlTracker
{
public:
    static bool is_restore_command(std::string_view line)
    {
        constexpr std::string_view marker = ";_CUSTOM_FAN_RESTORE";
        const size_t pos = line.find(marker);
        return pos != std::string_view::npos &&
               (pos + marker.size() == line.size() ||
                line.find_first_of(" \t\r\n", pos + marker.size()) == pos + marker.size());
    }

    static int fan_speed_percent(const GCodeReader::GCodeLine& line, GCodeFlavor flavor)
    {
        if (flavor == gcfMakerWare || flavor == gcfSailfish) {
            if (line.cmd_is("M126"))
                return 100;
            return line.cmd_is("M127") ? 0 : -1;
        }
        if (flavor == gcfMach3 || flavor == gcfMachinekit) {
            float value;
            if (line.cmd_is("M106") && line.has_value('P', value))
                return scaled_speed(value);
            if (line.cmd_is("M107") ||
                (line.cmd_is("M106") && line.has_value('S', value) && value == 0.f))
                return 0;
            return -1;
        }
        // This slicer's part fan uses no P or P1. P2/P3 control other fans.
        if (line.has('P') && line.p() != 1.f)
            return -1;
        if (line.cmd_is("M107"))
            return 0;
        if (line.cmd_is("M106")) {
            float value = 255.f;
            line.has_value('S', value);
            return scaled_speed(value);
        }
        return -1;
    }

    static int fan_speed_percent(std::string_view raw, GCodeFlavor flavor)
    {
        raw = trim(raw);
        if (raw.empty() || raw.front() != 'M')
            return -1;
        int speed = -1;
        GCodeReader reader;
        reader.parse_line(std::string(raw), [&speed, flavor](GCodeReader&, const GCodeReader::GCodeLine& line) {
            speed = fan_speed_percent(line, flavor);
        });
        return speed;
    }

    std::string process(std::string_view gcode, GCodeFlavor flavor)
    {
        std::string output;
        output.reserve(gcode.size());
        for (size_t pos = 0; pos < gcode.size();) {
            const size_t newline = gcode.find('\n', pos);
            const size_t end = newline == std::string_view::npos ? gcode.size() : newline;
            const std::string_view raw = gcode.substr(pos, end - pos);
            const std::string_view line = trim(raw);
            std::string replacement;
            if (line == ";_CUSTOM_FAN_CONTROL_BEGIN") {
                if (m_depth++ == 0) {
                    m_entry_command = m_last_command;
                    m_awaiting_restore = false;
                }
            } else if (line == ";_CUSTOM_FAN_CONTROL_END") {
                if (m_depth != 0 && --m_depth == 0)
                    m_awaiting_restore = true;
            } else if (fan_speed_percent(line, flavor) >= 0) {
                if (m_depth == 0 && m_awaiting_restore && is_restore_command(line)) {
                    if (!m_entry_command.empty())
                        replacement = m_entry_command + " ;_CUSTOM_FAN_RESTORE";
                    m_awaiting_restore = false;
                }
                const std::string_view command = replacement.empty() ? line : std::string_view(replacement);
                m_last_command = std::string(trim(command.substr(0, command.find(';'))));
                if (m_depth == 0 && m_awaiting_restore)
                    // A new printing target after END supersedes the entry state.
                    m_entry_command = m_last_command;
            }
            if (replacement.empty()) {
                output.append(raw);
            } else {
                output += replacement;
                if (!raw.empty() && raw.back() == '\r')
                    output += '\r';
            }
            if (newline != std::string_view::npos)
                output += '\n';
            pos = newline == std::string_view::npos ? gcode.size() : newline + 1;
        }
        return output;
    }

    void reset()
    {
        m_last_command.clear();
        m_entry_command.clear();
        m_depth = 0;
        m_awaiting_restore = false;
    }

private:
    static int scaled_speed(float value)
    {
        return std::isfinite(value) ? static_cast<int>(std::clamp(value, 0.f, 255.f) * (100.f / 255.f)) : -1;
    }

    static std::string_view trim(std::string_view text)
    {
        const size_t first = text.find_first_not_of(" \t\r");
        if (first == std::string_view::npos)
            return {};
        const size_t last = text.find_last_not_of(" \t\r");
        return text.substr(first, last - first + 1);
    }

    std::string m_last_command;
    std::string m_entry_command;
    unsigned int m_depth = 0;
    bool m_awaiting_restore = false;
};

} // namespace Slic3r

#endif // slic3r_GCode_CustomFanControl_hpp_
