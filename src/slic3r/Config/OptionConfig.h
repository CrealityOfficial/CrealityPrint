#ifndef OPTIONCONFIG_H
#define OPTIONCONFIG_H

#include <string>
#include <vector>

/*Tooltips, hyperlinks, and images in the tooltips used for initializing parameters*/
class OptionConfig
{
public:
    struct Option
    {
        std::string key;
        std::string tooltip;
        std::string tooltip_img;
        std::string url;

        bool is_valid() { return !key.empty(); }
    };

public:
    OptionConfig();
    virtual ~OptionConfig();
    static OptionConfig& Ins();
     
    OptionConfig::Option get_option(std::string key);
    std::string get_group_icon(std::string group);

private:
    struct priv;
    std::unique_ptr<priv> p;
};

#endif 
