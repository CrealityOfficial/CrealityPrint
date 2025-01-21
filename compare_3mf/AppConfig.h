#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <unordered_set>
#include <map>
#include <string>

class AppConfig
{
public:
    static AppConfig& getInstance();

    void init(const std::string& config_file);

private:
    AppConfig();
    ~AppConfig();
    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;

    std::unordered_set<std::string> mandatory_files_;
    std::map<std::string, std::unordered_set<std::string> > ignore_file_keys_;
};

#endif // !APP_CONFIG_H