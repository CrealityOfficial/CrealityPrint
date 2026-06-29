// Minimal device list popup rendering for Simple UI
#pragma once

#include <map>
#include <string>
#include "libslic3r/Format/bbs_3mf.hpp"
#include "libslic3r/Model.hpp"

namespace Slic3r { namespace GUI {

struct Simple_Device_List_Item_Data
{
    std::string name;
    std::string cover_path;
    std::string model_name;
    std::string address;
    std::string mac;
    int         state;
    bool        online;
    bool        isCurrent;
    int         device_type;
    bool        visible = true;
};

struct Simple_Device_List_Data
{
    std::map<std::string, Simple_Device_List_Item_Data> datas;
    std::set<std::string> online_device_list;
    std::set<std::string> offline_device_list;
    // std::pair<std::string, std::vector<std::string>> meaning: first is <cloud key>, second is <local keys>
    std::unordered_map<std::string, std::pair<std::string, std::vector<std::string>> > mac_2_key_map;

    void clear() {
        datas.clear();
        online_device_list.clear();
        offline_device_list.clear();
        mac_2_key_map.clear();
    }
    void push(std::string name, Simple_Device_List_Item_Data item);
    void manager_duplicate_deivce(std::pair<std::string, std::vector<std::string>>& duplicate_deivces);

//     unsigned int get_texture_id(std::string coverPath)
//     {
//         return cover2textureId[coverPath];
//     }

//     ~Simple_Device_List_Data();

// private:
//     std::map<std::string, unsigned int> cover2textureId;

};

// Prefer live data from DataCenter; fall back to preset list if needed.
// Cache last state to avoid recomputing strings / icon lookup every frame.
struct DeviceCachedInfo 
{
    bool        valid   = false;
    std::string mac;
    std::string name;
    std::string addr;
    std::string model;
    int         online  = -1;
    int         state   = -1;
    std::string title;
    std::string subtitle;
    std::string icon_path;
};

class GLCanvas3D;

class SimpleDeviceMgr
{
public:
    static SimpleDeviceMgr& instance();

    SimpleDeviceMgr(const SimpleDeviceMgr&) = delete;
    SimpleDeviceMgr& operator=(const SimpleDeviceMgr&) = delete;
    SimpleDeviceMgr(SimpleDeviceMgr&&) = delete;
    SimpleDeviceMgr& operator=(SimpleDeviceMgr&&) = delete;

    std::string get_preset_name_by_current_device(bool& already_loaded);
    bool is_current_device_match_current_preset();
    void clear();
    void on_device_switch_and_check_preset_change();

    bool check_device_change_and_cache();

    int get_last_selected_preset_idx() const { return m_last_selected_preset_idx; }
    void set_last_selected_preset_idx(int idx) { m_last_selected_preset_idx = idx; }
    bool is_printer_preset_changed();

    bool switch_current_device_simple(const std::string& mac, bool sync_printer_preset = true);

    void ensure_current_device_initialized();
    const Simple_Device_List_Data& get_device_list_data_simple(bool force_refresh = false);
    bool set_cur_device_by_cur_preset_simple();

    // Compute display title/subtitle/icon for current device pill. Returns true if refreshed.
    bool get_cur_device_info(std::string& out_title, std::string& out_subtitle, std::string& out_icon_path);

    std::string get_same_model_device_mac_with_current_preset();

    bool check_need_to_change_current_preset(const Slic3r::DynamicConfig& loaded_config, const Slic3r::PlateDataPtrs& plate_datas, Slic3r::Model& model);

private:
    SimpleDeviceMgr() = default;

    void auto_change_printer_preset(const std::string& preset_name);
    bool auto_select_printer_preset(const std::string& printer_preset_name);

    void update_printer_device_list_data_simple(std::string vendor, bool bForce = false);
    void update_same_model_printer_device_list_data(std::string vendor, bool bForce = false);

    // Internal helper to build device lists, with a filter predicate.
    template<typename Predicate>
    void rebuild_device_list(Simple_Device_List_Data& out, const std::string& vendor, Predicate pred);

    void check_current_preset_match_printer();
    void check_diff_settings_to_system(const Slic3r::DynamicConfig& loaded_config);

    DeviceCachedInfo m_current_device_cache;

    int m_last_selected_preset_idx = -1;

    bool m_device_list_dirty_mark = true;
    Simple_Device_List_Data m_simple_device_list_data;
    Simple_Device_List_Data m_same_model_device_list_data;

    std::vector<std::vector<int>> m_3mf_ori_plate_objects;
};

// Render the simple device list popover content inside the current ImGui window.
// The coordinates are provided for parity with SupportSimple but are not required
// by this implementation (content is laid out relative to the current window).
void render_device_list_popup(GLCanvas3D& canvas, float x, float y, float bottom_limit);


}} // namespace Slic3r::GUI

