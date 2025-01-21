#include "AppMgr.hpp"
#include "Routes/AppRoutes.hpp"
#include "TypeDefine.hpp"
namespace DM{

    struct AppMgr::priv
    {
        Apps m_apps;
        AppRoutes m_routes;
    };

    void AppMgr::Register(wxWebView* browser)
    {
        impl->m_apps.emplace_back(browser);
    }

    void AppMgr::UnRegister(wxWebView* browser)
    {
        impl->m_apps.erase(std::remove_if(impl->m_apps.begin(), impl->m_apps.end(), [browser](const App& app) {
                return app.browser == browser;
        }),
        impl->m_apps.end());
    }

    void AppMgr::RegisterEvents(wxWebView* browser, std::vector<std::string> events)
    {
        for (auto& app : impl->m_apps) {
            if (app.browser == browser) {
                app.events = std::move(events);
            }
        }
    }

    bool AppMgr::Invoke(wxWebView* browser, std::string data)
    {
        nlohmann::json j = nlohmann::json::parse(data);
        if (j.contains("command")) {
            return impl->m_routes.Invoke(browser, data);
        }

        return false;
    }

    void AppMgr::GetNeedToReceiveSysEventApp(std::string event, Apps& apps)
    {
        for (const auto& app : impl->m_apps) {
            if (std::find(app.events.begin(), app.events.end(), event) != app.events.end()) {
                apps.push_back(app);
            }
        }
    }


    AppMgr& AppMgr::Ins()
    {
        static AppMgr appmgr;
        return appmgr;
    }

    void AppMgr::SystemThemeChanged()
    {
        nlohmann::json commandJson = {{"command", DM::EVENT_SET_SYS_THEME},};

        for (const auto& app : impl->m_apps) {
            impl->m_routes.Invoke(app.browser, commandJson.dump(-1, ' ', true));
        }
    }

    void AppMgr::SystemUserChanged()
    {
        nlohmann::json commandJson = { "command", DM::EVENT_SET_USER_THEME };

        for (const auto& app : impl->m_apps) {
            impl->m_routes.Invoke(app.browser, commandJson.dump(-1, ' ', true));
        }
    }

    AppMgr::AppMgr()
        : impl{ std::make_unique<priv>() }
    {
    }
}