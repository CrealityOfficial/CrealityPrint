#include <iostream>
class GlobalConfig
{
private:
    std::string currentLanguage = "";
    GlobalConfig() {}
    static GlobalConfig* instance;
public:
    static GlobalConfig *getInstance();
    std::string getCurrentLanguage();
    void setCurrentLanguage(std::string lang);
};

