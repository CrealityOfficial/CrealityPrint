#pragma once

#include <cstddef>
#include <string>
#include <unordered_set>

namespace Slic3r { namespace GUI { namespace WebSocketProxy {

// Executor-owned. An oversized message opts this endpoint into unlimited reads
// on subsequent connections; reloading the page restores the initial limit.
class WebSocketReadLimitPolicy
{
public:
    explicit WebSocketReadLimitPolicy(std::size_t initial_limit)
        : m_initial_limit(initial_limit) {}

    std::size_t Limit(const std::string& endpoint) const
    {
        return m_unlimited.count(endpoint) ? 0 : m_initial_limit;
    }
    void Exceeded(const std::string& endpoint) { m_unlimited.insert(endpoint); }
    void Clear() { m_unlimited.clear(); }

private:
    std::size_t m_initial_limit;
    std::unordered_set<std::string> m_unlimited;
};

} } }
