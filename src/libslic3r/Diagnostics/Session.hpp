#ifndef slic3r_Diagnostics_Session_hpp_
#define slic3r_Diagnostics_Session_hpp_

#include <cstddef>
#include <limits>
#include <vector>

namespace Slic3r {

class Print;
class PrintObject;

namespace Diagnostics {

inline constexpr std::size_t no_object = std::numeric_limits<std::size_t>::max();

struct Scope
{
    int         plate_id {0};
    std::size_t object_id {no_object};
    std::size_t invocation_id {0};
};

class Report;

namespace Detail {

struct Target
{
    Report *fingerprint_report {nullptr};
    bool    performance {false};
    bool    bound {false};
    Scope   scope;

    bool fingerprint_enabled() const noexcept
    {
        return fingerprint_report != nullptr;
    }

    bool performance_enabled() const noexcept
    {
        return performance;
    }

    bool is_bound() const noexcept
    {
        return bound;
    }
};

Target find_target(const Print &print) noexcept;
Target find_target(const PrintObject &object) noexcept;
bool fingerprint_collection_enabled() noexcept;
bool session_active() noexcept;

} // namespace Detail

class Session
{
public:
    struct Config
    {
        Report *fingerprint_report {nullptr};
        bool    performance_enabled {false};
    };

    class Binding
    {
    public:
        Binding() = default;
        Binding(const Binding &) = delete;
        Binding &operator=(const Binding &) = delete;
        Binding(Binding &&other) noexcept;
        Binding &operator=(Binding &&other) noexcept;
        ~Binding();

        void reset() noexcept;

    private:
        friend class Session;
        Binding(const Print *print,
                std::vector<const PrintObject *> objects,
                bool fingerprint_enabled) noexcept;

        const Print                     *m_print {nullptr};
        std::vector<const PrintObject *> m_objects;
        bool                             m_fingerprint_enabled {false};
    };

    explicit Session(Config config) noexcept;
    ~Session() noexcept;
    Session(const Session &) = delete;
    Session &operator=(const Session &) = delete;

    Binding attach(const Print &print, int plate_id);

private:
    Config m_config;
};

} // namespace Diagnostics
} // namespace Slic3r

#endif
