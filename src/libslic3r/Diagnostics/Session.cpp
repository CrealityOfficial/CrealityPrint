#include "Session.hpp"

#include "Report.hpp"
#include "../Print.hpp"

#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace Slic3r::Diagnostics {
namespace {

struct Registry
{
    std::shared_mutex mutex;
    std::unordered_map<const Print *, Detail::Target> print_targets;
    std::unordered_map<const PrintObject *, Detail::Target> object_targets;
    std::atomic_size_t session_count {0};
    std::atomic_size_t binding_count {0};
    std::atomic_size_t fingerprint_binding_count {0};
};

Registry &registry()
{
    static Registry value;
    return value;
}

void detach(const Print *print,
            const std::vector<const PrintObject *> &objects,
            bool fingerprint_enabled) noexcept
{
    if (print == nullptr)
    {
        return;
    }

    Registry &state = registry();
    std::unique_lock<std::shared_mutex> lock(state.mutex);
    state.print_targets.erase(print);

    for (const PrintObject* object : objects)
    {
        state.object_targets.erase(object);
    }
        
    state.binding_count.fetch_sub(1, std::memory_order_release);
    if (fingerprint_enabled)
        state.fingerprint_binding_count.fetch_sub(1, std::memory_order_release);
}

} // namespace

namespace Detail {

Target find_target(const Print &print) noexcept
{
    Registry &state = registry();
    if (state.binding_count.load(std::memory_order_acquire) == 0)
        return {};

    std::shared_lock<std::shared_mutex> lock(state.mutex);
    const auto target = state.print_targets.find(&print);
    return target != state.print_targets.end() ? target->second : Target{};
}

Target find_target(const PrintObject &object) noexcept
{
    Registry &state = registry();
    if (state.binding_count.load(std::memory_order_acquire) == 0)
        return {};

    std::shared_lock<std::shared_mutex> lock(state.mutex);
    const auto target = state.object_targets.find(&object);
    return target != state.object_targets.end() ? target->second : Target{};
}

bool fingerprint_collection_enabled() noexcept
{
    return registry().fingerprint_binding_count.load(std::memory_order_acquire) != 0;
}

bool session_active() noexcept
{
    return registry().session_count.load(std::memory_order_acquire) != 0;
}

} // namespace Detail

Session::Session(Config config) noexcept : m_config(config)
{
    registry().session_count.fetch_add(1, std::memory_order_release);
}

Session::~Session() noexcept
{
    registry().session_count.fetch_sub(1, std::memory_order_release);
}

Session::Binding::Binding(const Print *print,
                          std::vector<const PrintObject *> objects,
                          bool fingerprint_enabled) noexcept
    : m_print(print)
    , m_objects(std::move(objects))
    , m_fingerprint_enabled(fingerprint_enabled)
{
}

Session::Binding::Binding(Binding &&other) noexcept
    : m_print(std::exchange(other.m_print, nullptr))
    , m_objects(std::move(other.m_objects))
    , m_fingerprint_enabled(std::exchange(other.m_fingerprint_enabled, false))
{
}

Session::Binding &Session::Binding::operator=(Binding &&other) noexcept
{
    if (this != &other) 
    {
        reset();
        m_print = std::exchange(other.m_print, nullptr);
        m_objects = std::move(other.m_objects);
        m_fingerprint_enabled = std::exchange(other.m_fingerprint_enabled, false);
    }

    return *this;
}

Session::Binding::~Binding()
{
    reset();
}

void Session::Binding::reset() noexcept
{
    detach(m_print, m_objects, m_fingerprint_enabled);
    m_print = nullptr;
    m_objects.clear();
    m_fingerprint_enabled = false;
}

Session::Binding Session::attach(const Print &print, int plate_id)
{
    if (plate_id <= 0)
    {
        throw std::invalid_argument("diagnostic session plate_id must be positive");
    }

    if (m_config.fingerprint_report == nullptr && !m_config.performance_enabled)
        return {};

    std::vector<const PrintObject *> objects;
    objects.reserve(print.objects().size());
    for (const PrintObject* object : print.objects())
    {
        objects.push_back(object);
    }
        
    const Detail::Target print_target{
        m_config.fingerprint_report,
        m_config.performance_enabled,
        true,
        Scope{plate_id}};

    Registry &state = registry();
    {
        std::unique_lock<std::shared_mutex> lock(state.mutex);

        if (state.print_targets.count(&print) != 0)
        {
            throw std::logic_error("Print is already attached to a diagnostic session");
        }
            
        for (const PrintObject *object : objects) 
        {
            if (state.object_targets.count(object) != 0)
            {
                throw std::logic_error("PrintObject is already attached to a diagnostic session");
            }
        }

        state.print_targets.emplace(&print, print_target);
        try 
        {
            for (std::size_t object_id = 0; object_id < objects.size(); ++object_id) 
            {
                Detail::Target target{
                    m_config.fingerprint_report,
                    m_config.performance_enabled,
                    true,
                    Scope{plate_id, object_id}};
                state.object_targets.emplace(objects[object_id],target);
            }
        } 
        catch (...)
        {
            state.print_targets.erase(&print);
            for (const PrintObject *object : objects)
                state.object_targets.erase(object);
            throw;
        }
        state.binding_count.fetch_add(1, std::memory_order_release);
        if (m_config.fingerprint_report != nullptr)
            state.fingerprint_binding_count.fetch_add(1, std::memory_order_release);
    }

    try 
    {
        if (m_config.fingerprint_report != nullptr)
        {
            m_config.fingerprint_report->begin_plate(plate_id);
        }
    } 
    catch (...) 
    {
        detach(&print, objects, m_config.fingerprint_report != nullptr);
        throw;
    }

    return Binding(
        &print, std::move(objects), m_config.fingerprint_report != nullptr);
}

} // namespace Slic3r::Diagnostics
