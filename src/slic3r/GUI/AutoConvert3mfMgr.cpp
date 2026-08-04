#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <filesystem>
#include <functional>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <string>
#include <codecvt>
#include <regex>
#include <format>
#include <map>
#include <boost/nowide/fstream.hpp>
#include <boost/filesystem.hpp>
#include <wx/timer.h>
#include "AutoConvert3mfMgr.hpp"
#include "libslic3r_version.h"
// #include "libslic3r.h"
// #include "Utils.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/ModelObject.hpp"
#include "libslic3r/ModelVolume.hpp"
#include "libslic3r/ModelInstance.hpp"
#include "GUI_App.hpp"
#include "SiderBar.h"
#include "Plater.hpp"
#include "PartPlate.hpp"
#include "Tab.hpp"
#include "GUI_Preview.hpp"
#include "MainFrame.hpp"
#include "NotificationManager.hpp"
namespace Slic3r {

namespace GUI {

void AutoConvert3mfMgr::start_conversion()
{
    m_has_serious_warnings = false;
    if (!m_printer_preset.empty() && !m_output_3mf_name.empty()) {
        convert_to_printer();
    }
}

void AutoConvert3mfMgr::convert_to_printer()
{
    wxGetApp().Bind(EVT_SLICE_ALL_PLATE_FINISHED, [this](wxCommandEvent& e) {
        bool has_error = e.GetInt() != 0;
        if (!has_error && !m_has_serious_warnings) {
            SaveStrategy save_strategy;
            if (m_conversion_mode == ConversionMode::STL_TO_3MF) {
                save_strategy = SaveStrategy::WithGcode | SaveStrategy::SplitModel | SaveStrategy::ShareMesh |
                                SaveStrategy::FullPathSources;
            } else {
                save_strategy = SaveStrategy::WithGcode | SaveStrategy::SplitModel | SaveStrategy::ShareMesh |
                                SaveStrategy::FullPathSources;
            }
            //(3) export 3mf
            if (wxGetApp().plater()->export_3mf(into_path(m_output_3mf_name), save_strategy, -1, nullptr, En3mfType::From_Creality)) {}
        }
        // 创建定时器，延迟 1 秒后退出程序
        wxTimer* timer = new wxTimer();
        timer->Bind(wxEVT_TIMER, [this, timer](wxTimerEvent& event) {
            // 定时器触发后退出程序
            if (wxGetApp().mainframe) {
                wxCloseEvent close_evt(wxEVT_CLOSE_WINDOW);
                close_evt.SetCanVeto(false);
                wxQueueEvent(wxGetApp().mainframe, close_evt.Clone());
            }

            // 停止并删除定时器
            timer->Stop();
            delete timer;
        });

        // 启动定时器，1000 毫秒 = 1 秒
        timer->Start(1000, wxTIMER_ONE_SHOT);
    });
    if (m_conversion_mode == ConversionMode::STL_TO_3MF) {
        auto_select_printer_preset(m_printer_preset);
        auto_change_filament_and_process_preset();
        wxGetApp().plater()->set_prepare_state(Job::PREPARE_STATE_DEFAULT);
        wxGetApp().plater()->arrange();
    } else {
        auto_change_printer_preset(m_printer_preset);
    }
}

void AutoConvert3mfMgr::start_sequential_arrange()
{
    m_current_plate_index = 0;
    arrange_next_plate();
}

void AutoConvert3mfMgr::arrange_next_plate()
{
    if (m_current_plate_index >= m_plate_object.size()) {
        on_all_plate_arrangement_finished();
        return; // 所有盘处理完成
    }

    Plater* plater = wxGetApp().plater();
    if (!plater) {
        m_current_plate_index++;
        return;
    }

    PartPlateList& plate_list = plater->get_partplate_list();
    Model&         model      = plater->model();
    if (model.objects.size() <= 0) {
        m_current_plate_index++;
        return;
    }

    Sidebar& sidebar = plater->sidebar();
    View3D*  view3D  = plater->get_vew3D();
    if (!view3D) {
        m_current_plate_index++;
        return;
    }
    view3D->deselect_all();
    int i = m_current_plate_index;
    plater->select_plate(i);
    // view3D->select_object_from_idx(m_plate_object[i]);
    // sidebar.obj_list()->update_selections();
    // view3D->center_selected_plate(i);
    do_necessary_arrange(i);
    m_current_plate_index++;
}

void AutoConvert3mfMgr::on_arrange_job_finished()
{
    Plater* plater = wxGetApp().plater();
    if (!plater)
        return;

    arrange_next_plate(); // 处理下一个盘
}

void AutoConvert3mfMgr::do_necessary_arrange(int plate_index)
{
    Plater* plater = wxGetApp().plater();
    if (!plater)
        return;
    Model& model = plater->model();
    if (model.objects.size() <= 0) {
        if (wxGetApp().mainframe) {
            wxGetApp().ExitMainLoop();
        }
        return;
    }
    PartPlate* cur_plate = plater->get_partplate_list().get_plate(plate_index);
    if (!cur_plate)
        return;
    // 处理空盘
    ModelObjectPtrs objs = cur_plate->get_objects_on_this_plate();
    if (objs.size() == 0) {
        wxGetApp().ExitMainLoop();
    }
    plater->set_prepare_state(Job::PREPARE_STATE_EXTRA);
    plater->arrange();
}

// check original logic from "Plater::priv::on_select_preset(wxCommandEvent& evt)"
void AutoConvert3mfMgr::auto_select_printer_preset(const std::string& printer_name)
{
    Plater* plater = wxGetApp().plater();
    if (!plater)
        return;
    Sidebar&     sidebar     = plater->sidebar();
    Preset::Type preset_type = Preset::TYPE_PRINTER;
    // std::string preset_name = wxGetApp().preset_bundle->get_preset_name_by_alias(preset_type,
    //                                                                              Preset::remove_suffix_modified(
    //                                                                                  combo->GetString(selection).ToUTF8().data()));
    std::string preset_name = printer_name;
    // bool select_preset = !combo->selection_is_changed_according_to_physical_printers();
    bool select_preset = true;
    if (select_preset) {
        if (preset_type == Preset::TYPE_PRINTER) {
            PhysicalPrinterCollection& physical_printers = wxGetApp().preset_bundle->physical_printers;
            // if (combo->is_selected_physical_printer())
            if (0)
                preset_name = physical_printers.get_selected_printer_preset_name();
            else
                physical_printers.unselect_printer();
        }
        // BBS
        // wxWindowUpdateLocker noUpdates1(sidebar->print_panel());
        wxWindowUpdateLocker noUpdates2(sidebar.filament_panel());
        wxGetApp().get_tab(preset_type)->select_preset(preset_name);
    }
    // update plater with new config
    plater->on_config_change(wxGetApp().preset_bundle->full_config());
    if (preset_type == Preset::TYPE_PRINTER) {
        /* Settings list can be changed after printer preset changing, so
         * update all settings items for all item had it.
         * Furthermore, Layers editing is implemented only for FFF printers
         * and for SLA presets they should be deleted
         */
        wxGetApp().obj_list()->update_object_list_by_printer_technology();
    }
}

void AutoConvert3mfMgr::auto_change_printer_preset(const std::string& printer_name)
{
    Plater* plater = wxGetApp().plater();
    if (!plater)
        return;
    Model& model = plater->model();
    if (model.objects.size() <= 0)
        return;
    Sidebar& sidebar = plater->sidebar();
    View3D*  view3D  = plater->get_vew3D();
    if (!view3D)
        return;
    m_plate_object.clear();
    m_need_arrange_object.clear();
    Preset::Type preset_type = Preset::TYPE_PRINTER;
    // BBS:Save the plate parameters before switching
    PartPlateList&   old_plate_list = plater->get_partplate_list();
    PartPlate*       old_plate      = old_plate_list.get_selected_plate();
    Vec3d            old_plate_pos  = old_plate->get_center_origin();
    std::vector<int> out_plate_obj_idxs;
    out_plate_obj_idxs.clear();
    std::vector<int> tmp_in_plate_obj_idxs;
    // BBS: Save the model in the current platelist
    for (size_t i = 0; i < old_plate_list.get_plate_count(); ++i) {
        PartPlate*       plate = old_plate_list.get_plate(i);
        std::vector<int> obj_idxs;
        for (int obj_idx = 0; obj_idx < model.objects.size(); obj_idx++) {
            if (plate && plate->contain_instance(obj_idx, 0)) {
                obj_idxs.emplace_back(obj_idx);
                tmp_in_plate_obj_idxs.emplace_back(obj_idx);
            }
        }
        m_plate_object.emplace_back(obj_idxs);
    }
    for (int k = 0; k < model.objects.size(); k++) {
        if (std::find(tmp_in_plate_obj_idxs.begin(), tmp_in_plate_obj_idxs.end(), k) == tmp_in_plate_obj_idxs.end()) {
            out_plate_obj_idxs.emplace_back(k);
        }
    }
    m_need_arrange_object.reserve(old_plate_list.get_plate_count());
    auto_select_printer_preset(printer_name);
    if (1) {
        // BBS:Model reset by plate center
        PartPlateList& cur_plate_list = plater->get_partplate_list();
        PartPlate*     cur_plate      = cur_plate_list.get_curr_plate();
        Vec3d          cur_plate_pos  = cur_plate->get_center_origin();
        // if (old_plate_pos.x() != cur_plate_pos.x() || old_plate_pos.y() != cur_plate_pos.y()) {
        if (1) { // always start sequential arrange; ("K2 0.4 nozzle" and "Hi 0.4 nozzle" have the same plate_width and plate_height)

            for (int i = 0; i < m_plate_object.size(); ++i) {
                view3D->select_object_from_idx(m_plate_object[i]);
                sidebar.obj_list()->update_selections();
                view3D->center_selected_plate(i);
                this->check_object_need_arrange_state(i);
            }
            // if in the original 3mf, there are some ModelObjects  out of any plate;  need to put them on top of first plate
            PartPlate* first_plate = cur_plate_list.get_plate(0);
            if (!first_plate)
                return;
            BoundingBoxf3 plate_bbox = first_plate->get_build_volume();
            for (int k = 0; k < out_plate_obj_idxs.size(); k++) {
                int object_idx = out_plate_obj_idxs[k];
                if (object_idx < 0 || object_idx >= model.objects.size())
                    continue;
                ModelObject* mo = model.objects[object_idx];
                if (!mo)
                    continue;
                for (ModelInstance* mi : mo->instances) {
                    Vec3d instance_bbox_size = mi->get_object()->instance_bounding_box(0).size();
                    auto  offset             = mi->get_offset();
                    Vec3d top_left = {plate_bbox.min.x() + instance_bbox_size.x(), plate_bbox.max.y() + instance_bbox_size.y(), offset(2)};
                    mi->set_offset(top_left);
                }
            }
            start_sequential_arrange();
            // view3D->deselect_all();
        } else {
            // 处理空盘
            for (int i = 0; i < m_plate_object.size(); ++i) {
                ModelObjectPtrs objs = cur_plate_list.get_plate_list()[i]->get_objects_on_this_plate();
                if (objs.size() == 0) {
                    wxGetApp().ExitMainLoop();
                    return;
                }
            }
            on_all_plate_arrangement_finished();
        }
    }
}

void AutoConvert3mfMgr::auto_change_filament_and_process_preset()
{
    if (m_filament_preset_name.empty() || m_process_preset_name.empty()) {
        return;
    }
    PresetBundle&        preset_bundle       = *wxGetApp().preset_bundle;
    std::vector<Preset*> project_presets     = preset_bundle.get_current_project_embedded_presets();
    std::string          used_printer_preset = m_printer_preset;
    if (preset_bundle.printers.find_preset(used_printer_preset) == nullptr) {
        used_printer_preset = "Creality K2 Plus 0.4 nozzle";
        assert(preset_bundle.printers.find_preset(used_printer_preset));
    }
    std::string used_filament_preset = (boost::format("%1% @%2%") % m_filament_preset_name % used_printer_preset).str();
    std::string filament_preset_name = m_filament_preset_name;
    if (preset_bundle.filaments.find_preset(used_filament_preset) == nullptr) {
        filament_preset_name = "Hyper PLA";
        used_filament_preset = "Hyper PLA @Creality K2 Plus 0.4 nozzle";
        assert(preset_bundle.filaments.find_preset(used_filament_preset));
    }
    std::string used_process_preset = (boost::format("%1% @%2%") % m_process_preset_name % used_printer_preset).str();
    std::string process_preset_name = m_process_preset_name;
    if (preset_bundle.prints.find_preset(used_process_preset) == nullptr) {
        process_preset_name = "0.20mm Standard";
        used_process_preset = "0.20mm Standard @Creality K2 Plus 0.4 nozzle";
        assert(preset_bundle.prints.find_preset(used_process_preset));
    }
    auto find_index = [](Slic3r::PresetCollection& collections, const std::string& name) -> size_t {
        const std::deque<Slic3r::Preset>& presets = collections.get_presets();
        for (auto it = presets.begin(); it != presets.end(); ++it) {
            if (it->name == name)
                return it - presets.begin();
        }
        return 0;
    };
    // size_t printer_idx = find_index(preset_bundle.printers, used_printer_preset);
    // preset_bundle.printers.select_preset(printer_idx);
    // size_t filament_idx = find_index(preset_bundle.filaments, used_filament_preset);
    // preset_bundle.filaments.select_preset(filament_idx);
    preset_bundle.set_filament_preset(0, used_filament_preset);
    size_t process_idx = find_index(preset_bundle.prints, used_process_preset);
    preset_bundle.prints.select_preset(process_idx);
    preset_bundle.prints.get_edited_preset().config.set_key_value("enable_support", new Slic3r::ConfigOptionBool(true));
    preset_bundle.prints.get_edited_preset().config.set_key_value("support_type", new Slic3r::ConfigOptionEnum<SupportType>(stTreeAuto));
    std::string default_filament_color = "#00FF80";
    // get current color
    Slic3r::DynamicPrintConfig* cfg    = &Slic3r::GUI::wxGetApp().preset_bundle->project_config;
    auto                        colors = static_cast<Slic3r::ConfigOptionStrings*>(cfg->option("filament_colour")->clone());
    colors->values[0]                  = default_filament_color;
    cfg->set_key_value("filament_colour", colors);
}

void AutoConvert3mfMgr::on_all_plate_arrangement_finished()
{
    Plater* plater = wxGetApp().plater();
    if (!plater) {
        wxGetApp().ExitMainLoop();
        return;
    }
    PartPlateList& part_plate_list = plater->get_partplate_list();
    if (!part_plate_list.is_all_plates_ready_for_slice()) {
        wxGetApp().ExitMainLoop();
        return;
    }
    // BBS: log modify of filament selection
    Slic3r::put_other_changes();
    // update slice state and set bedtype default for 3rd-party printer
    auto plate_list = wxGetApp().plater()->get_partplate_list().get_plate_list();
    for (auto plate : plate_list) {
        plate->update_slice_result_valid_state(false);
    }
    //(2) after all plates finish the arrangement, begin slice all plate
    wxCommandEvent evt = wxCommandEvent(EVT_GLTOOLBAR_SLICE_ALL);
    wxPostEvent(wxGetApp().plater(), evt);
}

void AutoConvert3mfMgr::check_object_need_arrange_state(int plate_index)
{
    Plater* plater = wxGetApp().plater();
    if (!plater)
        return;
    Model& model = plater->model();
    if (model.objects.size() <= 0)
        return;
    PartPlate* cur_plate = plater->get_partplate_list().get_plate(plate_index);
    if (!cur_plate)
        return;

    Selection&                                      selection = plater->get_view3D_canvas3D()->get_selection();
    std::vector<const Selection::InstanceIdxsList*> obj_sel(model.objects.size(), nullptr);
    for (auto& s : selection.get_content()) {
        if (s.first < int(obj_sel.size()))
            obj_sel[size_t(s.first)] = &s.second;
    }
    // ModelObjects that partially out of current plate or totally out of current plate  after printer change, store the "from_loaded_id" of
    // ModelObject
    std::vector<int> need_arrange_obj_loaded_ids;
    // Go through the objects and check if inside the selection
    for (size_t oidx = 0; oidx < model.objects.size(); ++oidx) {
        const Selection::InstanceIdxsList* instlist = obj_sel[oidx];
        ModelObject*                       mo       = model.objects[oidx];
        if (!mo || mo->from_loaded_id < 0)
            continue;
        std::vector<bool> inst_sel(mo->instances.size(), false);
        if (instlist) {
            for (auto inst_id : *instlist)
                inst_sel[size_t(inst_id)] = true;
        }
        for (size_t i = 0; i < inst_sel.size(); ++i) {
            bool totally_in_plate = cur_plate->contain_instance_totally(oidx, i);
            if (!inst_sel[i]) {
                continue;
            }
            if (totally_in_plate)
                continue;
            // mark down the "from_loaded_id" instead of the "oidx", because the model.objects may be re sorted after the arrangement
            need_arrange_obj_loaded_ids.emplace_back(mo->from_loaded_id);
        }
    }
    m_need_arrange_object.emplace_back(need_arrange_obj_loaded_ids);
}

std::vector<int> AutoConvert3mfMgr::get_need_arrange_object_ids(int plate_index)
{
    if (plate_index >= 0 && plate_index < m_need_arrange_object.size()) {
        return m_need_arrange_object[plate_index];
    }
    return std::vector<int>();
}

void AutoConvert3mfMgr::set_serious_warning_state(bool has_serious_warning) 
{ 
    m_has_serious_warnings = has_serious_warning; 
}

}

} // namespace Slic3r::GUI