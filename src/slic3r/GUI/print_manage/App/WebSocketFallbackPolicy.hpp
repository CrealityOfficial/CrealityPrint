#pragma once

#include <string>
#include <unordered_map>

namespace Slic3r { namespace GUI { namespace WebSocketProxy {

// Executor-owned; a page reload starts a fresh secure-first cycle.
class WebSocketFallbackPolicy
{
public:
    static constexpr unsigned failure_limit = 3;
    bool UsePlaintext(const std::string& endpoint) const
    {
        const auto it = m_failures.find(endpoint);
        return it != m_failures.end() && it->second >= failure_limit;
    }
    unsigned Failed(const std::string& endpoint)
    {
        auto& count = m_failures[endpoint];
        if (count < failure_limit) ++count;
        return count;
    }
    void Healthy(const std::string& endpoint) { m_failures.erase(endpoint); }
    void Clear() { m_failures.clear(); }
private:
    std::unordered_map<std::string, unsigned> m_failures;
};

} } }
