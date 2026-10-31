#ifndef slic3r_GUI_NoTowerOptions_hpp_
#define slic3r_GUI_NoTowerOptions_hpp_

namespace Slic3r::GUI {

inline bool show_machine_linked_no_tower_options(bool prime_tower_enabled, bool link_no_tower_options)
{
    return !prime_tower_enabled && link_no_tower_options;
}

} // namespace Slic3r::GUI

#endif
