#pragma once

#include "libslic3r/Utils.hpp"
#include "libslic3r/PrintConfig.hpp"
#include <iomanip>
#include <boost/log/trivial.hpp>
#include <atomic>
#include <exception>
#include <sstream>

namespace Slic3r { namespace GUI {

// Temporary opt-in diagnostics: enabled by log_severity_level=debug/trace.
// Only copied addresses are retained, so END never dereferences a retired object.
class ParameterSwitchTrace
{
public:
    static bool enabled() { return Slic3r::get_logging_level() >= 4; }

    ParameterSwitchTrace(const char* operation, const void* object, unsigned minimum_level = 4) noexcept
        : m_enabled(Slic3r::get_logging_level() >= minimum_level), m_operation(operation), m_object(object)
    {
        if (!m_enabled)
            return;
        m_id = ++sequence();
        m_parent = current();
        current() = m_id;
        m_exceptions = std::uncaught_exceptions();
        note("BEGIN");
    }

    ~ParameterSwitchTrace() noexcept
    {
        if (m_enabled) {
            note(std::uncaught_exceptions() > m_exceptions ? "UNWIND" : "END");
            current() = m_parent;
        }
    }

    template<class... Args> void note(const char* phase, const Args&... args) const noexcept
    {
        if (!m_enabled)
            return;
        try {
            std::ostringstream out;
            out << "[SwitchCrashDiag:v1] id=" << m_id << " parent=" << m_parent
                << " op=" << m_operation << " object=" << m_object << " phase=" << phase;
            (out << ... << args);
            BOOST_LOG_TRIVIAL(warning) << out.str();
            Slic3r::flush_logs();
        } catch (...) {
            // Diagnostic I/O must not change the original control flow.
        }
    }

    unsigned long long id() const { return m_id; }

    ParameterSwitchTrace(const ParameterSwitchTrace&) = delete;
    ParameterSwitchTrace& operator=(const ParameterSwitchTrace&) = delete;

private:
    static std::atomic<unsigned long long>& sequence() { static std::atomic<unsigned long long> n{0}; return n; }
    static unsigned long long& current() { static thread_local unsigned long long n = 0; return n; }
    bool m_enabled;
    const char* m_operation;
    const void* m_object;
    unsigned long long m_id = 0, m_parent = 0;
    int m_exceptions = 0;
};

// Temporary diagnostics for row reset; reuse switch trace IDs and flush policy.
inline bool trace_pa_reset(const std::string& key)
{
    if (!ParameterSwitchTrace::enabled()) return false;
    const auto base = key.substr(0, key.find('#'));
    return base == "pressure_advance" || base == "enable_pressure_advance";
}

inline void trace_pa_config(const ParameterSwitchTrace& trace, const char* phase,
                            const DynamicPrintConfig& config, const std::string& key) noexcept
{
    try {
        if (!trace_pa_reset(key)) return;
        trace.note(phase, " config=", &config);
        for (const char* name : {"enable_pressure_advance", "pressure_advance",
                                 "filament_extruder_variant", "filament_nozzle_variant"}) {
            const ConfigOption* option = config.option(name);
            if (option == nullptr) {
                trace.note("OPTION", " key=", name, " missing=1");
                continue;
            }
            const auto* values = dynamic_cast<const ConfigOptionVectorBase*>(option);
            trace.note("OPTION", " key=", name, " type=", int(option->type()),
                       " size=", values ? values->size() : 0, " value=", option->serialize());
            if (const auto* floats = dynamic_cast<const ConfigOptionFloats*>(option)) {
                std::ostringstream raw;
                raw << std::setprecision(17);
                for (double value : floats->values) raw << value << ',';
                trace.note("RAW_FLOATS", " key=", name, " values=", raw.str());
            }
        }
    } catch (...) {
        trace.note("CONFIG_SNAPSHOT_FAILED");
    }
}

}} // namespace Slic3r::GUI
