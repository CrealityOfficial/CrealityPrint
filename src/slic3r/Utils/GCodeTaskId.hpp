#ifndef slic3r_GCodeTaskId_hpp_
#define slic3r_GCodeTaskId_hpp_

#include <iostream>
#include <string>

namespace Slic3r {

// Update only reserved header space: moving subsequent bytes would invalidate
// G-code offsets. Supports both the initial placeholder and repeated sends.
inline bool update_gcode_task_id(std::iostream& file, const std::string& task_id)
{
    if (task_id.empty() || task_id.find_first_of("\r\n") != std::string::npos)
        return false;

    file.clear();
    file.seekg(0);
    std::string header(4096, '\0');
    file.read(&header[0], static_cast<std::streamsize>(header.size()));
    header.resize(static_cast<size_t>(file.gcount()));

    const std::string pending_marker = "; creality_task_id_pending: ";
    const std::string marker = "; creality_task_id: ";
    size_t line_start = 0;
    while (line_start < header.size()) {
        const size_t newline = header.find('\n', line_start);
        if (newline == std::string::npos)
            return false;

        if (header.compare(line_start, pending_marker.size(), pending_marker) == 0 ||
            header.compare(line_start, marker.size(), marker) == 0) {
            const size_t line_end = newline > line_start && header[newline - 1] == '\r'
                ? newline - 1 : newline;
            const size_t line_size = line_end - line_start;
            if (line_size < marker.size() || task_id.size() > line_size - marker.size())
                return false;

            std::string replacement = marker + task_id;
            replacement.resize(line_size, ' ');
            file.clear();
            file.seekp(static_cast<std::streamoff>(line_start));
            if (!file)
                return false;
            file.write(replacement.data(), static_cast<std::streamsize>(replacement.size()));
            file.flush();
            return static_cast<bool>(file);
        }
        line_start = newline + 1;
    }
    return false;
}

} // namespace Slic3r

#endif
