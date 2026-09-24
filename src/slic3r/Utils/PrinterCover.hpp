#ifndef slic3r_Utils_PrinterCover_hpp_
#define slic3r_Utils_PrinterCover_hpp_

#include <cstdint>
#include <string>

namespace Slic3r {
// Call from a background loader. Failure never invalidates a parameter package.
bool sync_printer_cover(const std::string& vendor, const std::string& model, const std::string& url);
uint64_t printer_cover_revision();
} // namespace Slic3r
#endif
