#include "Performance.hpp"

#if defined(SLIC3R_DIAGNOSTICS_PERFORMANCE_ENABLED)

#include "Session.hpp"
#include "../Print.hpp"

#include <tracy/Tracy.hpp>

#include <algorithm>
#include <cstdio>
#include <new>
#include <utility>

namespace Slic3r::Diagnostics {

struct PerformanceScope::State
{
    State(std::string_view name, std::string_view scope) noexcept
        : zone(0,
               "Diagnostics", sizeof("Diagnostics") - 1,
               "performance", sizeof("performance") - 1,
               name.data(), name.size(),
               0, -1, true)
    {
        if (!scope.empty())
            zone.Text(scope.data(), scope.size());
    }

    tracy::ScopedZone zone;
};

PerformanceScope::PerformanceScope() noexcept = default;
PerformanceScope::PerformanceScope(PerformanceScope &&other) noexcept = default;

PerformanceScope &PerformanceScope::operator=(PerformanceScope &&other) noexcept
{
    if (this != &other) {
        end();
        m_state = std::move(other.m_state);
    }
    return *this;
}

PerformanceScope::~PerformanceScope() noexcept
{
    end();
}

void PerformanceScope::start(std::string_view name, const Print &print) noexcept
{
    end();
    if (name.empty() || !tracy::GetProfiler().IsConnected())
        return;

    const Detail::Target target = Detail::find_target(print);
    if (Detail::session_active() &&
        (!target.is_bound() || !target.performance_enabled()))
        return;

    const int plate_id = target.is_bound()
        ? target.scope.plate_id
        : print.get_plate_index() + 1;
    char scope[32];
    const int written = std::snprintf(scope, sizeof(scope), "plate_id=%d", plate_id);
    const std::size_t scope_size = written > 0
        ? std::min<std::size_t>(static_cast<std::size_t>(written), sizeof(scope) - 1)
        : 0;
    m_state.reset(new (std::nothrow) State(name, std::string_view(scope, scope_size)));
}
void PerformanceScope::start(std::string_view name, const PrintObject &object,
                             const std::size_t *sample_id) noexcept
{
    end();
    if (name.empty() || !tracy::GetProfiler().IsConnected())
        return;

    const Detail::Target target = Detail::find_target(object);
    if (Detail::session_active() &&
        (!target.is_bound() || !target.performance_enabled()))
        return;

    const Print *print = object.print();
    int plate_id = 0;
    std::size_t object_id = 0;
    if (target.is_bound()) {
        plate_id = target.scope.plate_id;
        object_id = target.scope.object_id;
    } else if (print != nullptr) {
        plate_id = print->get_plate_index() + 1;
        for (const PrintObject *candidate : print->objects()) {
            if (candidate == &object)
                break;
            ++object_id;
        }
    }

    char scope[96];
    int written = 0;
    if (plate_id <= 0) {
        written = sample_id == nullptr
            ? std::snprintf(scope, sizeof(scope), "object_id=%zu", object_id)
            : std::snprintf(scope, sizeof(scope), "object_id=%zu;sample_id=%zu",
                            object_id, *sample_id);
    } else {
        written = sample_id == nullptr
            ? std::snprintf(scope, sizeof(scope), "plate_id=%d;object_id=%zu",
                            plate_id, object_id)
            : std::snprintf(scope, sizeof(scope), "plate_id=%d;object_id=%zu;sample_id=%zu",
                            plate_id, object_id, *sample_id);
    }
    const std::size_t scope_size = written > 0
        ? std::min<std::size_t>(static_cast<std::size_t>(written), sizeof(scope) - 1)
        : 0;
    m_state.reset(new (std::nothrow) State(name, std::string_view(scope, scope_size)));
}

void PerformanceScope::end() noexcept
{
    m_state.reset();
}

bool PerformanceScope::active() const noexcept
{
    return m_state != nullptr;
}

PerformanceScope performance(std::string_view group_path,
                             const Print &print) noexcept
{
    PerformanceScope scope;
    scope.start(group_path, print);
    return scope;
}

PerformanceScope performance(std::string_view group_path,
                             const PrintObject &object) noexcept
{
    PerformanceScope scope;
    scope.start(group_path, object, nullptr);
    return scope;
}

PerformanceScope performance(std::string_view group_path,
                             const PrintObject &object,
                             std::size_t sample_id) noexcept
{
    PerformanceScope scope;
    scope.start(group_path, object, &sample_id);
    return scope;
}

} // namespace Slic3r::Diagnostics

#endif
