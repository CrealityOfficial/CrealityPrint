#ifndef RemotePrint_AppMgr_hpp_
#define RemotePrint_AppMgr_hpp_
#include "nlohmann/json.hpp"
#include <string>
class wxWebView;
namespace DM{

    struct App
    {
        App(wxWebView* browser = nullptr)
            : browser(browser)
        {
        }

        wxWebView* browser = nullptr;
        std::vector<std::string> events;
    };

    typedef std::vector<App> Apps;


    class AppMgr
    {
    public:
        void Register(wxWebView* browser);
        void UnRegister(wxWebView* browser);
        void RegisterEvents(wxWebView* browser, std::vector<std::string> events);
        bool Invoke(wxWebView* browser, std::string data);
        void GetNeedToReceiveSysEventApp(std::string event, Apps& apps);
        static AppMgr& Ins();

    public://system event to every app.
        void SystemThemeChanged();
        void SystemUserChanged();
    private:
        AppMgr();

        struct priv;
        std::unique_ptr<priv> impl;
    };
}
#endif