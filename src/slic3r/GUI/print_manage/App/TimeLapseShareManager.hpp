#ifndef slic3r_GUI_TimeLapseShareManager_hpp_
#define slic3r_GUI_TimeLapseShareManager_hpp_

#include "TimeLapseShareTypes.hpp"

#include <functional>
#include <memory>
#include <string>

namespace Slic3r { namespace GUI { namespace TimeLapseShare {

class TimeLapseShareManager final
{
public:
    struct StartResult
    {
        bool        accepted {false};
        std::string error_code;
        std::string error_message;
    };

    using EventCallback = std::function<void(const ShareEvent&)>;
    using AccountSessionProvider = std::function<std::string()>;

    TimeLapseShareManager(EventCallback event_callback, AccountSessionProvider account_session_provider);
    ~TimeLapseShareManager();

    TimeLapseShareManager(const TimeLapseShareManager&) = delete;
    TimeLapseShareManager& operator=(const TimeLapseShareManager&) = delete;

    StartResult Start(ShareRequest request);
    bool Cancel(const std::string& request_id);
    void Shutdown();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}}} // namespace Slic3r::GUI::TimeLapseShare

#endif /* slic3r_GUI_TimeLapseShareManager_hpp_ */
