#include "SimpleModelMgr.hpp"

#include "slic3r/GUI/GLCanvas3D.hpp"
#include "slic3r/GUI/ImGuiWrapper.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/I18N.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/GUI_ObjectList.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "slic3r/GUI/GUI_Preview.hpp"
#include "slic3r/GUI/Tab.hpp"
#include "slic3r/GUI/ConfigWizard.hpp"
#include "print_manage/data/DataCenter.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/ModelInstance.hpp"
#include <nlohmann/json.hpp>

#include <imgui/imgui.h>
#include <unordered_map>
#include <GL/glew.h>
#include "slic3r/GUI/MsgDialog.hpp"
#include "GLSimpleUtils.hpp"

namespace Slic3r { namespace GUI {


SimpleModelMgr& SimpleModelMgr::instance()
{
    static SimpleModelMgr inst;
    return inst;
}

void SimpleModelMgr::mark_3mf_plate_objects(const PlateDataPtrs& plate_data, const Slic3r::Model& model)
{
    m_3mf_ori_plate_objects.clear();
    std::vector<int> obj_idxs;
    for (int i = 0; i < plate_data.size(); ++i) {
        obj_idxs.clear();
        for (auto item : plate_data[i]->obj_inst_map) {
            for (int k = 0; k < model.objects.size(); k++) {
                if (item.first == model.objects[k]->from_loaded_id) {
                    obj_idxs.emplace_back(k);
                }
            }
        }
        m_3mf_ori_plate_objects.emplace_back(obj_idxs);
    }

    m_need_reposition = true;
}

void SimpleModelMgr::center_3mf_objects_to_plate()
{
    if(!m_need_reposition || m_3mf_ori_plate_objects.empty())
        return;

    GLCanvas3D* view3D = wxGetApp().plater()->get_view3D_canvas3D();
    if(!view3D)
        return;

    Sidebar& sidebar = wxGetApp().plater()->sidebar();

    for (int i = 0; i < m_3mf_ori_plate_objects.size(); ++i) {
        view3D->select_object_from_idx(m_3mf_ori_plate_objects[i]);
        sidebar.obj_list()->update_selections();
        view3D->do_center_plate(i);
    }

    m_3mf_ori_plate_objects.clear();
}

void SimpleModelMgr::reset()
{
    m_need_reposition = false;
    m_3mf_ori_plate_objects.clear();
}

}} // namespace Slic3r::GUI

