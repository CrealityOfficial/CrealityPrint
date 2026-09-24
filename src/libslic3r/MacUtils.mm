#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#include <string> 
#import "MacUtils.hpp"

namespace Slic3r {

std::string get_system_language()
{
    @autoreleasepool {
        NSArray *languages = [NSLocale preferredLanguages];
        if (languages && [languages count] > 0) {
            NSString *language = [languages objectAtIndex:0];
            std::string lang = [language UTF8String];
            
            // 转换系统语言代码到应用支持的语言
            if (lang.find("zh-Hans") != std::string::npos || lang.find("zh-CN") != std::string::npos) {
                return "zh_CN"; // 简体中文
            } else if (lang.find("zh-Hant") != std::string::npos || lang.find("zh-TW") != std::string::npos) {
                return "zh_TW"; // 繁体中文
            } else if (lang.find("ja") != std::string::npos) {
                return "ja";
            } else if (lang.find("ko") != std::string::npos) {
                return "ko";
            } else if (lang.find("de") != std::string::npos) {
                return "de";
            } else if (lang.find("fr") != std::string::npos) {
                return "fr";
            } else if (lang.find("es") != std::string::npos) {
                return "es";
            }
            return "en"; // 默认英语
        }
        return "en";
    }
}

bool is_macos_support_boost_add_file_log()
{
    if (@available(macOS 12.0, *)) {
		return true;
	} else {
	    return false;
	}
}
int is_mac_version_15()
{
    if (@available(macOS 15.0, *)) {//This code runs on macOS 15 or later.
        return true;
    } else {
        return false;
    }
}
void macos_set_menu_bar_hidden(bool enabled)
{
    [NSMenu setMenuBarVisible:!enabled];
}

void activate_app()
{
    @autoreleasepool {
        [NSApp activateIgnoringOtherApps:YES];
    }
}
} // namespace Slic3r
