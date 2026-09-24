#ifndef slic3r_Diagnostics_Report_hpp_
#define slic3r_Diagnostics_Report_hpp_

#include "Session.hpp"

#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace Slic3r::Diagnostics {

struct Sample
{
    Scope       scope;
    std::string fingerprint;
};

struct Error
{
    Scope       scope;
    std::string path;
    std::string message;
};

class Report final
{
public:
    void begin_plate(int plate_id);
    void record(const Scope &scope, std::string path, std::string fingerprint);
    void record_error(const Scope &scope, std::string path, std::string message) noexcept;
    bool save(const std::string &path, std::string &error) const;

private:
    using FingerprintSamples = std::map<std::string, std::vector<Sample>>;
    mutable std::mutex              m_mutex;
    std::map<int, FingerprintSamples> m_plates;
    std::vector<Error>                m_errors;
};

} // namespace Slic3r::Diagnostics

#endif
