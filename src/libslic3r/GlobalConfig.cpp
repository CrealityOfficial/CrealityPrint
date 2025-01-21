#include "GlobalConfig.hpp"
GlobalConfig* GlobalConfig::instance = nullptr;
 GlobalConfig* GlobalConfig::getInstance()
{
    if (instance == nullptr)
    {
        instance = new GlobalConfig();
    }
    return instance;
}
std::string GlobalConfig::getCurrentLanguage()
{
    //return GUI::wxGetApp()->get("language");
    return currentLanguage;
}
void GlobalConfig::setCurrentLanguage(std::string lang)
{
    currentLanguage = lang;
}
