#include "Routes.hpp"
#include "nlohmann/json.hpp"
#include "../AppUtils.hpp"

namespace DM{
    bool Routes::Invoke(wxWebView* browser, const std::string& data)
    {
        nlohmann::json j = nlohmann::json::parse(data);
        if (j.contains("command")) {
            std::string command = j["command"].get<std::string>();
            auto it = m_handlers.find(command);
            if (it != m_handlers.end()) {
                return it->second(browser, data, j);
            }
        }

        return false;
    }

    void Routes::Handler(const std::string& command, std::function<bool(wxWebView* browse, const std::string&, const nlohmann::json&)> handler) {
        m_handlers[command] = std::move(handler);
    }

    Routes::Routes()
    {
    }
}