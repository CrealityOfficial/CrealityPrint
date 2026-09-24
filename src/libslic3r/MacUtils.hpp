#ifndef __MAC_UTILS_H
#define __MAC_UTILS_H

namespace Slic3r {

bool is_macos_support_boost_add_file_log();
void activate_app();
int  is_mac_version_15();
void macos_set_menu_bar_hidden(bool hidden);

#ifdef __APPLE__
std::string get_system_language();
#endif // __APPLE__
}

#endif
