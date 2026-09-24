#ifndef slic3r_Diagnostics_Performance_hpp_
#define slic3r_Diagnostics_Performance_hpp_

#include <cstddef>
#include <string_view>

#if defined(SLIC3R_DIAGNOSTICS_PERFORMANCE_ENABLED)
#include <memory>
#endif

namespace Slic3r {

class Print;
class PrintObject;

namespace Diagnostics {

#if defined(SLIC3R_DIAGNOSTICS_PERFORMANCE_ENABLED)

// Owns one dynamically named performance scope. Performance diagnostics are
// compiled only into the RelWithDebInfo application module; all other
// configurations use the inline no-op implementation below.
class PerformanceScope
{
public:
    PerformanceScope() noexcept;
    PerformanceScope(const PerformanceScope &) = delete;
    PerformanceScope &operator=(const PerformanceScope &) = delete;
    PerformanceScope(PerformanceScope &&other) noexcept;
    PerformanceScope &operator=(PerformanceScope &&other) noexcept;
    ~PerformanceScope() noexcept;

    void end() noexcept;
    bool active() const noexcept;

private:
    friend PerformanceScope performance(std::string_view group_path,
                                        const Print &print) noexcept;
    friend PerformanceScope performance(std::string_view group_path,
                                        const PrintObject &object) noexcept;
    friend PerformanceScope performance(std::string_view group_path,
                                        const PrintObject &object,
                                        std::size_t sample_id) noexcept;

    void start(std::string_view name, const Print &print) noexcept;
    void start(std::string_view name, const PrintObject &object,
               const std::size_t *sample_id) noexcept;

    struct State;
    std::unique_ptr<State> m_state;
};

PerformanceScope performance(std::string_view group_path,
                             const Print &print) noexcept;
PerformanceScope performance(std::string_view group_path,
                             const PrintObject &object) noexcept;
PerformanceScope performance(std::string_view group_path,
                             const PrintObject &object,
                             std::size_t sample_id) noexcept;
#else

class PerformanceScope
{
public:
    void end() noexcept {}
    bool active() const noexcept { return false; }
};

inline PerformanceScope performance(std::string_view,
                                    const Print &) noexcept
{
    return {};
}

inline PerformanceScope performance(std::string_view,
                                    const PrintObject &) noexcept
{
    return {};
}

inline PerformanceScope performance(std::string_view,
                                    const PrintObject &,
                                    std::size_t) noexcept
{
    return {};
}

#endif
} // namespace Diagnostics
} // namespace Slic3r

#endif
