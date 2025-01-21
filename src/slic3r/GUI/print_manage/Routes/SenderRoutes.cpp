#include "SenderRoutes.hpp"
#include "nlohmann/json.hpp"
#include "../AppUtils.hpp"

namespace DM{
    SenderRoutes::SenderRoutes()
    {
        this->Handler("is_dark_theme", [](wxWebView*browse, const std::string& data, const nlohmann::json& json_data) {
            //AppUtils::PostMsg(browse, );
            return true;
        });
    }
}