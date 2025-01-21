#include "AppConfig.h"
#include "nlohmann/json.hpp"
#include <boost/nowide/cenv.hpp>
#include <boost/nowide/convert.hpp>
#include <boost/nowide/cstdio.hpp>
#include <boost/nowide/fstream.hpp>
using namespace nlohmann;

AppConfig::AppConfig() {}

AppConfig::~AppConfig() {}

AppConfig& AppConfig::getInstance() {
    static AppConfig instance;
    return instance;
}

void AppConfig::init(const std::string& config_file)
{
    if (!std::filesystem::exists(config_file)) {
        return;
    }
    boost::nowide::ifstream ifs(config_file);
    json                    j;
    ifs >> j;
}
