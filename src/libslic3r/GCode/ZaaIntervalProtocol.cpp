#include "../I18N.hpp"
#include "ZaaIntervalProtocol.hpp"

namespace Slic3r {

namespace {

bool has_token_prefix(std::string_view line, std::string_view token)
{
    if (line.size() < token.size() || line.substr(0, token.size()) != token)
        return false;
    return line.size() == token.size() || line[token.size()] == ' ' || line[token.size()] == '\t' || line[token.size()] == '\r';
}

} // namespace

ZaaIntervalMarker ZaaIntervalTracker::consume_line(std::string_view line)
{
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
        line.remove_suffix(1);

    static constexpr std::string_view begin_marker = "; ZAA_BEGIN";
    static constexpr std::string_view end_marker   = "; ZAA_END";

    if (has_token_prefix(line, begin_marker)) {
        if (m_inside)
            throw Slic3r::LogicError(_u8L("ZAA interval contains a repeated ZAA_BEGIN marker"));
        m_inside = true;
        return ZaaIntervalMarker::Begin;
    }

    if (has_token_prefix(line, end_marker)) {
        if (!m_inside)
            throw Slic3r::LogicError(_u8L("ZAA interval contains an orphan ZAA_END marker"));
        m_inside = false;
        return ZaaIntervalMarker::End;
    }

    return ZaaIntervalMarker::None;
}

void ZaaIntervalTracker::consume_gcode(std::string_view gcode)
{
    size_t start = 0;
    while (start < gcode.size()) {
        const size_t end = gcode.find('\n', start);
        if (end == std::string_view::npos) {
            consume_line(gcode.substr(start));
            break;
        }
        consume_line(gcode.substr(start, end - start + 1));
        start = end + 1;
    }
}

void ZaaIntervalTracker::finish_layer() const
{
    if (m_inside)
        throw Slic3r::LogicError(_u8L("ZAA interval is still active at the end of a layer"));
}

} // namespace Slic3r
