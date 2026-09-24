#pragma once

#include "Utils.hpp"
#include <boost/log/trivial.hpp>
#include <atomic>
#include <exception>
#include <sstream>

namespace Slic3r {

// Temporary diagnostics, enabled only at debug/trace. Never retain object references.
class RetractConfigTrace
{
public:
    static bool enabled() { return get_logging_level() >= 4; }
    RetractConfigTrace(const char* operation, const void* object) noexcept
        : m_enabled(enabled()), m_operation(operation), m_object(object)
    {
        if (!m_enabled) return;
        m_id = ++sequence();
        m_parent = current();
        current() = m_id;
        m_exceptions = std::uncaught_exceptions();
        note("BEGIN");
    }
    ~RetractConfigTrace() noexcept
    {
        if (!m_enabled) return;
        note(std::uncaught_exceptions() > m_exceptions ? "UNWIND" : "END");
        current() = m_parent;
    }
    template<class... Args> void note(const char* phase, const Args&... args) const noexcept
    {
        if (!m_enabled) return;
        try {
            std::ostringstream out;
            out << "[RetractCrashDiag:v1] id=" << m_id << " parent=" << m_parent
                << " op=" << m_operation << " object=" << m_object << " phase=" << phase;
            (out << ... << args);
            BOOST_LOG_TRIVIAL(warning) << out.str();
            flush_logs();
        } catch (...) {}
    }
    // Only call with a config known to belong to the live writer/input object.
    template<class Config> void config(const char* phase, const Config& value) const noexcept
    {
        if (!m_enabled) return;
        note(phase, " config=", &value);
        note("ARRAYS", " restart_size=", value.retract_restart_extra.values.size(),
             " restart_data=", static_cast<const void*>(value.retract_restart_extra.values.data()),
             " length_size=", value.retraction_length.values.size(),
             " speed_size=", value.retraction_speed.values.size(),
             " toolchange_restart_size=", value.retract_restart_extra_toolchange.values.size(),
             " toolchange_length_size=", value.retract_length_toolchange.values.size());
    }
    RetractConfigTrace(const RetractConfigTrace&) = delete;
    RetractConfigTrace& operator=(const RetractConfigTrace&) = delete;
private:
    static std::atomic<unsigned long long>& sequence() { static std::atomic<unsigned long long> n{0}; return n; }
    static unsigned long long& current() { static thread_local unsigned long long n = 0; return n; }
    bool m_enabled;
    const char* m_operation;
    const void* m_object;
    unsigned long long m_id = 0, m_parent = 0;
    int m_exceptions = 0;
};

} // namespace Slic3r
