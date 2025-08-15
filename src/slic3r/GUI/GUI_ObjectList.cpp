#include "libslic3r/libslic3r.h"
#include "libslic3r/PresetBundle.hpp"
#include "GUI_ObjectList.hpp"
#include "GUI_Factories.hpp"
// #include "GUI_ObjectLayers.hpp"
#include "GUI_App.hpp"
#include "UITour.hpp"
#include "I18N.hpp"
#include "Plater.hpp"
#include "BitmapComboBox.hpp"
#include "MainFrame.hpp"

#include "slic3r/Utils/UndoRedo.hpp"

#include "OptionsGroup.hpp"
#include "Tab.hpp"
#include "wxExtensions.hpp"
#include "libslic3r/Model.hpp"
#include "GLCanvas3D.hpp"
#include "Selection.hpp"
#include "PartPlate.hpp"
#include "format.hpp"
#include "NotificationManager.hpp"
#include "MsgDialog.hpp"
#include "Widgets/ProgressDialog.hpp"
#include "SingleChoiceDialog.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <wx/progdlg.h>
#include <wx/listbook.h>
#include <wx/numformatter.h>
#include <wx/headerctrl.h>
#include <GL/glew.h>

#include "slic3r/Utils/FixModelByWin10.hpp"
#include "libslic3r/Format/bbs_3mf.hpp"
#include "libslic3r/PrintConfig.hpp"

#ifdef __WXMSW__
#include "wx/uiaction.h"
#include <wx/renderer.h>
#endif /* __WXMSW__ */
#include "Gizmos/GLGizmoScale.hpp"

#include "PhysicalPrinterDialog.hpp"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>
#include "slic3r/Config/DispConfig.h"
#include "print_manage/data/DataCenter.hpp"
#include "GLTexture.hpp"
#include "print_manage/Utils.hpp"

#include "libslic3r/ModelVolume.hpp"
#include "libslic3r/ModelInstance.hpp"

namespace Slic3r
{
namespace GUI
{
wxDEFINE_EVENT(EVT_OBJ_LIST_OBJECT_SELECT, SimpleEvent);
wxDEFINE_EVENT(EVT_OBJ_LIST_COLUMN_SELECT, IntEvent);
wxDEFINE_EVENT(EVT_PARTPLATE_LIST_PLATE_SELECT, IntEvent);
wxDEFINE_EVENT(EVT_UPDATE_DEVICES, wxCommandEvent);
wxDEFINE_EVENT(EVT_OPEN_DEVICE_LIST, wxCommandEvent);

static PrinterTechnology printer_technology() { return wxGetApp().preset_bundle->printers.get_selected_preset().printer_technology(); }

static const Selection& scene_selection()
{
    // BBS AssembleView canvas has its own selection
    if (wxGetApp().plater()->get_current_canvas3D()->get_canvas_type() == GLCanvas3D::ECanvasType::CanvasAssembleView)
        return wxGetApp().plater()->get_assmeble_canvas3D()->get_selection();

    return wxGetApp().plater()->get_view3D_canvas3D()->get_selection();
}

// Config from current edited printer preset
static DynamicPrintConfig& printer_config() { return wxGetApp().preset_bundle->printers.get_edited_preset().config; }

static int filaments_count() { return wxGetApp().filaments_cnt(); }

static void take_snapshot(const std::string& snapshot_name)
{
    Plater* plater = wxGetApp().plater();
    if (plater)
        plater->take_snapshot(snapshot_name);
}

class wxRenderer : public wxDelegateRendererNative
{
public:
    wxRenderer() : wxDelegateRendererNative(wxRendererNative::Get()) {}
    virtual void DrawItemSelectionRect(wxWindow* win, wxDC& dc, const wxRect& rect, int flags = 0) wxOVERRIDE
    {
        GetGeneric().DrawItemSelectionRect(win, dc, rect, flags);
    }
};

ObjectList::ObjectList(wxWindow* parent) : wxDataViewCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_MULTIPLE)
{
    wxGetApp().UpdateDVCDarkUI(this, true);

#ifdef __linux__
    // Temporary fix for incorrect dark mode application regarding list item's text color.
    // See: https://github.com/SoftFever/CrealityPrint/issues/2086
    this->SetForegroundColour(*wxBLACK);
#endif

    SetFont(Label::sysFont(13));
#ifdef __WXMSW__
    GenericGetHeader()->SetFont(Label::sysFont(13));
    static auto render = new wxRenderer;
    wxRendererNative::Set(render);
#endif

    // create control
    create_objects_ctrl();

    m_device_list_data.set_object_list(this);

    m_png_textures.reset(new ObjList_Png_Texture_Wrapper);

    //BBS: add part plate related event
    //Bind(EVT_PARTPLATE_LIST_PLATE_SELECT, &ObjectList::on_select_plate, this);

    // describe control behavior
    Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, [this](wxDataViewEvent& event) {
        // detect the current mouse position here, to pass it to list_manipulation() method
        // if we detect it later, the user may have moved the mouse pointer while calculations are performed, and this would mess-up the
        // HitTest() call performed into list_manipulation()
        if (!GetScreenRect().Contains(wxGetMousePosition())) {
            return;
        }
#ifndef __WXOSX__
        const wxPoint mouse_pos = this->get_mouse_position_in_control();
#endif

#ifndef __APPLE__
        // On Windows and Linux:
        // It's not invoked KillFocus event for "temporary" panels (like "Manipulation panel", "Settings", "Layer ranges"),
        // if we change selection in object list.
        // But, if we call SetFocus() for ObjectList it will cause an invoking of a KillFocus event for "temporary" panels
        this->SetFocus();
#else
        // To avoid selection update from SetSelection() and UnselectAll() under osx
        if (m_prevent_list_events)
            return;
#endif // __APPLE__

        /* For multiple selection with pressed SHIFT,
         * event.GetItem() returns value of a first item in selection list
         * instead of real last clicked item.
         * So, let check last selected item in such strange way
         */
#ifdef __WXMSW__
        // Workaround for entering the column editing mode on Windows. Simulate keyboard enter when another column of the active line is selected.
        int new_selected_column = -1;
#endif //__WXMSW__
        if (wxGetKeyState(WXK_SHIFT)) {
            wxDataViewItemArray sels;
            GetSelections(sels);
            if (!sels.empty() && sels.front() == m_last_selected_item)
                m_last_selected_item = sels.back();
            else
                m_last_selected_item = event.GetItem();
        } else {
            wxDataViewItem new_selected_item = event.GetItem();
            // BBS: use wxDataViewCtrl's internal mechanism
#if 0
#ifdef __WXMSW__
			// Workaround for entering the column editing mode on Windows. Simulate keyboard enter when another column of the active line is selected.
		    wxDataViewItem    item;
		    wxDataViewColumn *col;
		    this->HitTest(this->get_mouse_position_in_control(), item, col);
		    new_selected_column = (col == nullptr) ? -1 : (int)col->GetModelColumn();
	        if (new_selected_item == m_last_selected_item && m_last_selected_column != -1 && m_last_selected_column != new_selected_column) {
	        	// Mouse clicked on another column of the active row. Simulate keyboard enter to enter the editing mode of the current column.
	        	wxUIActionSimulator sim;
				sim.Char(WXK_RETURN);
	        }
#endif //__WXMSW__
#endif
            m_last_selected_item = new_selected_item;
        }
#ifdef __WXMSW__
        m_last_selected_column = new_selected_column;
#endif //__WXMSW__

        ObjectDataViewModelNode* sel_node = (ObjectDataViewModelNode*) event.GetItem().GetID();
        if (sel_node && (sel_node->GetType() & ItemType::itPlate)) {
            if (wxGetApp().plater()->is_preview_shown()) {
                wxGetApp().plater()->select_sliced_plate(sel_node->GetPlateIdx());
            } else {
                wxGetApp().plater()->select_plate(sel_node->GetPlateIdx());
            }
            wxGetApp().plater()->deselect_all();
        } else {
            selection_changed();
        }
#ifndef __WXMSW__
        set_tooltip_for_item(this->get_mouse_position_in_control());
#endif //__WXMSW__

#ifndef __WXOSX__
        list_manipulation(mouse_pos);
#endif //__WXOSX__
    });

#ifdef __WXOSX__
    // Key events are not correctly processed by the wxDataViewCtrl on OSX.
    // Our patched wxWidgets process the keyboard accelerators.
    // On the other hand, using accelerators will break in-place editing on Windows & Linux/GTK (there is no in-place editing working on OSX
    // for wxDataViewCtrl for now).
    //    Bind(wxEVT_KEY_DOWN, &ObjectList::OnChar, this);
    {
        // Accelerators
        // 	wxAcceleratorEntry entries[25];
        wxAcceleratorEntry entries[26];
        int                index = 0;
        entries[index++].Set(wxACCEL_CTRL, (int) 'C', wxID_COPY);
        entries[index++].Set(wxACCEL_CTRL, (int) 'X', wxID_CUT);
        entries[index++].Set(wxACCEL_CTRL, (int) 'V', wxID_PASTE);
        entries[index++].Set(wxACCEL_CTRL, (int) 'M', wxID_DUPLICATE);
        entries[index++].Set(wxACCEL_CTRL, (int) 'A', wxID_SELECTALL);
        entries[index++].Set(wxACCEL_CTRL, (int) 'Z', wxID_UNDO);
        entries[index++].Set(wxACCEL_CTRL, (int) 'Y', wxID_REDO);
        entries[index++].Set(wxACCEL_NORMAL, WXK_BACK, wxID_DELETE);
        // entries[index++].Set(wxACCEL_NORMAL, int('+'), wxID_ADD);
        // entries[index++].Set(wxACCEL_NORMAL, WXK_NUMPAD_ADD, wxID_ADD);
        // entries[index++].Set(wxACCEL_NORMAL, int('-'), wxID_REMOVE);
        // entries[index++].Set(wxACCEL_NORMAL, WXK_NUMPAD_SUBTRACT, wxID_REMOVE);
        // entries[index++].Set(wxACCEL_NORMAL, int('p'), wxID_PRINT);

        int numbers_cnt = 0;
        for (auto char_number : {'1', '2', '3', '4', '5', '6', '7', '8', '9'}) {
            entries[index + numbers_cnt].Set(wxACCEL_NORMAL, int(char_number), wxID_LAST + numbers_cnt + 1);
            entries[index + 9 + numbers_cnt].Set(wxACCEL_NORMAL, WXK_NUMPAD0 + numbers_cnt - 1, wxID_LAST + numbers_cnt + 1);
            numbers_cnt++;
            // index++;
        }
        wxAcceleratorTable accel(26, entries);
        SetAcceleratorTable(accel);

        this->Bind(wxEVT_MENU, [this](wxCommandEvent& evt) { this->copy(); }, wxID_COPY);
        this->Bind(wxEVT_MENU, [this](wxCommandEvent& evt) { this->paste(); }, wxID_PASTE);
        this->Bind(wxEVT_MENU, [this](wxCommandEvent& evt) { this->select_item_all_children(); }, wxID_SELECTALL);
        this->Bind(wxEVT_MENU, [this](wxCommandEvent& evt) { this->remove(); }, wxID_DELETE);
        this->Bind(wxEVT_MENU, [this](wxCommandEvent& evt) { this->undo(); }, wxID_UNDO);
        this->Bind(wxEVT_MENU, [this](wxCommandEvent& evt) { this->redo(); }, wxID_REDO);
        this->Bind(wxEVT_MENU, [this](wxCommandEvent& evt) { this->cut(); }, wxID_CUT);
        this->Bind(wxEVT_MENU, [this](wxCommandEvent& evt) { this->clone(); }, wxID_DUPLICATE);
        // this->Bind(wxEVT_MENU, [this](wxCommandEvent &evt) { this->increase_instances();        }, wxID_ADD);
        // this->Bind(wxEVT_MENU, [this](wxCommandEvent &evt) { this->decrease_instances();        }, wxID_REMOVE);
        // this->Bind(wxEVT_MENU, [this](wxCommandEvent &evt) { this->toggle_printable_state();    }, wxID_PRINT);

        for (int i = 1; i < 10; i++)
            this->Bind(
                wxEVT_MENU,
                [this, i](wxCommandEvent& evt) {
                    if (filaments_count() > 1 && i <= filaments_count())
                        this->set_extruder_for_selected_items(i);
                },
                wxID_LAST + i);

        m_accel = accel;
    }
#else //__WXOSX__
    Bind(wxEVT_CHAR, [this](wxKeyEvent& event) { key_event(event); }); // doesn't work on OSX
#endif

#ifdef __WXMSW__
    GetMainWindow()->Bind(wxEVT_MOTION, [this](wxMouseEvent& event) {
        // BBS
        // this->SetFocus();
        set_tooltip_for_item(this->get_mouse_position_in_control());
        event.Skip();
    });
#endif //__WXMSW__

    Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &ObjectList::OnContextMenu, this);

    // BBS
    Bind(wxEVT_DATAVIEW_ITEM_BEGIN_DRAG, &ObjectList::OnBeginDrag, this);
    Bind(wxEVT_DATAVIEW_ITEM_DROP_POSSIBLE, &ObjectList::OnDropPossible, this);
    Bind(wxEVT_DATAVIEW_ITEM_DROP, &ObjectList::OnDrop, this);

    Bind(wxEVT_DATAVIEW_ITEM_START_EDITING, &ObjectList::OnStartEditing, this);
    Bind(wxEVT_DATAVIEW_ITEM_EDITING_STARTED, &ObjectList::OnEditingStarted, this);
    Bind(wxEVT_DATAVIEW_ITEM_EDITING_DONE, &ObjectList::OnEditingDone, this);

    Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &ObjectList::ItemValueChanged, this);

    // BBS: dont need to do extra setting for a deleted object
    // Bind(wxCUSTOMEVT_LAST_VOLUME_IS_DELETED, [this](wxCommandEvent& e)   { last_volume_is_deleted(e.GetInt()); });

    Bind(wxEVT_SIZE, ([this](wxSizeEvent& e) {
             if (m_last_size == this->GetSize()) {
                 e.Skip();
                 return;
             } else {
                 m_last_size = this->GetSize();
             }
#ifdef __WXGTK__
             // On GTK, the EnsureVisible call is postponed to Idle processing (see wxDataViewCtrl::m_ensureVisibleDefered).
             // So the postponed EnsureVisible() call is planned for an item, which may not exist at the Idle processing time, if this
             // wxEVT_SIZE event is succeeded by a delete of the currently active item. We are trying our luck by postponing the wxEVT_SIZE
             // triggered EnsureVisible(), which seems to be working as of now.
             this->CallAfter([this]() { ensure_current_item_visible(); });
#else
             update_name_column_width();

             // BBS
             this->CallAfter([this]() { ensure_current_item_visible(); });
#endif
             e.Skip();
         }));

    Bind(EVT_OBJ_LIST_COLUMN_SELECT, [this](IntEvent& event) {
        int type = event.get_data();

        if (type == 0) {
            show_context_menu(true);
        } else if (type == ObjList_Texture::IM_TEXTURE_NAME::texSupportPainting) {
            GLGizmosManager& gizmos_mgr = wxGetApp().plater()->get_view3D_canvas3D()->get_gizmos_manager();
            if (gizmos_mgr.get_current_type() != GLGizmosManager::EType::FdmSupports)
                gizmos_mgr.open_gizmo(GLGizmosManager::EType::FdmSupports);
            else
                gizmos_mgr.reset_all_states();

        } else if (type == ObjList_Texture::IM_TEXTURE_NAME::texColorPainting) {
            GLGizmosManager& gizmos_mgr = wxGetApp().plater()->get_view3D_canvas3D()->get_gizmos_manager();
            if (gizmos_mgr.get_current_type() != GLGizmosManager::EType::MmuSegmentation)
                gizmos_mgr.open_gizmo(GLGizmosManager::EType::MmuSegmentation);
            else
                gizmos_mgr.reset_all_states();
        }
    });

    Bind(EVT_UPDATE_DEVICES, [this](wxCommandEvent& evt) { 
        // mark to update device list
        m_device_list_dirty_mark = true;
        if (m_device_list_popup_opened) {
            wxGetApp().plater()->get_current_canvas3D()->render();
        }
    });

    Bind(EVT_OPEN_DEVICE_LIST, [this](wxCommandEvent& evt) {
        // open device list
        if (!m_device_list_popup_opened) {
            m_device_list_popup_open_request = true;
            wxGetApp().plater()->set_current_canvas_as_dirty();
            wxGetApp().plater()->get_current_canvas3D()->render();
        }
    });

    m_last_size = this->GetSize();
}

ObjectList::~ObjectList()
{
    if (m_objects_model)
        m_objects_model->DecRef();
    m_png_textures.reset();
}

bool ObjectList::ObjList_Texture::init_svg_texture()
{
    bool        is_dark = wxGetApp().dark_mode();
    std::string svg     = is_dark ? "/images/obj_list_integrate_icon_dark.svg" : "/images/obj_list_integrate_icon_light.svg";
    if (IMTexture::load_from_svg_file(Slic3r::resources_dir() + svg, texCount * 20, 40, m_texture_id)) {
        m_valid = true;
    } else {
        m_valid = false;
    }

    return m_valid;
}

void ObjectList::set_min_height()
{
    // BBS
#if 0
    if (m_items_count == size_t(-1))
        m_items_count = 7;
    int list_min_height = lround(2.25 * (m_items_count + 1) * wxGetApp().em_unit()); // +1 is for height of control header
    this->SetMinSize(wxSize(1, list_min_height));
#endif
}

void ObjectList::update_min_height()
{
    wxDataViewItemArray all_items;
    m_objects_model->GetAllChildren(wxDataViewItem(nullptr), all_items);
    size_t items_cnt = all_items.Count();
#if 0
    if (items_cnt < 7)
        items_cnt = 7;
    else if (items_cnt >= 15)
        items_cnt = 15;
#else
    items_cnt = 8;
#endif

    if (m_items_count == items_cnt)
        return;

    m_items_count = items_cnt;
    set_min_height();
}

void ObjectList::create_objects_ctrl()
{
    // BBS
#if 0
    /* Temporary workaround for the correct behavior of the Scrolled sidebar panel:
     * 1. set a height of the list to some big value
     * 2. change it to the normal(meaningful) min value after first whole Mainframe updating/layouting
     */
    SetMinSize(wxSize(-1, 3000));
#endif

    m_objects_model = new ObjectDataViewModel;
    AssociateModel(m_objects_model);
    m_objects_model->SetAssociatedControl(this);
#if wxUSE_DRAG_AND_DROP && wxUSE_UNICODE
    EnableDragSource(wxDF_UNICODETEXT);
    EnableDropTarget(wxDF_UNICODETEXT);
#endif // wxUSE_DRAG_AND_DROP && wxUSE_UNICODE

    const int em = wxGetApp().em_unit();

    m_columns_width.resize(colCount);
    m_columns_width[colName]         = 22;
    m_columns_width[colPrint]        = 3;
    m_columns_width[colFilament]     = 5;
    m_columns_width[colSupportPaint] = 3;
    m_columns_width[colSinking]      = 3;
    m_columns_width[colColorPaint]   = 3;
    m_columns_width[colEditing]      = 3;

    // column ItemName(Icon+Text) of the view control:
    // And Icon can be consisting of several bitmaps
    BitmapTextRenderer* bmp_text_renderer = new BitmapTextRenderer();
    bmp_text_renderer->set_can_create_editor_ctrl_function([this]() {
        auto type = m_objects_model->GetItemType(GetSelection());
        return type & (itVolume | itObject | itPlate);
    });

    // BBS
    wxDataViewColumn* name_col = new wxDataViewColumn(_L("Name"), bmp_text_renderer, colName, m_columns_width[colName] * em, wxALIGN_LEFT,
                                                      wxDATAVIEW_COL_RESIZABLE);
    // name_col->SetBitmap(create_scaled_bitmap("organize", nullptr, FromDIP(18)));
    AppendColumn(name_col);

    // column PrintableProperty (Icon) of the view control:
    AppendBitmapColumn(" ", colPrint, wxOSX ? wxDATAVIEW_CELL_EDITABLE : wxDATAVIEW_CELL_INERT, 3 * em, wxALIGN_CENTER_HORIZONTAL, 0);

    // column Extruder of the view control:
    BitmapChoiceRenderer* bmp_choice_renderer = new BitmapChoiceRenderer();
    bmp_choice_renderer->set_can_create_editor_ctrl_function(
        [this]() { return m_objects_model->GetItemType(GetSelection()) & (itVolume | itLayer | itObject); });
    bmp_choice_renderer->set_default_extruder_idx([this]() { return m_objects_model->GetDefaultExtruderIdx(GetSelection()); });
    bmp_choice_renderer->set_has_default_extruder([this]() {
        return m_objects_model->GetVolumeType(GetSelection()) == ModelVolumeType::PARAMETER_MODIFIER ||
               m_objects_model->GetItemType(GetSelection()) == itLayer;
    });
    AppendColumn(new wxDataViewColumn(_L("Fila."), bmp_choice_renderer, colFilament, m_columns_width[colFilament] * em,
                                      wxALIGN_CENTER_HORIZONTAL, 0));

    // BBS
    AppendBitmapColumn(" ", colSupportPaint, wxOSX ? wxDATAVIEW_CELL_EDITABLE : wxDATAVIEW_CELL_INERT,
                       m_columns_width[colSupportPaint] * em, wxALIGN_CENTER_HORIZONTAL, 0);
    AppendBitmapColumn(" ", colColorPaint, wxOSX ? wxDATAVIEW_CELL_EDITABLE : wxDATAVIEW_CELL_INERT, m_columns_width[colColorPaint] * em,
                       wxALIGN_CENTER_HORIZONTAL, 0);
    AppendBitmapColumn(" ", colSinking, wxOSX ? wxDATAVIEW_CELL_EDITABLE : wxDATAVIEW_CELL_INERT, m_columns_width[colSinking] * em,
                       wxALIGN_CENTER_HORIZONTAL, 0);

    // column ItemEditing of the view control:
    AppendBitmapColumn(" ", colEditing, wxOSX ? wxDATAVIEW_CELL_EDITABLE : wxDATAVIEW_CELL_INERT, m_columns_width[colEditing] * em,
                       wxALIGN_CENTER_HORIZONTAL, 0);

    // for (int cn = colName; cn < colCount; cn++) {
    //     GetColumn(cn)->SetResizeable(cn == colName);
    // }

    // For some reason under OSX on 4K(5K) monitors in wxDataViewColumn constructor doesn't set width of column.
    // Therefore, force set column width.
    if (wxOSX) {
        for (int cn = colName; cn < colCount; cn++)
            GetColumn(cn)->SetWidth(m_columns_width[cn] * em);
    }
}

void ObjectList::get_selected_item_indexes(int& obj_idx, int& vol_idx, const wxDataViewItem& input_item /* = wxDataViewItem(nullptr)*/)
{
    const wxDataViewItem item = input_item == wxDataViewItem(nullptr) ? GetSelection() : input_item;

    if (!item) {
        obj_idx = vol_idx = -1;
        return;
    }

    const ItemType type = m_objects_model->GetItemType(item);

    obj_idx = type & itObject ? m_objects_model->GetIdByItem(item) :
              type & itVolume ? m_objects_model->GetIdByItem(m_objects_model->GetObject(item)) :
                                -1;

    vol_idx = type & itVolume ? m_objects_model->GetVolumeIdByItem(item) : -1;
}

void ObjectList::get_selection_indexes(std::vector<int>& obj_idxs, std::vector<int>& vol_idxs)
{
    wxDataViewItemArray sels;
    GetSelections(sels);
    if (sels.IsEmpty())
        return;

    if (m_objects_model->GetItemType(sels[0]) & itVolume ||
        (sels.Count() == 1 && m_objects_model->GetItemType(m_objects_model->GetParent(sels[0])) & itVolume)) {
        for (wxDataViewItem item : sels) {
            obj_idxs.emplace_back(m_objects_model->GetIdByItem(m_objects_model->GetObject(item)));

            if (sels.Count() == 1 && m_objects_model->GetItemType(m_objects_model->GetParent(item)) & itVolume)
                item = m_objects_model->GetParent(item);

            assert(m_objects_model->GetItemType(item) & itVolume);
            vol_idxs.emplace_back(m_objects_model->GetVolumeIdByItem(item));
        }
    } else {
        for (wxDataViewItem item : sels) {
            const ItemType type = m_objects_model->GetItemType(item);
            obj_idxs.emplace_back(type & itObject ? m_objects_model->GetIdByItem(item) :
                                                    m_objects_model->GetIdByItem(m_objects_model->GetObject(item)));
        }
    }

    std::sort(obj_idxs.begin(), obj_idxs.end(), std::less<int>());
    obj_idxs.erase(std::unique(obj_idxs.begin(), obj_idxs.end()), obj_idxs.end());
}

int ObjectList::get_repaired_errors_count(const int obj_idx, const int vol_idx /*= -1*/) const
{
    return obj_idx >= 0 ? (*m_objects)[obj_idx]->get_repaired_errors_count(vol_idx) : 0;
}

static std::string get_warning_icon_name(const TriangleMeshStats& stats)
{
    return stats.manifold() ? (stats.repaired() ? "obj_warning" : "") : "obj_warning";
}

MeshErrorsInfo ObjectList::get_mesh_errors_info(const int obj_idx,
                                                const int vol_idx /*= -1*/,
                                                wxString* sidebar_info /*= nullptr*/,
                                                int*      non_manifold_edges) const
{
    if (obj_idx < 0)
        return {{}, {}}; // hide tooltip
    if (m_objects->size() <= obj_idx)
        return {{}, {}}; // hide tooltip
    const TriangleMeshStats& stats = vol_idx == -1 ? (*m_objects)[obj_idx]->get_object_stl_stats() :
                                                     (*m_objects)[obj_idx]->volumes[vol_idx]->mesh().stats();

    if (!stats.repaired() && stats.manifold()) {
        // if (sidebar_info)
        //     *sidebar_info = _L("No errors");
        return {{}, {}}; // hide tooltip
    }

    wxString tooltip, auto_repaired_info, remaining_info;

    // Create tooltip string, if there are errors
    if (stats.repaired()) {
        const int errors   = get_repaired_errors_count(obj_idx, vol_idx);
        auto_repaired_info = format_wxstr(_L_PLURAL("%1$d error repaired", "%1$d errors repaired", errors), errors);
        tooltip += auto_repaired_info + "\n";
    }
    if (!stats.manifold()) {
        remaining_info = format_wxstr(_L_PLURAL("Error: %1$d non-manifold edge.", "Error: %1$d non-manifold edges.", stats.open_edges),
                                      stats.open_edges);

        tooltip += _L("Remaining errors") + ":\n";
        tooltip += "\t" + format_wxstr(_L_PLURAL("%1$d non-manifold edge", "%1$d non-manifold edges", stats.open_edges), stats.open_edges) +
                   "\n";
    }

    if (sidebar_info) {
        *sidebar_info = stats.manifold() ? auto_repaired_info : (remaining_info + (stats.repaired() ? ("\n" + auto_repaired_info) : ""));
    }

    if (non_manifold_edges)
        *non_manifold_edges = stats.open_edges;

    if (is_windows10() && !sidebar_info)
        tooltip += "\n" + _L("Left click the icon to fix model object");

    return {tooltip, get_warning_icon_name(stats)};
}

MeshErrorsInfo ObjectList::get_mesh_errors_info(wxString* sidebar_info /*= nullptr*/, int* non_manifold_edges)
{
    wxDataViewItem item = GetSelection();
    if (!item)
        return {"", ""};

    int obj_idx, vol_idx;
    get_selected_item_indexes(obj_idx, vol_idx);

    if (obj_idx < 0) { // child of ObjectItem is selected
        if (sidebar_info)
            obj_idx = m_objects_model->GetObjectIdByItem(item);
        else
            return {"", ""};
    }

    if (obj_idx < 0) {
        return {"", ""};
    }
    // assert(obj_idx >= 0);

    return get_mesh_errors_info(obj_idx, vol_idx, sidebar_info, non_manifold_edges);
}

void ObjectList::set_tooltip_for_item(const wxPoint& pt)
{
    wxDataViewItem    item;
    wxDataViewColumn* col;
    HitTest(pt, item, col);

    /* GetMainWindow() return window, associated with wxDataViewCtrl.
     * And for this window we should to set tooltips.
     * Just this->SetToolTip(tooltip) => has no effect.
     */

    if (!item || GetSelectedItemsCount() > 1) {
        GetMainWindow()->SetToolTip(""); // hide tooltip
        return;
    }

    wxString                 tooltip = "";
    ObjectDataViewModelNode* node    = (ObjectDataViewModelNode*) item.GetID();

    if (col->GetModelColumn() == (unsigned int) colEditing) {
        if (node->IsActionEnabled())
#ifdef __WXOSX__
            tooltip = _(L("Right button click the icon to drop the object settings"));
#else
            tooltip = _(L("Click the icon to reset all settings of the object"));
#endif //__WXMSW__
    } else if (col->GetModelColumn() == (unsigned int) colPrint)
#ifdef __WXOSX__
        tooltip = _(L("Right button click the icon to drop the object printable property"));
#else
        tooltip = _(L("Click the icon to toggle printable property of the object"));
#endif //__WXMSW__
    // BBS
    else if (col->GetModelColumn() == (unsigned int) colSupportPaint) {
        if (node->HasSupportPainting())
            tooltip = _(L("Click the icon to edit support painting of the object"));

    } else if (col->GetModelColumn() == (unsigned int) colColorPaint) {
        if (node->HasColorPainting())
            tooltip = _(L("Click the icon to edit color painting of the object"));
    } else if (col->GetModelColumn() == (unsigned int) colSinking) {
        if (node->HasSinking())
            tooltip = _(L("Click the icon to shift this object to the bed"));
    } else if (col->GetModelColumn() == (unsigned int) colName && (pt.x >= 2 * wxGetApp().em_unit() && pt.x <= 4 * wxGetApp().em_unit())) {
        if (const ItemType type = m_objects_model->GetItemType(item); type & (itObject | itVolume)) {
            int obj_idx = m_objects_model->GetObjectIdByItem(item);
            int vol_idx = type & itVolume ? m_objects_model->GetVolumeIdByItem(item) : -1;
            tooltip     = get_mesh_errors_info(obj_idx, vol_idx).tooltip;
        }
    }

    GetMainWindow()->SetToolTip(tooltip);
}

int ObjectList::get_selected_obj_idx() const
{
    if (GetSelectedItemsCount() == 1)
        return m_objects_model->GetIdByItem(m_objects_model->GetObject(GetSelection()));

    return -1;
}

ModelConfig& ObjectList::get_item_config(const wxDataViewItem& item) const
{
    static ModelConfig s_empty_config;

    assert(item);
    const ItemType type = m_objects_model->GetItemType(item);

    if (type & itPlate)
        return s_empty_config;

    const int obj_idx = m_objects_model->GetObjectIdByItem(item);
    const int vol_idx = type & itVolume ? m_objects_model->GetVolumeIdByItem(item) : -1;

    assert(obj_idx >= 0 || ((type & itVolume) && vol_idx >= 0));
    return type & itVolume ? (*m_objects)[obj_idx]->volumes[vol_idx]->config :
           type & itLayer  ? (*m_objects)[obj_idx]->layer_config_ranges[m_objects_model->GetLayerRangeByItem(item)] :
                             (*m_objects)[obj_idx]->config;
}

void ObjectList::update_filament_values_for_items(const size_t filaments_count)
{
    for (size_t i = 0; i < m_objects->size(); ++i) {
        wxDataViewItem item = m_objects_model->GetItemById(i);
        if (!item)
            continue;

        auto     object = (*m_objects)[i];
        wxString extruder;
        if (!object->config.has("extruder") || size_t(object->config.extruder()) > filaments_count) {
            extruder = "1";
            object->config.set_key_value("extruder", new ConfigOptionInt(1));
        } else {
            extruder = wxString::Format("%d", object->config.extruder());
        }
        m_objects_model->SetExtruder(extruder, item);

        static const char* keys[] = {"support_filament", "support_interface_filament"};
        for (auto key : keys)
            if (object->config.has(key) && object->config.opt_int(key) > filaments_count)
                object->config.erase(key);

        if (object->volumes.size() > 1) {
            for (size_t id = 0; id < object->volumes.size(); id++) {
                item = m_objects_model->GetItemByVolumeId(i, id);
                if (!item)
                    continue;
                if (!object->volumes[id]->config.has("extruder") || size_t(object->volumes[id]->config.extruder()) > filaments_count) {
                    extruder = wxString::Format("%d", object->config.extruder());
                } else {
                    extruder = wxString::Format("%d", object->volumes[id]->config.extruder());
                }

                m_objects_model->SetExtruder(extruder, item);

                for (auto key : keys)
                    if (object->volumes[id]->config.has(key) && object->volumes[id]->config.opt_int(key) > filaments_count)
                        object->volumes[id]->config.erase(key);
            }
        }
    }

    // BBS
    wxGetApp().plater()->update();
}

void ObjectList::update_filament_values_for_items_when_delete_filament(const size_t filament_id, const int replace_id)
{
    int replace_filament_id = replace_id == -1 ? 1 : (replace_id + 1);
    for (size_t i = 0; i < m_objects->size(); ++i) {
        wxDataViewItem item = m_objects_model->GetItemById(i);
        if (!item)
            continue;

        auto     object = (*m_objects)[i];
        wxString extruder;
        if (!object->config.has("extruder")) {
            extruder = std::to_string(1);
            object->config.set_key_value("extruder", new ConfigOptionInt(1));
        } else if (size_t(object->config.extruder()) == filament_id + 1) {
            extruder = std::to_string(replace_filament_id);
            object->config.set_key_value("extruder", new ConfigOptionInt(replace_filament_id));
        } else {
            int new_extruder = object->config.extruder() > filament_id ? object->config.extruder() - 1 : object->config.extruder();
            extruder         = wxString::Format("%d", new_extruder);
            object->config.set_key_value("extruder", new ConfigOptionInt(new_extruder));
        }
        m_objects_model->SetExtruder(extruder, item);

        static const char* keys[] = {"support_filament", "support_interface_filament"};
        for (auto key : keys) {
            if (object->config.has(key)) {
                if (object->config.opt_int(key) == filament_id + 1)
                    object->config.erase(key);
                else {
                    int new_value = object->config.opt_int(key) > filament_id ? object->config.opt_int(key) - 1 :
                                                                                object->config.opt_int(key);
                    object->config.set_key_value(key, new ConfigOptionInt(new_value));
                }
            }
        }

        // if (object->volumes.size() > 1) {
        for (size_t id = 0; id < object->volumes.size(); id++) {
            item = m_objects_model->GetItemByVolumeId(i, id);
            if (!item)
                continue;

            for (auto key : keys) {
                if (object->volumes[id]->config.has(key)) {
                    if (object->volumes[id]->config.opt_int(key) == filament_id + 1)
                        object->volumes[id]->config.erase(key);
                    else {
                        int new_value = object->volumes[id]->config.opt_int(key) > filament_id ?
                                            object->volumes[id]->config.opt_int(key) - 1 :
                                            object->volumes[id]->config.opt_int(key);
                        object->config.set_key_value(key, new ConfigOptionInt(new_value));
                    }
                }
            }

            if (!object->volumes[id]->config.has("extruder")) {
                continue;
            } else if (size_t(object->volumes[id]->config.extruder()) == filament_id + 1) {
                object->volumes[id]->config.set_key_value("extruder", new ConfigOptionInt(replace_filament_id));
            } else {
                int new_extruder = object->volumes[id]->config.extruder() > filament_id ? object->volumes[id]->config.extruder() - 1 :
                                                                                          object->volumes[id]->config.extruder();
                extruder         = wxString::Format("%d", new_extruder);
                object->volumes[id]->config.set_key_value("extruder", new ConfigOptionInt(new_extruder));
            }

            m_objects_model->SetExtruder(extruder, item);
        }
        //}

        item                                 = m_objects_model->GetItemById(i);
        ObjectDataViewModelNode* object_node = static_cast<ObjectDataViewModelNode*>(item.GetID());
        if (object_node->GetChildCount() == 0)
            continue;

        // update height_range
        for (size_t i = 0; i < object_node->GetChildCount(); i++) {
            ObjectDataViewModelNode* layer_root_node = object_node->GetNthChild(i);
            if (layer_root_node->GetType() != ItemType::itLayerRoot)
                continue;
            for (size_t j = 0; j < layer_root_node->GetChildCount(); j++) {
                ObjectDataViewModelNode* layer_node = layer_root_node->GetNthChild(j);
                auto                     layer_item = wxDataViewItem((void*) layer_root_node->GetNthChild(j));
                if (!layer_item)
                    continue;
                auto l_iter = object->layer_config_ranges.find(layer_node->GetLayerRange());
                if (l_iter != object->layer_config_ranges.end()) {
                    auto& layer_range_item = *(l_iter);
                    if (layer_range_item.second.has("extruder") && layer_range_item.second.option("extruder")->getInt() == filament_id + 1) {
                        int new_extruder = replace_id == -1 ? 0 : (replace_id + 1);
                        extruder         = new_extruder <= 1 ? _(L("default")) : wxString::Format("%d", new_extruder);
                        layer_range_item.second.set("extruder", new_extruder);
                    } else {
                        int layer_filament_id = layer_range_item.second.option("extruder")->getInt();
                        int new_extruder      = layer_filament_id > filament_id ? layer_filament_id - 1 : layer_filament_id;
                        extruder = new_extruder <= 1 ? _(L("default")) : wxString::Format("%d", new_extruder);
                        layer_range_item.second.set("extruder", new_extruder);
                    }
                    m_objects_model->SetExtruder(extruder, layer_item);
                }
            }
        }
    }
    // BBS
    wxGetApp().plater()->update();
}

void ObjectList::update_plate_values_for_items()
{
#ifdef __WXOSX__
    AssociateModel(nullptr);
#endif
    PartPlateList& list = wxGetApp().plater()->get_partplate_list();
    for (size_t i = 0; i < m_objects->size(); ++i) {
        wxDataViewItem item = m_objects_model->GetItemById(i);
        if (!item)
            continue;

        int                      plate_idx       = list.find_instance_belongs(i, 0);
        wxDataViewItem           old_parent      = m_objects_model->GetParent(item);
        ObjectDataViewModelNode* old_parent_node = (ObjectDataViewModelNode*) old_parent.GetID();
        int                      old_plate_idx   = old_parent_node->GetPlateIdx();
        if (plate_idx == old_plate_idx)
            continue;

        // hotfix for wxDataViewCtrl selection not updated after wxDataViewModel::ItemDeleted()
        Unselect(item);

        bool is_old_parent_expanded = IsExpanded(old_parent);
        bool is_expanded            = IsExpanded(item);
        m_objects_model->OnPlateChange(plate_idx, item);
        if (is_old_parent_expanded)
            Expand(old_parent);
        ExpandAncestors(item);
        Expand(item);
        Select(item);
    }
#ifdef __WXOSX__
    AssociateModel(m_objects_model);
#endif
}

// BBS
void ObjectList::update_name_for_items()
{
    m_objects_model->UpdateItemNames();

    wxGetApp().plater()->update();
}

void ObjectList::object_config_options_changed(const ObjectVolumeID& ov_id)
{
    if (ov_id.object == nullptr)
        return;

    ModelObjectPtrs& objects = wxGetApp().model().objects;
    ModelObject*     mo      = ov_id.object;
    ModelVolume*     mv      = ov_id.volume;

    wxDataViewItem obj_item = m_objects_model->GetObjectItem(mo);
    if (mv != nullptr) {
        size_t vol_idx;
        for (vol_idx = 0; vol_idx < mo->volumes.size(); vol_idx++) {
            if (mo->volumes[vol_idx] == mv)
                break;
        }
        assert(vol_idx < mo->volumes.size());

        SettingsFactory::Bundle cat_options = SettingsFactory::get_bundle(&mv->config.get(), false);
        wxDataViewItem          vol_item    = m_objects_model->GetVolumeItem(obj_item, vol_idx);
        if (cat_options.size() > 0) {
            add_settings_item(vol_item, &mv->config.get());
        } else {
            m_objects_model->DeleteSettings(vol_item);
        }
    } else {
        SettingsFactory::Bundle cat_options = SettingsFactory::get_bundle(&mo->config.get(), true);
        if (cat_options.size() > 0) {
            add_settings_item(obj_item, &mo->config.get());
        } else {
            m_objects_model->DeleteSettings(obj_item);
        }
    }
}

void ObjectList::printable_state_changed(const std::vector<ObjectVolumeID>& ov_ids)
{
    std::vector<size_t> obj_idxs;
    for (const ObjectVolumeID ov_id : ov_ids) {
        if (ov_id.object == nullptr)
            continue;

        ModelInstance* mi       = ov_id.object->instances[0];
        wxDataViewItem obj_item = m_objects_model->GetObjectItem(ov_id.object);
        m_objects_model->SetObjectPrintableState(mi->printable ? piPrintable : piUnprintable, obj_item);

        int obj_idx = m_objects_model->GetObjectIdByItem(obj_item);
        obj_idxs.emplace_back(static_cast<size_t>(obj_idx));
    }

    sort(obj_idxs.begin(), obj_idxs.end());
    obj_idxs.erase(unique(obj_idxs.begin(), obj_idxs.end()), obj_idxs.end());

    // update printable state on canvas
    wxGetApp().plater()->get_view3D_canvas3D()->update_instance_printable_state_for_objects(obj_idxs);

    // update scene
    wxGetApp().plater()->update();
}

void ObjectList::assembly_plate_object_name() { m_objects_model->assembly_name(); }

void ObjectList::selected_object(ObjectDataViewModelNode* item)
{
    if (!item) {
        return;
    }
    this->SetFocus();
    select_item(wxDataViewItem(item));
    ensure_current_item_visible();
    selection_changed();
}

void ObjectList::update_objects_list_filament_column(size_t filaments_count)
{
    assert(filaments_count >= 1);

    if (printer_technology() == ptSLA)
        filaments_count = 1;

    m_prevent_update_filament_in_config = true;

    // BBS: update extruder values even when filaments_count is 1, because it may be reduced from value greater than 1
    if (m_objects)
        update_filament_values_for_items(filaments_count);

    update_filament_colors();

    // set show/hide for this column
    set_filament_column_hidden(filaments_count == 1);
    // a workaround for a wrong last column width updating under OSX
    GetColumn(colEditing)->SetWidth(25);

    m_prevent_update_filament_in_config = false;
}
void ObjectList::update_objects_list_filament_column_when_delete_filament(size_t filament_id,
                                                                          size_t filaments_count,
                                                                          int    replace_filament_id)
{
    m_prevent_update_filament_in_config = true;

    // BBS: update extruder values even when filaments_count is 1, because it may be reduced from value greater than 1
    if (m_objects)
        update_filament_values_for_items_when_delete_filament(filament_id, replace_filament_id);

    update_filament_colors();

    // set show/hide for this column
    set_filament_column_hidden(filaments_count == 1);
    // a workaround for a wrong last column width updating under OSX
    GetColumn(colEditing)->SetWidth(25);

    m_prevent_update_filament_in_config = false;
}

void ObjectList::update_filament_colors()
{
    m_objects_model->UpdateColumValues(colFilament);
    // BBS: fix color not refresh
    Refresh();
}

void ObjectList::update_name_column_width() const
{
    wxSize client_size = this->GetClientSize();
    bool   p_vbar      = this->GetParent()->HasScrollbar(wxVERTICAL);
    bool   p_hbar      = this->GetParent()->HasScrollbar(wxHORIZONTAL);

    auto em = em_unit(const_cast<ObjectList*>(this));
    // BBS: walkaround for wxDataViewCtrl::HasScrollbar() does not return correct status
    int others_width = 0;
    for (int cn = colName; cn < colCount; cn++) {
        if (cn != colName) {
            if (!GetColumn(cn)->IsHidden())
                others_width += m_columns_width[cn];
        }
    }

    GetColumn(colName)->SetWidth(client_size.x - (others_width) *em);
}

void ObjectList::set_filament_column_hidden(const bool hide) const
{
    GetColumn(colFilament)->SetHidden(hide);
    update_name_column_width();
}

// BBS
void ObjectList::set_color_paint_hidden(const bool hide) const
{
    GetColumn(colColorPaint)->SetHidden(hide);
    update_name_column_width();
}

void ObjectList::set_support_paint_hidden(const bool hide) const
{
    GetColumn(colSupportPaint)->SetHidden(hide);
    update_name_column_width();
}

void GUI::ObjectList::set_sinking_hidden(const bool hide) const
{
    GetColumn(colSinking)->SetHidden(hide);
    update_name_column_width();
}

void ObjectList::update_filament_in_config(const wxDataViewItem& item)
{
    if (m_prevent_update_filament_in_config)
        return;

    const ItemType item_type = m_objects_model->GetItemType(item);
    if (item_type & itObject) {
        const int obj_idx = m_objects_model->GetIdByItem(item);
        m_config          = &(*m_objects)[obj_idx]->config;
    } else {
        const int obj_idx = m_objects_model->GetIdByItem(m_objects_model->GetObject(item));
        if (item_type & itVolume) {
            const int volume_id = m_objects_model->GetVolumeIdByItem(item);
            if (obj_idx < 0 || volume_id < 0)
                return;
            m_config = &(*m_objects)[obj_idx]->volumes[volume_id]->config;
        } else if (item_type & itLayer)
            m_config = &get_item_config(item);
    }

    if (!m_config)
        return;

    take_snapshot("Change Filament");

    const int extruder = m_objects_model->GetExtruderNumber(item);
    m_config->set_key_value("extruder", new ConfigOptionInt(extruder));

    // BBS
    if (item_type & itObject) {
        const int obj_idx = m_objects_model->GetIdByItem(item);
        for (ModelVolume* mv : (*m_objects)[obj_idx]->volumes) {
            if (mv->config.has("extruder"))
                mv->config.erase("extruder");
        }
    }

    // update scene
    wxGetApp().plater()->update();
}

void ObjectList::update_name_in_model(const wxDataViewItem& item) const
{
    if (m_objects_model->GetItemType(item) & itPlate) {
        std::string    name      = m_objects_model->GetName(item).ToUTF8().data();
        int            plate_idx = -1;
        const ItemType type0     = m_objects_model->GetItemType(item, plate_idx);
        if (plate_idx >= 0) {
            auto plate = wxGetApp().plater()->get_partplate_list().get_plate(plate_idx);
            if (plate->get_plate_name() != name) {
                plate->set_plate_name(name);
            }
            m_objects_model->SetCurSelectedPlateFullName(plate_idx, name);
        }
        return;
    }

    const int obj_idx = m_objects_model->GetObjectIdByItem(item);
    if (obj_idx < 0)
        return;
    const int volume_id = m_objects_model->GetVolumeIdByItem(item);

    take_snapshot(volume_id < 0 ? "Rename Object" : "Rename Part");

    ModelObject* obj = object(obj_idx);
    if (m_objects_model->GetItemType(item) & itObject) {
        std::string name = m_objects_model->GetName(item).ToUTF8().data();
        if (obj->name != name) {
            obj->name = name;
            // if object has just one volume, rename this volume too
            if (obj->volumes.size() == 1)
                obj->volumes[0]->name = obj->name;
            Slic3r::save_object_mesh(*obj);
        }
        return;
    }

    if (volume_id < 0)
        return;
    obj->volumes[volume_id]->name = m_objects_model->GetName(item).ToUTF8().data();
}

void ObjectList::update_name_in_list(int obj_idx, int vol_idx) const
{
    if (obj_idx < 0)
        return;
    wxDataViewItem item = GetSelection();
    if (!item || !(m_objects_model->GetItemType(item) & (itVolume | itObject)))
        return;

    wxString new_name = from_u8(object(obj_idx)->volumes[vol_idx]->name);
    if (new_name.IsEmpty() || m_objects_model->GetName(item) == new_name)
        return;

    m_objects_model->SetName(new_name, item);
}

void ObjectList::selection_changed()
{
    if (m_prevent_list_events)
        return;

    fix_multiselection_conflicts();

    fix_cut_selection();

    // update object selection on Plater
    if (!m_prevent_canvas_selection_update)
        update_selections_on_canvas();

    // to update the toolbar and info sizer
    if (!GetSelection() || m_objects_model->GetItemType(GetSelection()) == itObject) {
        auto event = SimpleEvent(EVT_OBJ_LIST_OBJECT_SELECT);
        event.SetEventObject(this);
        wxPostEvent(this, event);
    }

    if (const wxDataViewItem item = GetSelection()) {
        const ItemType type = m_objects_model->GetItemType(item);
        // to correct visual hints for layers editing on the Scene
        if (type & (itLayer | itLayerRoot)) {
            wxGetApp().obj_layers()->reset_selection();

            if (type & itLayerRoot)
                wxGetApp().plater()->canvas3D()->handle_sidebar_focus_event("", false);
            else {
                wxGetApp().obj_layers()->set_selectable_range(m_objects_model->GetLayerRangeByItem(item));
                wxGetApp().obj_layers()->update_scene_from_editor_selection();
            }
        }
    }

    part_selection_changed();
}

void ObjectList::copy_layers_to_clipboard()
{
    wxDataViewItemArray sel_layers;
    GetSelections(sel_layers);

    const int obj_idx = m_objects_model->GetObjectIdByItem(sel_layers.front());
    if (obj_idx < 0 || (int) m_objects->size() <= obj_idx)
        return;

    const t_layer_config_ranges& ranges       = object(obj_idx)->layer_config_ranges;
    t_layer_config_ranges&       cache_ranges = m_clipboard.get_ranges_cache();

    if (sel_layers.Count() == 1 && m_objects_model->GetItemType(sel_layers.front()) & itLayerRoot) {
        cache_ranges.clear();
        cache_ranges = ranges;
        return;
    }

    for (const auto& layer_item : sel_layers)
        if (m_objects_model->GetItemType(layer_item) & itLayer) {
            auto range = m_objects_model->GetLayerRangeByItem(layer_item);
            auto it    = ranges.find(range);
            if (it != ranges.end())
                cache_ranges[it->first] = it->second;
        }
}

void ObjectList::paste_layers_into_list()
{
    const int              obj_idx      = m_objects_model->GetObjectIdByItem(GetSelection());
    t_layer_config_ranges& cache_ranges = m_clipboard.get_ranges_cache();

    if (obj_idx < 0 || (int) m_objects->size() <= obj_idx || cache_ranges.empty() || printer_technology() == ptSLA)
        return;

    const wxDataViewItem object_item = m_objects_model->GetItemById(obj_idx);
    wxDataViewItem       layers_item = m_objects_model->GetLayerRootItem(object_item);
    if (layers_item)
        m_objects_model->Delete(layers_item);

    t_layer_config_ranges& ranges = object(obj_idx)->layer_config_ranges;

    // and create Layer item(s) according to the layer_config_ranges
    for (const auto& range : cache_ranges)
        ranges.emplace(range);

    layers_item = add_layer_root_item(object_item);

    changed_object(obj_idx);

    select_item(layers_item);
#ifndef __WXOSX__
    selection_changed();
#endif // no __WXOSX__
}

void ObjectList::copy_settings_to_clipboard()
{
    wxDataViewItem item = GetSelection();
    assert(item.IsOk());
    if (m_objects_model->GetItemType(item) & itSettings)
        item = m_objects_model->GetParent(item);

    m_clipboard.get_config_cache() = get_item_config(item).get();
}

void ObjectList::paste_settings_into_list()
{
    wxDataViewItem item = GetSelection();
    assert(item.IsOk());
    if (m_objects_model->GetItemType(item) & itSettings)
        item = m_objects_model->GetParent(item);

    ItemType item_type = m_objects_model->GetItemType(item);
    if (!(item_type & (itObject | itVolume | itLayer)))
        return;

    DynamicPrintConfig& config_cache = m_clipboard.get_config_cache();
    assert(!config_cache.empty());

    auto keys         = config_cache.keys();
    auto part_options = SettingsFactory::get_options(true);

    for (const std::string& opt_key : keys) {
        if (item_type & (itVolume | itLayer) && std::find(part_options.begin(), part_options.end(), opt_key) == part_options.end())
            continue; // we can't to add object specific options for the part's(itVolume | itLayer) config

        const ConfigOption* option = config_cache.option(opt_key);
        if (option)
            m_config->set_key_value(opt_key, option->clone());
    }

    // Add settings item for object/sub-object and show them
    show_settings(add_settings_item(item, &m_config->get()));
}

void ObjectList::paste_volumes_into_list(int obj_idx, const ModelVolumePtrs& volumes)
{
    if ((obj_idx < 0) || ((int) m_objects->size() <= obj_idx))
        return;

    if (volumes.empty())
        return;

    wxDataViewItemArray items = reorder_volumes_and_get_selection(obj_idx, [volumes](const ModelVolume* volume) {
        return std::find(volumes.begin(), volumes.end(), volume) != volumes.end();
    });
    if (items.size() > 1) {
        m_selection_mode     = smVolume;
        m_last_selected_item = wxDataViewItem(nullptr);
    }

    select_items(items);
    selection_changed();

    // BBS: notify partplate the modify
    notify_instance_updated(obj_idx);
}

void ObjectList::paste_objects_into_list(const std::vector<size_t>& object_idxs)
{
    if (object_idxs.empty())
        return;

    wxDataViewItemArray items;
    for (const size_t object : object_idxs) {
        add_object_to_list(object);
        items.Add(m_objects_model->GetItemById(object));
    }

    wxGetApp().plater()->changed_objects(object_idxs);

    select_items(items); // Do not change the geometric selection status of the current plate
    selection_changed();
}

#ifdef __WXOSX__
/*
void ObjectList::OnChar(wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_BACK){
        remove();
    }
    else if (wxGetKeyState(wxKeyCode('A')) && wxGetKeyState(WXK_SHIFT))
        select_item_all_children();

    event.Skip();
}
*/
#endif /* __WXOSX__ */

void ObjectList::OnContextMenu(wxDataViewEvent& evt)
{
    // The mouse position returned by get_mouse_position_in_control() here is the one at the time the mouse button is released (mouse up event)
    wxPoint mouse_pos = this->get_mouse_position_in_control();

    // Do not show the context menu if the user pressed the right mouse button on the 3D scene and released it on the objects list
    GLCanvas3D* canvas           = wxGetApp().plater()->canvas3D();
    bool        evt_context_menu = (canvas != nullptr) ? !canvas->is_mouse_dragging() : true;
    if (!evt_context_menu)
        canvas->mouse_up_cleanup();

    list_manipulation(mouse_pos, evt_context_menu);
}

void ObjectList::list_manipulation(const wxPoint& mouse_pos, bool evt_context_menu /* = false*/)
{
    // Interesting fact: when mouse_pos.x < 0, HitTest(mouse_pos, item, col) returns item = null, but column = last column.
    // So, when mouse was moved to scene immediately after clicking in ObjectList, in the scene will be shown context menu for the Editing column.
    if (mouse_pos.x < 0)
        return;

    wxDataViewItem    item;
    wxDataViewColumn* col = nullptr;
    HitTest(mouse_pos, item, col);

    if (m_extruder_editor)
        m_extruder_editor->Hide();

    /* Note: Under OSX right click doesn't send "selection changed" event.
     * It means that Selection() will be return still previously selected item.
     * Thus under OSX we should force UnselectAll(), when item and col are nullptr,
     * and select new item otherwise.
     */

    // BBS
    // if (!item)
    {
        if (col == nullptr) {
            if (wxOSX && !multiple_selection())
                UnselectAll();
            else if (!evt_context_menu)
                // Case, when last item was deleted and under GTK was called wxEVT_DATAVIEW_SELECTION_CHANGED,
                // which invoked next list_manipulation(false)
                return;
        }

        if (evt_context_menu) {
            show_context_menu(evt_context_menu);
            return;
        }
    }

    if (wxOSX && item && col) {
        wxDataViewItemArray sels;
        GetSelections(sels);
        UnselectAll();
        if (sels.Count() > 1)
            SetSelections(sels);
        else
            Select(item);
    }

    if (col != nullptr) {
        const wxString title   = col->GetTitle();
        ColumnNumber   col_num = (ColumnNumber) col->GetModelColumn();
        if (col_num == colPrint)
            toggle_printable_state();
        else if (col_num == colSupportPaint) {
            ObjectDataViewModelNode* node = (ObjectDataViewModelNode*) item.GetID();
            if (node && node->HasSupportPainting()) {
                GLGizmosManager& gizmos_mgr = wxGetApp().plater()->get_view3D_canvas3D()->get_gizmos_manager();
                if (gizmos_mgr.get_current_type() != GLGizmosManager::EType::FdmSupports)
                    gizmos_mgr.open_gizmo(GLGizmosManager::EType::FdmSupports);
                else
                    gizmos_mgr.reset_all_states();
            }
        } else if (col_num == colColorPaint) {
            if (wxGetApp().plater()->get_current_canvas3D()->get_canvas_type() != GLCanvas3D::CanvasAssembleView) {
                ObjectDataViewModelNode* node = (ObjectDataViewModelNode*) item.GetID();
                if (node && node->HasColorPainting()) {
                    GLGizmosManager& gizmos_mgr = wxGetApp().plater()->get_view3D_canvas3D()->get_gizmos_manager();
                    if (gizmos_mgr.get_current_type() != GLGizmosManager::EType::MmuSegmentation)
                        gizmos_mgr.open_gizmo(GLGizmosManager::EType::MmuSegmentation);
                    else
                        gizmos_mgr.reset_all_states();
                }
            }
        } else if (col_num == colSinking) {
            Plater*     plater = wxGetApp().plater();
            GLCanvas3D* cnv    = plater->canvas3D();
            int         obj_idx, vol_idx;
            get_selected_item_indexes(obj_idx, vol_idx, item);
            if (obj_idx != -1) {
                Plater::TakeSnapshot(plater, "Shift objects to bed");
                (*m_objects)[obj_idx]->ensure_on_bed();
                cnv->reload_scene(true, true);
                update_info_items(obj_idx);
                notify_instance_updated(obj_idx);
            }
        } else if (col_num == colEditing) {
            // show_context_menu(evt_context_menu);
            int obj_idx, vol_idx;

            get_selected_item_indexes(obj_idx, vol_idx, item);
            // wxGetApp().plater()->PopupObjectTable(obj_idx, vol_idx, mouse_pos);
            if (m_objects_model->GetItemType(item) & itPlate)
                dynamic_cast<TabPrintPlate*>(wxGetApp().get_plate_tab())->reset_model_config();
            else if (m_objects_model->GetItemType(item) & itLayer)
                dynamic_cast<TabPrintLayer*>(wxGetApp().get_layer_tab())->reset_model_config();
            else
                dynamic_cast<TabPrintModel*>(wxGetApp().get_model_tab(vol_idx >= 0))->reset_model_config();
        } else if (col_num == colName) {
            if (is_windows10() && m_objects_model->HasWarningIcon(item) && mouse_pos.x > 2 * wxGetApp().em_unit() &&
                mouse_pos.x < 4 * wxGetApp().em_unit())
                fix_through_netfabb();
            else if (evt_context_menu)
                show_context_menu(evt_context_menu); // show context menu for "Name" column too
        }
        // workaround for extruder editing under OSX
        else if (wxOSX && evt_context_menu && col_num == colFilament)
            extruder_editing();
    }

#ifndef __WXMSW__
    GetMainWindow()->SetToolTip(""); // hide tooltip
#endif                               //__WXMSW__
}

void ObjectList::show_context_menu(const bool evt_context_menu)
{
    // BBS Disable menu popup if current canvas is Preview
    if (wxGetApp().plater()->get_current_canvas3D()->get_canvas_type() == GLCanvas3D::ECanvasType::CanvasPreview)
        return;

    // unregister all extra render event(render-fill-in-bed or render-object-clone) when show right click menu
    wxGetApp().plater()->get_current_canvas3D()->unregister_all_extra_render_event();

    wxMenu* menu{nullptr};
    Plater* plater = wxGetApp().plater();

    if (multiple_selection()) {
        if (selected_instances_of_same_object())
            menu = plater->instance_menu();
        else
            menu = plater->multi_selection_menu();
    } else {
        const auto item = GetSelection();
        if (item) {
            const ItemType type = m_objects_model->GetItemType(item);
            if (!(type & (itPlate | itObject | itVolume | itInstance)))
                return;
            if (type & itVolume) {
                int obj_idx, vol_idx;
                get_selected_item_indexes(obj_idx, vol_idx, item);
                if (obj_idx < 0 || vol_idx < 0)
                    return;
                const ModelVolume* volume = object(obj_idx)->volumes[vol_idx];

                menu = volume->is_text() ? plater->text_part_menu() :
			volume->is_svg() ? plater->svg_part_menu() : // ORCA fixes missing "Edit SVG" item for Add/Negative/Modifier SVG objects in object list
                    plater->part_menu();
            } else
                menu = type & itPlate                ? plater->plate_menu() :
                       type & itInstance             ? plater->instance_menu() :
                       printer_technology() == ptFFF ? plater->object_menu() :
                                                       plater->sla_object_menu();
            plater->SetPlateIndexByRightMenuInLeftUI(-1);
            if (type & itPlate) {
                int            plate_idx = -1;
                const ItemType type0     = m_objects_model->GetItemType(item, plate_idx);
                if (plate_idx >= 0) {
                    plater->SetPlateIndexByRightMenuInLeftUI(plate_idx);
                }
            }
        } else if (evt_context_menu)
            menu = plater->default_menu();
    }

    if (menu)
        plater->PopupMenu(menu);
}

void ObjectList::extruder_editing()
{
    wxDataViewItem item = GetSelection();
    if (!item || !(m_objects_model->GetItemType(item) & (itVolume | itObject)))
        return;

    const int column_width = GetColumn(colFilament)->GetWidth() + wxSystemSettings::GetMetric(wxSYS_VSCROLL_X) + 5;

    wxPoint pos  = this->get_mouse_position_in_control();
    wxSize  size = wxSize(column_width, -1);
    pos.x        = GetColumn(colName)->GetWidth() + GetColumn(colPrint)->GetWidth() + 5;
    pos.y -= GetTextExtent("m").y;

    apply_extruder_selector(&m_extruder_editor, this, "1", pos, size);

    m_extruder_editor->SetSelection(m_objects_model->GetExtruderNumber(item));
    m_extruder_editor->Show();

    auto set_extruder = [this]() {
        wxDataViewItem item = GetSelection();
        if (!item)
            return;

        const int selection = m_extruder_editor->GetSelection();
        if (selection >= 0)
            m_objects_model->SetExtruder(m_extruder_editor->GetString(selection), item);

        m_extruder_editor->Hide();
        update_filament_in_config(item);
    };

    // to avoid event propagation to other sidebar items
    m_extruder_editor->Bind(wxEVT_COMBOBOX, [set_extruder](wxCommandEvent& evt) {
        set_extruder();
        evt.StopPropagation();
    });
}

void ObjectList::copy() { wxPostEvent((wxEvtHandler*) wxGetApp().plater()->canvas3D()->get_wxglcanvas(), SimpleEvent(EVT_GLTOOLBAR_COPY)); }

void ObjectList::paste()
{
    wxPostEvent((wxEvtHandler*) wxGetApp().plater()->canvas3D()->get_wxglcanvas(), SimpleEvent(EVT_GLTOOLBAR_PASTE));
}

void ObjectList::cut() { wxPostEvent((wxEvtHandler*) wxGetApp().plater()->canvas3D()->get_wxglcanvas(), SimpleEvent(EVT_GLTOOLBAR_CUT)); }

void ObjectList::clone()
{
    wxPostEvent((wxEvtHandler*) wxGetApp().plater()->canvas3D()->get_wxglcanvas(), SimpleEvent(EVT_GLTOOLBAR_CLONE));
}

bool ObjectList::cut_to_clipboard() { return copy_to_clipboard(); }

bool ObjectList::copy_to_clipboard()
{
    wxDataViewItemArray sels;
    GetSelections(sels);
    if (sels.IsEmpty())
        return false;
    ItemType type = m_objects_model->GetItemType(sels.front());
    if (!(type & (itSettings | itLayer | itLayerRoot))) {
        m_clipboard.reset();
        return false;
    }

    if (type & itSettings)
        copy_settings_to_clipboard();
    if (type & (itLayer | itLayerRoot))
        copy_layers_to_clipboard();

    m_clipboard.set_type(type);
    return true;
}

bool ObjectList::paste_from_clipboard()
{
    if (!(m_clipboard.get_type() & (itSettings | itLayer | itLayerRoot))) {
        m_clipboard.reset();
        return false;
    }

    if (m_clipboard.get_type() & itSettings)
        paste_settings_into_list();
    if (m_clipboard.get_type() & (itLayer | itLayerRoot))
        paste_layers_into_list();

    return true;
}

void ObjectList::undo() { wxGetApp().plater()->undo(); }

void ObjectList::redo() { wxGetApp().plater()->redo(); }

void ObjectList::increase_instances() { wxGetApp().plater()->increase_instances(1); }

void ObjectList::decrease_instances() { wxGetApp().plater()->decrease_instances(1); }

#ifndef __WXOSX__
void ObjectList::key_event(wxKeyEvent& event)
{
    // if (event.GetKeyCode() == WXK_TAB)
    //     Navigate(event.ShiftDown() ? wxNavigationKeyEvent::IsBackward : wxNavigationKeyEvent::IsForward);
    // else
    if (event.GetKeyCode() == WXK_DELETE /*|| event.GetKeyCode() == WXK_BACK*/)
        remove();
    // else if (event.GetKeyCode() == WXK_F5)
    //     wxGetApp().plater()->reload_all_from_disk();
    else if (wxGetKeyState(wxKeyCode('A')) && wxGetKeyState(WXK_CONTROL /*WXK_SHIFT*/))
        select_item_all_children();
    else if (wxGetKeyState(wxKeyCode('C')) && wxGetKeyState(WXK_CONTROL))
        copy();
    else if (wxGetKeyState(wxKeyCode('V')) && wxGetKeyState(WXK_CONTROL))
        paste();
    else if (wxGetKeyState(wxKeyCode('Y')) && wxGetKeyState(WXK_CONTROL))
        redo();
    else if (wxGetKeyState(wxKeyCode('Z')) && wxGetKeyState(WXK_CONTROL))
        undo();
    else if (wxGetKeyState(wxKeyCode('X')) && wxGetKeyState(WXK_CONTROL))
        cut();
    else if (wxGetKeyState(wxKeyCode('K')) && wxGetKeyState(WXK_CONTROL))
        clone();
    // else if (event.GetUnicodeKey() == '+')
    //     increase_instances();
    // else if (event.GetUnicodeKey() == '-')
    //     decrease_instances();
    // else if (event.GetUnicodeKey() == 'p')
    //     toggle_printable_state();
    else if (filaments_count() > 1) {
        std::vector<wxChar> numbers  = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
        wxChar              key_char = event.GetUnicodeKey();
        if (std::find(numbers.begin(), numbers.end(), key_char) != numbers.end()) {
            long extruder_number;
            if (wxNumberFormatter::FromString(wxString(key_char), &extruder_number) && filaments_count() >= extruder_number)
                set_extruder_for_selected_items(int(extruder_number));
        } else
            event.Skip();
    } else
        event.Skip();
}
#endif /* __WXOSX__ */

void ObjectList::OnBeginDrag(wxDataViewEvent& event)
{
    const bool mult_sel = multiple_selection();
    if (mult_sel) {
        event.Veto();
        return;
    }

    const wxDataViewItem item(event.GetItem());
    const ItemType&      type = m_objects_model->GetItemType(item);
    if (!(type & (itVolume | itObject))) {
        event.Veto();
        return;
    }

    if (type & itObject) {
        m_dragged_data.init(m_objects_model->GetIdByItem(item), type);
    } else if (type & itVolume) {
        m_dragged_data.init(m_objects_model->GetObjectIdByItem(item), m_objects_model->GetVolumeIdByItem(item), type);
    }
#if 0
    if ((mult_sel && !selected_instances_of_same_object()) ||
        (!mult_sel && (GetSelection() != item)) ) {
        event.Veto();
        return;
    }

    if (!(type & (itVolume | itObject | itInstance))) {
        event.Veto();
        return;
    }

    if (mult_sel)
    {
        m_dragged_data.init(m_objects_model->GetObjectIdByItem(item),type);
        std::set<int>& sub_obj_idxs = m_dragged_data.inst_idxs();
        wxDataViewItemArray sels;
        GetSelections(sels);
        for (auto sel : sels )
            sub_obj_idxs.insert(m_objects_model->GetInstanceIdByItem(sel));
    }
    else if (type & itObject)
        m_dragged_data.init(m_objects_model->GetIdByItem(item), type);
    else
        m_dragged_data.init(m_objects_model->GetObjectIdByItem(item),
                            type&itVolume ? m_objects_model->GetVolumeIdByItem(item) :
                                        m_objects_model->GetInstanceIdByItem(item),
                            type);
#endif

    /* Under MSW or OSX, DnD moves an item to the place of another selected item
     * But under GTK, DnD moves an item between another two items.
     * And as a result - call EVT_CHANGE_SELECTION to unselect all items.
     * To prevent such behavior use m_prevent_list_events
     **/
    m_prevent_list_events = true; // it's needed for GTK

    /* Under GTK, DnD requires to the wxTextDataObject been initialized with some valid value,
     * so set some nonempty string
     */
    wxTextDataObject* obj = new wxTextDataObject;
    obj->SetText("Some text"); // it's needed for GTK

    event.SetDataObject(obj);
    event.SetDragFlags(wxDrag_DefaultMove); // allows both copy and move;
}

bool ObjectList::can_drop(const wxDataViewItem& item, int& src_obj_id, int& src_plate, int& dest_obj_id, int& dest_plate) const
{
    if (!item.IsOk() || (m_objects_model->GetItemType(item) != m_dragged_data.type()) || !(m_dragged_data.type() & (itVolume | itObject)))
        return false;

    if (m_dragged_data.type() & itObject) {
        int            from_obj_id    = m_dragged_data.obj_idx();
        int            to_obj_id      = m_objects_model->GetIdByItem(item);
        PartPlateList& partplate_list = wxGetApp().plater()->get_partplate_list();

        int from_plate = partplate_list.find_instance(from_obj_id, 0);
        if (from_plate == -1)
            return false;
        int to_plate = partplate_list.find_instance(to_obj_id, 0);
        if ((to_plate == -1) || (from_plate != to_plate))
            return false;

        src_obj_id  = from_obj_id;
        dest_obj_id = to_obj_id;
        src_plate   = from_plate;
        dest_plate  = to_plate;

        // move instance(s) or object on "empty place" of ObjectList
        // if ( (m_dragged_data.type() & (itInstance | itObject)) && !item.IsOk() )
        //    return true;
    } else if (m_dragged_data.type() & itVolume) { // move volumes inside one object only
        if (m_dragged_data.obj_idx() != m_objects_model->GetObjectIdByItem(item))
            return false;
        wxDataViewItem dragged_item = m_objects_model->GetItemByVolumeId(m_dragged_data.obj_idx(), m_dragged_data.sub_obj_idx());
        if (!dragged_item)
            return false;
        ModelVolumeType item_v_type         = m_objects_model->GetVolumeType(item);
        ModelVolumeType dragged_item_v_type = m_objects_model->GetVolumeType(dragged_item);

        if (dragged_item_v_type == item_v_type && dragged_item_v_type != ModelVolumeType::MODEL_PART)
            return true;
        if ((dragged_item_v_type != item_v_type) ||          // we can't reorder volumes outside of types
            item_v_type >= ModelVolumeType::SUPPORT_BLOCKER) // support blockers/enforcers can't change its place
            return false;

        bool  only_one_solid_part = true;
        auto& volumes             = (*m_objects)[m_dragged_data.obj_idx()]->volumes;
        for (size_t cnt, id = cnt = 0; id < volumes.size() && cnt < 2; id++)
            if (volumes[id]->type() == ModelVolumeType::MODEL_PART) {
                if (++cnt > 1)
                    only_one_solid_part = false;
            }

        if (dragged_item_v_type == ModelVolumeType::MODEL_PART) {
            if (only_one_solid_part)
                return false;
            return (m_objects_model->GetVolumeIdByItem(item) == 0 ||
                    (m_dragged_data.sub_obj_idx() == 0 && volumes[1]->type() == ModelVolumeType::MODEL_PART) ||
                    (m_dragged_data.sub_obj_idx() != 0 && volumes[0]->type() == ModelVolumeType::MODEL_PART));
        }
        if ((dragged_item_v_type == ModelVolumeType::NEGATIVE_VOLUME || dragged_item_v_type == ModelVolumeType::PARAMETER_MODIFIER)) {
            if (only_one_solid_part)
                return false;
            return m_objects_model->GetVolumeIdByItem(item) != 0;
        }
        return false;
    }
    return true;
}

void ObjectList::OnDropPossible(wxDataViewEvent& event)
{
    const wxDataViewItem& item = event.GetItem();

    int src_obj_id, src_plate, dest_obj_id, dest_plate;
    if (!can_drop(item, src_obj_id, src_plate, dest_obj_id, dest_plate)) {
        event.Veto();
        m_prevent_list_events = false;
    }
}

void ObjectList::OnDrop(wxDataViewEvent& event)
{
    const wxDataViewItem& item = event.GetItem();

    int src_obj_id, src_plate, dest_obj_id, dest_plate;
    if (!can_drop(item, src_obj_id, src_plate, dest_obj_id, dest_plate)) {
        event.Veto();
        m_dragged_data.clear();
        return;
    }

    // #if 1
    take_snapshot("Object order changed");

    if (m_dragged_data.type() & itObject) {
        int            delta          = dest_obj_id < src_obj_id ? -1 : 1;
        PartPlateList& partplate_list = wxGetApp().plater()->get_partplate_list();
        /*int cnt = 0, cur_id = src_obj_id, next_id, total = abs(src_obj_id - dest_obj_id);
        //for (cur_id = src_obj_id; cnt < total; id += delta, cnt++)
        next_id = src_obj_id + delta;
        while (cnt < total)
        {
            int cur_plate = partplate_list.find_instance(next_id, 0);
            if (cur_plate != src_plate) {
                cnt ++;
                next_id += delta;
                continue;
            }
            std::swap((*m_objects)[cur_id], (*m_objects)[next_id]);
            cur_id = next_id;
            cnt ++;
            next_id += delta;
        }*/

        int cnt = 0;
        for (int id = src_obj_id; cnt < abs(src_obj_id - dest_obj_id); id += delta, cnt++)
            std::swap((*m_objects)[id], (*m_objects)[id + delta]);

        select_item(m_objects_model->ReorganizeObjects(src_obj_id, dest_obj_id));

        partplate_list.reload_all_objects(false, src_plate);
        changed_object(src_obj_id);
    } else if (m_dragged_data.type() & itVolume) {
        int from_volume_id = m_dragged_data.sub_obj_idx();
        int to_volume_id   = m_objects_model->GetVolumeIdByItem(item);
        int delta          = to_volume_id < from_volume_id ? -1 : 1;

        auto& volumes = (*m_objects)[m_dragged_data.obj_idx()]->volumes;

        int cnt = 0;
        for (int id = from_volume_id; cnt < abs(from_volume_id - to_volume_id); id += delta, cnt++)
            std::swap(volumes[id], volumes[id + delta]);

        select_item(m_objects_model->ReorganizeChildren(from_volume_id, to_volume_id, m_objects_model->GetParent(item)));

        changed_object(m_dragged_data.obj_idx());
    }

    m_dragged_data.clear();

    wxGetApp().plater()->set_current_canvas_as_dirty();
}

void ObjectList::add_category_to_settings_from_selection(const std::vector<std::pair<std::string, bool>>& category_options,
                                                         wxDataViewItem                                   item)
{
    if (category_options.empty())
        return;

    const ItemType item_type = m_objects_model->GetItemType(item);

    if (!m_config)
        m_config = &get_item_config(item);

    assert(m_config);
    auto opt_keys = m_config->keys();

    const std::string snapshot_text = item_type & itLayer  ? "Layer setting added" :
                                      item_type & itVolume ? "Part setting added" :
                                                             "Object setting added";
    take_snapshot(snapshot_text);

    const DynamicPrintConfig& from_config = printer_technology() == ptFFF ? wxGetApp().preset_bundle->prints.get_edited_preset().config :
                                                                            wxGetApp().preset_bundle->sla_prints.get_edited_preset().config;

    for (auto& opt : category_options) {
        auto& opt_key = opt.first;
        if (find(opt_keys.begin(), opt_keys.end(), opt_key) != opt_keys.end() && !opt.second)
            m_config->erase(opt_key);

        if (find(opt_keys.begin(), opt_keys.end(), opt_key) == opt_keys.end() && opt.second) {
            const ConfigOption* option = from_config.option(opt_key);
            if (!option) {
                // if current option doesn't exist in prints.get_edited_preset(),
                // get it from default config values
                option = DynamicPrintConfig::new_from_defaults_keys({opt_key})->option(opt_key);
            }
            m_config->set_key_value(opt_key, option->clone());
        }
    }

    // Add settings item for object/sub-object and show them
    if (!(item_type & (itPlate | itObject | itVolume | itLayer)))
        item = m_objects_model->GetObject(item);
    show_settings(add_settings_item(item, &m_config->get()));
}

void ObjectList::add_category_to_settings_from_frequent(const std::vector<std::string>& options, wxDataViewItem item)
{
    const ItemType item_type = m_objects_model->GetItemType(item);

    if (!m_config)
        m_config = &get_item_config(item);

    assert(m_config);
    auto opt_keys = m_config->keys();

    const std::string snapshot_text = item_type & itLayer  ? "Height range settings added" :
                                      item_type & itVolume ? "Part settings added" :
                                                             "Object settings added";
    take_snapshot(snapshot_text);

    const DynamicPrintConfig& from_config = wxGetApp().preset_bundle->prints.get_edited_preset().config;
    for (auto& opt_key : options) {
        if (find(opt_keys.begin(), opt_keys.end(), opt_key) == opt_keys.end()) {
            const ConfigOption* option = from_config.option(opt_key);
            if (!option) {
                // if current option doesn't exist in prints.get_edited_preset(),
                // get it from default config values
                option = DynamicPrintConfig::new_from_defaults_keys({opt_key})->option(opt_key);
            }
            m_config->set_key_value(opt_key, option->clone());
        }
    }

    // Add settings item for object/sub-object and show them
    if (!(item_type & (itPlate | itObject | itVolume | itLayer)))
        item = m_objects_model->GetObject(item);

    show_settings(add_settings_item(item, &m_config->get()));
}

void ObjectList::show_settings(const wxDataViewItem settings_item)
{
    if (!settings_item)
        return;

    select_item(settings_item);

    // update object selection on Plater
    if (!m_prevent_canvas_selection_update)
        update_selections_on_canvas();
}

bool ObjectList::is_instance_or_object_selected()
{
    const Selection& selection = scene_selection();
    return selection.is_single_full_instance() || selection.is_single_full_object();
}

void ObjectList::load_subobject(ModelVolumeType type, bool from_galery /* = false*/)
{
    wxDataViewItem item = GetSelection();
    // we can add volumes for Object or Instance
    if (!item || !(m_objects_model->GetItemType(item) & (itObject | itInstance)))
        return;
    const int obj_idx = m_objects_model->GetObjectIdByItem(item);

    if (obj_idx < 0)
        return;

    // Get object item, if Instance is selected
    if (m_objects_model->GetItemType(item) & itInstance)
        item = m_objects_model->GetItemById(obj_idx);

    wxArrayString input_files;
    /*if (from_galery) {
        GalleryDialog dlg(this);
        if (dlg.ShowModal() != wxID_CLOSE)
            dlg.get_input_files(input_files);
    }
    else*/
    wxGetApp().import_model(wxGetApp().tab_panel()->GetPage(0), input_files);

    if (input_files.IsEmpty())
        return;

    take_snapshot((type == ModelVolumeType::MODEL_PART) ? "Load Part" : "Load Modifier");

    std::vector<ModelVolume*> volumes;
    // ! ysFIXME - delete commented code after testing and rename "load_modifier" to something common
    /*
    if (type == ModelVolumeType::MODEL_PART)
        load_part(*(*m_objects)[obj_idx], volumes, type, from_galery);
    else*/
    load_modifier(input_files, *(*m_objects)[obj_idx], volumes, type, from_galery);

    if (volumes.empty())
        return;

    wxDataViewItemArray items = reorder_volumes_and_get_selection(obj_idx, [volumes](const ModelVolume* volume) {
        return std::find(volumes.begin(), volumes.end(), volume) != volumes.end();
    });

    if (type == ModelVolumeType::MODEL_PART)
        // update printable state on canvas
        wxGetApp().plater()->get_view3D_canvas3D()->update_instance_printable_state_for_object((size_t) obj_idx);

    if (items.size() > 1) {
        m_selection_mode     = smVolume;
        m_last_selected_item = wxDataViewItem(nullptr);
    }
    select_items(items);

    selection_changed();

    // BBS: notify partplate the modify
    notify_instance_updated(obj_idx);
}
/*
void ObjectList::load_part(ModelObject& model_object, std::vector<ModelVolume*>& added_volumes, ModelVolumeType type, bool from_galery = false)
{
    if (type != ModelVolumeType::MODEL_PART)
        return;

    wxWindow* parent = wxGetApp().tab_panel()->GetPage(0);

    wxArrayString input_files;

    if (from_galery) {
        GalleryDialog dlg(this);
        if (dlg.ShowModal() == wxID_CLOSE)
            return;
        dlg.get_input_files(input_files);
        if (input_files.IsEmpty())
            return;
    }
    else
        wxGetApp().import_model(parent, input_files);

    ProgressDialog dlg(_L("Loading") + dots, "", 100, wxGetApp().mainframe wxPD_AUTO_HIDE);
    wxBusyCursor busy;

    for (size_t i = 0; i < input_files.size(); ++i) {
        std::string input_file = input_files.Item(i).ToUTF8().data();

        dlg.Update(static_cast<int>(100.0f * static_cast<float>(i) / static_cast<float>(input_files.size())),
            _L("Loading file") + ": " + from_path(boost::filesystem::path(input_file).filename()));
        dlg.Fit();

        Model model;
        try {
            model = Model::read_from_file(input_file);
        }
        catch (std::exception &e) {
            auto msg = _L("Error!") + " " + input_file + " : " + e.what() + ".";
            show_error(parent, msg);
            exit(1);
        }

        for (auto object : model.objects) {
            Vec3d delta = Vec3d::Zero();
            if (model_object.origin_translation != Vec3d::Zero()) {
                object->center_around_origin();
                delta = model_object.origin_translation - object->origin_translation;
            }
            for (auto volume : object->volumes) {
                volume->translate(delta);
                auto new_volume = model_object.add_volume(*volume, type);
                new_volume->name = boost::filesystem::path(input_file).filename().string();
                // set a default extruder value, since user can't add it manually
                // BBS
                new_volume->config.set_key_value("extruder", new ConfigOptionInt(1));

                added_volumes.push_back(new_volume);
            }
        }
    }
}
*/
void ObjectList::load_modifier(const wxArrayString&       input_files,
                               ModelObject&               model_object,
                               std::vector<ModelVolume*>& added_volumes,
                               ModelVolumeType            type,
                               bool                       from_galery)
{
    // ! ysFIXME - delete commented code after testing and rename "load_modifier" to something common
    // if (type == ModelVolumeType::MODEL_PART)
    //    return;

    wxWindow* parent = wxGetApp().tab_panel()->GetPage(0);

    ProgressDialog dlg(_L("Loading") + dots, "", 100, wxGetApp().mainframe, wxPD_AUTO_HIDE);

    wxBusyCursor busy;

    const int obj_idx = get_selected_obj_idx();
    if (obj_idx < 0)
        return;

    const Selection& selection = scene_selection();
    assert(obj_idx == selection.get_object_idx());

    /** Any changes of the Object's composition is duplicated for all Object's Instances
     * So, It's enough to take a bounding box of a first selected Instance and calculate Part(generic_subobject) position
     */
    int instance_idx = *selection.get_instance_idxs().begin();
    assert(instance_idx != -1);
    if (instance_idx == -1)
        return;

    // Bounding box of the selected instance in world coordinate system including the translation, without modifiers.
    const BoundingBoxf3 instance_bb = model_object.instance_bounding_box(instance_idx);

    // First (any) GLVolume of the selected instance. They all share the same instance matrix.
    const GLVolume*                v                  = selection.get_first_volume();
    const Geometry::Transformation inst_transform     = v->get_instance_transformation();
    const Transform3d              inv_inst_transform = inst_transform.get_matrix_no_offset().inverse();
    const Vec3d                    instance_offset    = v->get_instance_offset();

    for (size_t i = 0; i < input_files.size(); ++i) {
        const std::string input_file = input_files.Item(i).ToUTF8().data();

        dlg.Update(static_cast<int>(100.0f * static_cast<float>(i) / static_cast<float>(input_files.size())),
                   _L("Loading file") + ": " + from_path(boost::filesystem::path(input_file).filename()));
        dlg.Fit();

        Model model;
        try {
            model = Model::read_from_file(input_file, nullptr, nullptr, LoadStrategy::LoadModel);
        } catch (std::exception& e) {
            // auto msg = _L("Error!") + " " + input_file + " : " + e.what() + ".";
            auto msg = _L("Error!") + " " + _L("Failed to get the model data in the current file.");
            show_error(parent, msg);
            return;
        }

        if (from_galery)
            model.center_instances_around_point(Vec2d::Zero());
        else {
            for (auto object : model.objects) {
                if (model_object.origin_translation != Vec3d::Zero()) {
                    object->center_around_origin();
                    const Vec3d delta = model_object.origin_translation - object->origin_translation;
                    for (auto volume : object->volumes) {
                        volume->translate(delta);
                    }
                }
            }
        }

        model.add_default_instances();
        TriangleMesh mesh = model.mesh();
        // Mesh will be centered when loading.
        ModelVolume* new_volume = model_object.add_volume(std::move(mesh), type);
        new_volume->name        = boost::filesystem::path(input_file).filename().string();

        // adjust offset as prusaslicer ObjectList::load_from_files does (works) instead of BBS method
        //// BBS: object_mesh.get_init_shift() keep the relative position
        // TriangleMesh object_mesh = model_object.volumes[0]->mesh();
        // new_volume->set_offset(new_volume->mesh().get_init_shift() - object_mesh.get_init_shift());

        // set a default extruder value, since user can't add it manually
        // BBS
        int extruder_id = 0;
        if (new_volume->type() == ModelVolumeType::MODEL_PART && model_object.config.has("extruder"))
            extruder_id = model_object.config.opt_int("extruder");
        new_volume->config.set_key_value("extruder", new ConfigOptionInt(extruder_id));
        // update source data
        new_volume->source.input_file = input_file;
        new_volume->source.object_idx = obj_idx;
        new_volume->source.volume_idx = int(model_object.volumes.size()) - 1;
        if (model.objects.size() == 1 && model.objects.front()->volumes.size() == 1)
            new_volume->source.mesh_offset = model.objects.front()->volumes.front()->source.mesh_offset;

        if (from_galery) {
            // Transform the new modifier to be aligned with the print bed.
            const BoundingBoxf3 mesh_bb = new_volume->mesh().bounding_box();
            new_volume->set_transformation(Geometry::Transformation::volume_to_bed_transformation(inst_transform, mesh_bb));
            // Set the modifier position.
            // Translate the new modifier to be pickable: move to the left front corner of the instance's bounding box, lift to print bed.
            const Vec3d offset = Vec3d(instance_bb.max.x(), instance_bb.min.y(), instance_bb.min.z()) + 0.5 * mesh_bb.size() -
                                 instance_offset;
            new_volume->set_offset(inv_inst_transform * offset);
        } else
            new_volume->set_offset(new_volume->source.mesh_offset - model_object.volumes.front()->source.mesh_offset);

        added_volumes.push_back(new_volume);
    }
}

static TriangleMesh create_mesh(const std::string& type_name, const BoundingBoxf3& bb)
{
    const double side = wxGetApp().plater()->canvas3D()->get_size_proportional_to_max_bed_size(0.1);

    TriangleMesh mesh;
    if (type_name == "Cube")
        // Sitting on the print bed, left front front corner at (0, 0).
        mesh = TriangleMesh(its_make_cube(20.0f, 20.0f, 20.0f));
    else if (type_name == "Cylinder")
        // Centered around 0, sitting on the print bed.
        // The cylinder has the same volume as the box above.
        mesh = TriangleMesh(its_make_cylinder(20.0f, 30.0f));
    else if (type_name == "Sphere")
        // Centered around 0, half the sphere below the print bed, half above.
        // The sphere has the same volume as the box above.
        mesh = TriangleMesh(its_make_sphere(20.0f, PI / 18));
    else if (type_name == "Slab")
        // Sitting on the print bed, left front front corner at (0, 0).
        mesh = TriangleMesh(its_make_cube(bb.size().x() * 1.5, bb.size().y() * 1.5, bb.size().z() * 0.5));
    else if (type_name == "Cone")
        mesh = TriangleMesh(its_make_cone(20.0f, 30.0f));
    else if (type_name == "Disc")
        // mesh.ReadSTLFile((Slic3r::resources_dir() + "/creality_models/helper_disk.stl").c_str(), true, nullptr);
        mesh = TriangleMesh(its_make_cylinder(5.0f, 0.3f));
    else if (type_name == "Torus")
        mesh.ReadSTLFile((Slic3r::resources_dir() + "/creality_models/torus.stl").c_str(), true, nullptr);
    else if (type_name == "Prism")
        mesh = TriangleMesh(its_make_cylinder(20.0f, 30.0f, 2.0 * PI / 3.0));
    else if (type_name == "Truncated Cone")
        mesh = TriangleMesh(its_make_frustum(20.0f, 30.0f));
    else if (type_name == "Pyramid")
        mesh = TriangleMesh(its_make_cone(20.0f, 30.0f, 0.50 * PI));

    return TriangleMesh(mesh);
}

void ObjectList::load_generic_subobject(const std::string& type_name, const ModelVolumeType type)
{
    // BBS: single snapshot
    Plater::SingleSnapshot single(wxGetApp().plater());

    if (type == ModelVolumeType::INVALID) {
        load_shape_object(type_name);
        return;
    }

    const int obj_idx = get_selected_obj_idx();
    if (obj_idx < 0)
        return;

    const Selection& selection = scene_selection();
    assert(obj_idx == selection.get_object_idx());

    /** Any changes of the Object's composition is duplicated for all Object's Instances
     * So, It's enough to take a bounding box of a first selected Instance and calculate Part(generic_subobject) position
     */
    int instance_idx = *selection.get_instance_idxs().begin();
    assert(instance_idx != -1);
    if (instance_idx == -1)
        return;

    take_snapshot("Add primitive");

    // Selected object
    ModelObject& model_object = *(*m_objects)[obj_idx];
    // Bounding box of the selected instance in world coordinate system including the translation, without modifiers.
    BoundingBoxf3 instance_bb = model_object.instance_bounding_box(instance_idx);

    TriangleMesh mesh = create_mesh(type_name, instance_bb);

    // Mesh will be centered when loading.
    ModelVolume* new_volume = model_object.add_volume(std::move(mesh), type);

    // First (any) GLVolume of the selected instance. They all share the same instance matrix.
    const GLVolume* v = selection.get_first_volume();
    // Transform the new modifier to be aligned with the print bed.
    new_volume->set_transformation(v->get_instance_transformation().get_matrix_no_offset().inverse());
    const BoundingBoxf3 mesh_bb = new_volume->mesh().bounding_box();
    // Set the modifier position.
    auto offset =
        (type_name == "Slab") ?
            // Slab: Lift to print bed
            Vec3d(0., 0., 0.5 * mesh_bb.size().z() + instance_bb.min.z() - v->get_instance_offset().z()) :
            // Translate the new modifier to be pickable: move to the left front corner of the instance's bounding box, lift to print bed.
            Vec3d(instance_bb.max.x(), instance_bb.min.y(), instance_bb.min.z()) + 0.5 * mesh_bb.size() - v->get_instance_offset();
    new_volume->set_offset(v->get_instance_transformation().get_matrix_no_offset().inverse() * offset);

    // BBS: backup
    Slic3r::save_object_mesh(model_object);

    const wxString name = _L("Generic") + "-" + _(type_name);
    new_volume->name    = into_u8(name);

    // set a default extruder value, since user can't add it manually
    // BBS
    int extruder_id = 0;
    if (new_volume->type() == ModelVolumeType::MODEL_PART && model_object.config.has("extruder"))
        extruder_id = model_object.config.opt_int("extruder");
    new_volume->config.set_key_value("extruder", new ConfigOptionInt(extruder_id));
    new_volume->source.is_from_builtin_objects = true;

    select_item([this, obj_idx, new_volume]() {
        wxDataViewItem sel_item;

        wxDataViewItemArray items = reorder_volumes_and_get_selection(obj_idx, [new_volume](const ModelVolume* volume) {
            return volume == new_volume;
        });
        if (!items.IsEmpty())
            sel_item = items.front();

        return sel_item;
    });
    if (type == ModelVolumeType::MODEL_PART)
        // update printable state on canvas
        wxGetApp().plater()->get_view3D_canvas3D()->update_instance_printable_state_for_object((size_t) obj_idx);

    // apply the instance transform to all volumes and reset instance transform except the offset
    apply_object_instance_transfrom_to_all_volumes(&model_object);

    selection_changed();

    // BBS: notify partplate the modify
    notify_instance_updated(obj_idx);

    // BBS Switch to Objects List after add a modifier
    wxGetApp().params_panel()->switch_to_object(true);

    // Show Dialog
    if (wxGetApp().app_config->get("do_not_show_modifer_tips").empty()) {
        TipsDialog dlg(wxGetApp().mainframe, _L("Add Modifier"), _L("Switch to per-object setting mode to edit modifier settings."),
                       "do_not_show_modifer_tips");
        dlg.ShowModal();
    }
}

void ObjectList::switch_to_object_process()
{
    wxGetApp().params_panel()->switch_to_object(true);

    // Show Dialog
    if (wxGetApp().app_config->get("do_not_show_object_process_tips").empty()) {
        TipsDialog dlg(wxGetApp().mainframe, _L("Edit Process Settings"),
                       _L("Switch to per-object setting mode to edit process settings of selected objects."),
                       "do_not_show_object_process_tips");
        dlg.ShowModal();
    }
}

void ObjectList::load_shape_object(const std::string& type_name)
{
    const Selection& selection = wxGetApp().plater()->canvas3D()->get_selection();
    // assert(selection.get_object_idx() == -1); // Add nothing is something is selected on 3DScene
    //     if (selection.get_object_idx() != -1) // Add new geometry without affecting the state of existing geometry
    //         return;

    const int obj_idx = m_objects->size();
    if (obj_idx < 0)
        return;

    Plater::TakeSnapshot snapshot(wxGetApp().plater(), "Add Primitive");

    // Create mesh
    BoundingBoxf3 bb;
    TriangleMesh  mesh = create_mesh(type_name, bb);
    // BBS: remove "Shape" prefix
    load_mesh_object(mesh, _(type_name));
    wxGetApp().mainframe->update_title();
}

void ObjectList::load_mesh_object(const TriangleMesh& mesh, const wxString& name, bool center)
{
    // Add mesh to model as a new object
    Model& model = wxGetApp().plater()->model();

#ifdef _DEBUG
    check_model_ids_validity(model);
#endif /* _DEBUG */

    std::vector<size_t> object_idxs;
    auto                bb         = mesh.bounding_box();
    ModelObject*        new_object = model.add_object();
    new_object->name               = into_u8(name);
    new_object->add_instance(); // each object should have at least one instance

    ModelVolume* new_volume = new_object->add_volume(mesh);
    new_object->sort_volumes(true);
    new_volume->name = into_u8(name);
    // set a default extruder value, since user can't add it manually
    // BBS
    new_object->config.set_key_value("extruder", new ConfigOptionInt(1));
    new_object->invalidate_bounding_box();
    new_object->translate(-bb.center());

    // BBS: backup
    Slic3r::save_object_mesh(*new_object);

    //// BBS: find an empty cell to put the copied object
    // auto start_point = wxGetApp().plater()->build_volume().bounding_volume2d().center();

    // BoundingBoxf3 bbox;
    // bbox.merge(new_volume->get_convex_hull().bounding_box());

    // Vec2f bbox_size = { bbox.size().x() + 1.0, bbox.size().y() + 1.0 };

    // auto empty_cell  = wxGetApp().plater()->canvas3D()->get_nearest_empty_cell({start_point(0), start_point(1)}, bbox_size);

    // new_object->instances[0]->set_offset(center ? to_3d(Vec2d(empty_cell(0), empty_cell(1)), -new_object->origin_translation.z()) : bb.center());

    wxGetApp().plater()->arrange_loaded_object_to_new_position(new_object->instances[0]);

    new_object->ensure_on_bed();

    // BBS init assmeble transformation
    Geometry::Transformation t = new_object->instances[0]->get_transformation();
    new_object->instances[0]->set_assemble_transformation(t);

    object_idxs.push_back(model.objects.size() - 1);
#ifdef _DEBUG
    check_model_ids_validity(model);
#endif /* _DEBUG */

    paste_objects_into_list(object_idxs);

#ifdef _DEBUG
    check_model_ids_validity(model);
#endif /* _DEBUG */
}

// BBS
bool ObjectList::del_object(const int obj_idx, bool refresh_immediately)
{
    return wxGetApp().plater()->delete_object_from_model(obj_idx, refresh_immediately);
}

// Delete subobject
void ObjectList::del_subobject_item(wxDataViewItem& item)
{
    if (!item)
        return;

    int      obj_idx, idx;
    ItemType type;

    m_objects_model->GetItemInfo(item, type, obj_idx, idx);
    if (type == itUndef)
        return;

    wxDataViewItem parent = m_objects_model->GetParent(item);

    InfoItemType item_info_type = m_objects_model->GetInfoItemType(item);
    if (type & itSettings)
        del_settings_from_config(parent);
    else if (type & itInstanceRoot && obj_idx != -1)
        del_instances_from_object(obj_idx);
    else if (type & itLayerRoot && obj_idx != -1)
        del_layers_from_object(obj_idx);
    else if (type & itLayer && obj_idx != -1)
        del_layer_from_object(obj_idx, m_objects_model->GetLayerRangeByItem(item));
    else if (type & itInfo && obj_idx != -1)
        del_info_item(obj_idx, item_info_type);
    else if (idx == -1)
        return;
    else if (!del_subobject_from_object(obj_idx, idx, type))
        return;

    // If last volume item with warning was deleted, unmark object item
    if (type & itVolume) {
        const std::string& icon_name = get_warning_icon_name(object(obj_idx)->get_object_stl_stats());
        m_objects_model->UpdateWarningIcon(parent, icon_name);
    }

    if (!(type & itInfo) || item_info_type != InfoItemType::CutConnectors) {
        // Connectors Item is already updated/deleted inside the del_info_item()
        m_objects_model->Delete(item);
        update_info_items(obj_idx);
    }
}

void ObjectList::del_info_item(const int obj_idx, InfoItemType type)
{
    Plater*     plater = wxGetApp().plater();
    GLCanvas3D* cnv    = plater->canvas3D();

    switch (type) {
    case InfoItemType::CustomSupports:
        cnv->get_gizmos_manager().reset_all_states();
        Plater::TakeSnapshot(plater, "Remove support painting");
        for (ModelVolume* mv : (*m_objects)[obj_idx]->volumes)
            mv->supported_facets.reset();
        break;

    // BBS: remove CustomSeam
    case InfoItemType::MmuSegmentation:
        cnv->get_gizmos_manager().reset_all_states();
        Plater::TakeSnapshot(plater, "Remove color painting");
        for (ModelVolume* mv : (*m_objects)[obj_idx]->volumes)
            mv->mmu_segmentation_facets.reset();
        break;

    case InfoItemType::CutConnectors:
        if (!del_from_cut_object(true)) {
            // there is no need to post EVT_GLCANVAS_SCHEDULE_BACKGROUND_PROCESS if nothing was changed
            return;
        }
        break;

    // BBS: remove Sinking
    case InfoItemType::Undef: assert(false); break;
    }
    cnv->post_event(SimpleEvent(EVT_GLCANVAS_SCHEDULE_BACKGROUND_PROCESS));
}

void ObjectList::del_settings_from_config(const wxDataViewItem& parent_item)
{
    const bool is_layer_settings = m_objects_model->GetItemType(parent_item) == itLayer;

    const size_t opt_cnt = m_config->keys().size();
    if ((opt_cnt == 1 && m_config->has("extruder")) ||
        (is_layer_settings && opt_cnt == 2 && m_config->has("extruder") && m_config->has("layer_height")))
        return;

    take_snapshot("Delete Settings");

    int extruder = m_config->has("extruder") ? m_config->extruder() : -1;

    coordf_t layer_height = 0.0;
    if (is_layer_settings)
        layer_height = m_config->opt_float("layer_height");

    m_config->reset();

    if (extruder >= 0)
        m_config->set_key_value("extruder", new ConfigOptionInt(extruder));
    if (is_layer_settings)
        m_config->set_key_value("layer_height", new ConfigOptionFloat(layer_height));

    changed_object();
}

void ObjectList::del_instances_from_object(const int obj_idx)
{
    auto& instances = (*m_objects)[obj_idx]->instances;
    if (instances.size() <= 1)
        return;

    // BBS: remove snapshot name "Delete All Instances from Object"
    take_snapshot("");

    while (instances.size() > 1)
        instances.pop_back();

    (*m_objects)[obj_idx]->invalidate_bounding_box(); // ? #ys_FIXME

    changed_object(obj_idx);
}

void ObjectList::del_layer_from_object(const int obj_idx, const t_layer_height_range& layer_range)
{
    const auto del_range = object(obj_idx)->layer_config_ranges.find(layer_range);
    if (del_range == object(obj_idx)->layer_config_ranges.end())
        return;

    take_snapshot("Remove height range");

    object(obj_idx)->layer_config_ranges.erase(del_range);

    changed_object(obj_idx);
}

void ObjectList::del_layers_from_object(const int obj_idx)
{
    object(obj_idx)->layer_config_ranges.clear();

    changed_object(obj_idx);
}

bool ObjectList::del_from_cut_object(bool is_cut_connector, bool is_model_part /* = false*/, bool is_negative_volume /* = false*/)
{
    const long buttons_style = is_cut_connector ? (wxYES | wxNO | wxCANCEL) : (wxYES | wxCANCEL);

    const wxString title = is_cut_connector   ? _L("Delete connector from object which is a part of cut") :
                           is_model_part      ? _L("Delete solid part from object which is a part of cut") :
                           is_negative_volume ? _L("Delete negative volume from object which is a part of cut") :
                                                "";

    const wxString msg_end = is_cut_connector ?
                                 ("\n" + _L("To save cut correspondence you can delete all connectors from all related objects.")) :
                                 "";

    InfoDialog dialog(wxGetApp().plater(), title,
                      (_L("This action will break a cut correspondence.\n"
                          "After that model consistency can't be guaranteed .\n"
                          "\n"
                          "To manipulate with solid parts or negative volumes you have to invalidate cut infornation first.") +
                       msg_end),
                      false, buttons_style | wxCANCEL_DEFAULT | wxICON_WARNING);

    dialog.SetButtonLabel(wxID_YES, _L("Invalidate cut info"));
    if (is_cut_connector)
        dialog.SetButtonLabel(wxID_NO, _L("Delete all connectors"));

    const int answer = dialog.ShowModal();
    if (answer == wxID_CANCEL)
        return false;

    if (answer == wxID_YES)
        invalidate_cut_info_for_selection();
    else if (answer == wxID_NO)
        delete_all_connectors_for_selection();
    return true;
}

bool ObjectList::del_subobject_from_object(const int obj_idx, const int idx, const int type)
{
    assert(idx >= 0);

    // BBS: support partplage logic
    int n_plates = wxGetApp().plater()->get_partplate_list().get_plate_count();
    if ((obj_idx >= 1000 && obj_idx < 1000 + n_plates) || idx < 0)
        // Cannot delete a wipe tower or volume with negative id
        return false;

    ModelObject* object = (*m_objects)[obj_idx];

    if (type == itVolume) {
        const auto volume = object->volumes[idx];

        // if user is deleting the last solid part, throw error
        int solid_cnt = 0;
        for (auto vol : object->volumes)
            if (vol->is_model_part())
                ++solid_cnt;
        if (volume->is_model_part() && solid_cnt == 1) {
            Slic3r::GUI::show_error(nullptr, _L("Deleting the last solid part is not allowed."));
            return false;
        }
        if (object->is_cut() && (volume->is_model_part() || volume->is_negative_volume())) {
            del_from_cut_object(volume->is_cut_connector(), volume->is_model_part(), volume->is_negative_volume());
            // in any case return false to break the deletion
            return false;
        }

        take_snapshot("Delete part");

        object->delete_volume(idx);

        if (object->volumes.size() == 1) {
            const auto last_volume = object->volumes[0];
            if (!last_volume->config.empty()) {
                object->config.apply(last_volume->config);
                last_volume->config.reset();

                // update extruder color in ObjectList
                wxDataViewItem obj_item = m_objects_model->GetItemById(obj_idx);
                if (obj_item) {
                    // BBS
                    if (last_volume->config.has("extruder")) {
                        int extruder_id = last_volume->config.opt_int("extruder");
                        object->config.set("extruder", extruder_id);
                    }
                    wxString extruder = object->config.has("extruder") ? wxString::Format("%d", object->config.extruder()) : _devL("1");
                    m_objects_model->SetExtruder(extruder, obj_item);
                }
                // add settings to the object, if it has them
                add_settings_item(obj_item, &object->config.get());
            }
        }
        // BBS: notify partplate the modify
        notify_instance_updated(obj_idx);
    } else if (type == itInstance) {
        if (object->instances.size() == 1) {
            // BBS: remove snapshot name "Last instance of an object cannot be deleted."
            Slic3r::GUI::show_error(nullptr, "");
            return false;
        }

        // BBS: remove snapshot name "Delete Instance"
        take_snapshot("");
        object->delete_instance(idx);
    } else
        return false;

    changed_object(obj_idx);

    return true;
}

void ObjectList::split()
{
    const auto item    = GetSelection();
    const int  obj_idx = get_selected_obj_idx();
    if (!item || obj_idx < 0)
        return;

    ModelVolume* volume;
    if (!get_volume_by_item(item, volume))
        return;
    DynamicPrintConfig& config = printer_config();
    // BBS
    const ConfigOptionStrings* filament_colors = config.option<ConfigOptionStrings>("filament_colour", false);
    const auto                 filament_cnt    = (filament_colors == nullptr) ? size_t(1) : filament_colors->size();
    if (!volume->is_splittable()) {
        wxMessageBox(_(L("The target object contains only one part and can not be splited.")));
        return;
    }

    take_snapshot("Split to parts");

    volume->split(filament_cnt);

    wxBusyCursor wait;

    auto model_object = (*m_objects)[obj_idx];

    auto parent = m_objects_model->GetObject(item);
    if (parent)
        m_objects_model->DeleteVolumeChildren(parent);
    else
        parent = item;

    for (const ModelVolume* volume : model_object->volumes) {
        const wxDataViewItem& vol_item = m_objects_model->AddVolumeChild(
            parent, from_u8(volume->name),
            volume->type(), // is_modifier() ? ModelVolumeType::PARAMETER_MODIFIER : ModelVolumeType::MODEL_PART,
            volume->is_text(), volume->is_svg(), get_warning_icon_name(volume->mesh().stats()),
            volume->config.has("extruder") ? volume->config.extruder() : 0, false);
        // add settings to the part, if it has those
        add_settings_item(vol_item, &volume->config.get());
    }

    model_object->input_file.clear();

    if (parent == item)
        Expand(parent);

    changed_object(obj_idx);

    // BBS: notify partplate the modify
    notify_instance_updated(obj_idx);

    update_info_items(obj_idx);
}

void ObjectList::merge(bool to_multipart_object)
{
    // merge selected objects to the multipart object
    if (to_multipart_object) {
        auto get_object_idxs = [this](std::vector<int>& obj_idxs, wxDataViewItemArray& sels) {
            // check selections and split instances to the separated objects...
            bool instance_selection = false;
            for (wxDataViewItem item : sels)
                if (m_objects_model->GetItemType(item) & itInstance) {
                    instance_selection = true;
                    break;
                }

            if (!instance_selection) {
                for (wxDataViewItem item : sels) {
                    assert(m_objects_model->GetItemType(item) & itObject);
                    obj_idxs.emplace_back(m_objects_model->GetIdByItem(item));
                }
                return;
            }

            // map of obj_idx -> set of selected instance_idxs
            std::map<int, std::set<int>> sel_map;
            std::set<int>                empty_set;
            for (wxDataViewItem item : sels) {
                if (m_objects_model->GetItemType(item) & itObject) {
                    int obj_idx  = m_objects_model->GetIdByItem(item);
                    int inst_cnt = (*m_objects)[obj_idx]->instances.size();
                    if (inst_cnt == 1)
                        sel_map.emplace(obj_idx, empty_set);
                    else
                        for (int i = 0; i < inst_cnt; i++)
                            sel_map[obj_idx].emplace(i);
                    continue;
                }
                int obj_idx = m_objects_model->GetIdByItem(m_objects_model->GetObject(item));
                sel_map[obj_idx].emplace(m_objects_model->GetInstanceIdByItem(item));
            }

            // all objects, created from the instances will be added to the end of list
            int new_objects_cnt = 0; // count of this new objects

            for (auto map_item : sel_map) {
                int obj_idx = map_item.first;
                // object with just 1 instance
                if (map_item.second.empty()) {
                    obj_idxs.emplace_back(obj_idx);
                    continue;
                }

                // object with selected all instances
                if ((*m_objects)[map_item.first]->instances.size() == map_item.second.size()) {
                    instances_to_separated_objects(obj_idx);
                    // first instance stay on its own place and another all add to the end of list :
                    obj_idxs.emplace_back(obj_idx);
                    new_objects_cnt += map_item.second.size() - 1;
                    continue;
                }

                // object with selected some of instances
                instances_to_separated_object(obj_idx, map_item.second);

                if (map_item.second.size() == 1)
                    new_objects_cnt += 1;
                else { // we should split to separate instances last object
                    instances_to_separated_objects(m_objects->size() - 1);
                    // all instances will stay at the end of list :
                    new_objects_cnt += map_item.second.size();
                }
            }

            // all instatnces are extracted to the separate objects and should be selected
            m_prevent_list_events = true;
            sels.Clear();
            for (int obj_idx : obj_idxs)
                sels.Add(m_objects_model->GetItemById(obj_idx));
            int obj_cnt = m_objects->size();
            for (int obj_idx = obj_cnt - new_objects_cnt; obj_idx < obj_cnt; obj_idx++) {
                sels.Add(m_objects_model->GetItemById(obj_idx));
                obj_idxs.emplace_back(obj_idx);
            }
            UnselectAll();
            SetSelections(sels);
            assert(!sels.IsEmpty());
            m_prevent_list_events = false;
        };

        std::vector<int>    obj_idxs;
        wxDataViewItemArray sels;
        GetSelections(sels);
        assert(!sels.IsEmpty());

        Plater::TakeSnapshot snapshot(wxGetApp().plater(), "Assemble");

        get_object_idxs(obj_idxs, sels);

        // resulted objects merge to the one
        Model*       model      = (*m_objects)[0]->get_model();
        ModelObject* new_object = model->add_object();
        new_object->name        = _u8L("Assembly");
        ModelConfig& config     = new_object->config;

        Slic3r::SaveObjectGaurd gaurd(*new_object);

        for (int obj_idx : obj_idxs) {
            ModelObject* object = (*m_objects)[obj_idx];

            const Geometry::Transformation& transformation = object->instances[0]->get_transformation();
            // const Vec3d scale     = transformation.get_scaling_factor();
            // const Vec3d mirror    = transformation.get_mirror();
            // const Vec3d rotation  = transformation.get_rotation();

            if (object->id() == (*m_objects)[obj_idxs.front()]->id())
                new_object->add_instance();
            const Transform3d& transformation_matrix = transformation.get_matrix();

            // merge volumes
            for (const ModelVolume* volume : object->volumes) {
                ModelVolume* new_volume = new_object->add_volume(*volume);

                // BBS: keep volume's transform
                const Transform3d& volume_matrix = new_volume->get_matrix();
                Transform3d        new_matrix    = transformation_matrix * volume_matrix;
                new_volume->set_transformation(new_matrix);
                // set rotation
                /*const Vec3d vol_rot = new_volume->get_rotation() + rotation;
                new_volume->set_rotation(vol_rot);

                // set scale
                const Vec3d vol_sc_fact = new_volume->get_scaling_factor().cwiseProduct(scale);
                new_volume->set_scaling_factor(vol_sc_fact);

                // set mirror
                const Vec3d vol_mirror = new_volume->get_mirror().cwiseProduct(mirror);
                new_volume->set_mirror(vol_mirror);

                // set offset
                const Vec3d vol_offset = volume_offset_correction* new_volume->get_offset();
                new_volume->set_offset(vol_offset);*/

                // BBS: add config from old objects
                // for object config, it has settings of PrintObjectConfig and PrintRegionConfig
                // for volume config, it only has settings of PrintRegionConfig
                // so we can not copy settings from object to volume
                // but we can copy settings from volume to object
                if (object->volumes.size() > 1) {
                    new_volume->config.assign_config(volume->config);
                }

                new_volume->mmu_segmentation_facets.assign(std::move(volume->mmu_segmentation_facets));
            }
            new_object->sort_volumes(true);

            // merge settings
            auto               new_opt_keys = config.keys();
            const ModelConfig& from_config  = object->config;
            auto               opt_keys     = from_config.keys();

            for (auto& opt_key : opt_keys) {
                if (find(new_opt_keys.begin(), new_opt_keys.end(), opt_key) == new_opt_keys.end()) {
                    const ConfigOption* option = from_config.option(opt_key);
                    if (!option) {
                        // if current option doesn't exist in prints.get_edited_preset(),
                        // get it from default config values
                        option = DynamicPrintConfig::new_from_defaults_keys({opt_key})->option(opt_key);
                    }
                    config.set_key_value(opt_key, option->clone());
                }
            }
            // save extruder value if it was set
            if (object->volumes.size() == 1 && find(opt_keys.begin(), opt_keys.end(), "extruder") != opt_keys.end()) {
                ModelVolume*        volume = new_object->volumes.back();
                const ConfigOption* option = from_config.option("extruder");
                if (option)
                    volume->config.set_key_value("extruder", option->clone());
            }

            // merge layers
            for (const auto& range : object->layer_config_ranges)
                new_object->layer_config_ranges.emplace(range);
        }

        // BBS: ensure on bed, and no need to center around origin
        new_object->ensure_on_bed();
        new_object->center_around_origin();
        new_object->translate_instances(-new_object->origin_translation);
        new_object->origin_translation = Vec3d::Zero();
        // BBS init asssmble transformation
        Geometry::Transformation t = new_object->instances[0]->get_transformation();
        new_object->instances[0]->set_assemble_transformation(t);
        // BBS: notify it before remove
        notify_instance_updated(m_objects->size() - 1);

        // remove selected objects
        remove();

        // Add new object(merged) to the object_list
        add_object_to_list(m_objects->size() - 1);
        select_item(m_objects_model->GetItemById(m_objects->size() - 1));
        update_selections_on_canvas();
    }
    // merge all parts to the one single object
    // all part's settings will be lost
    else {
        wxDataViewItem item = GetSelection();
        if (!item)
            return;
        const int obj_idx = m_objects_model->GetIdByItem(item);
        if (obj_idx == -1)
            return;

        Plater::TakeSnapshot snapshot(wxGetApp().plater(), "Merge parts to an object");

        ModelObject* model_object = (*m_objects)[obj_idx];
        model_object->merge();

        m_objects_model->DeleteVolumeChildren(item);

        changed_object(obj_idx);
    }
}

/*void ObjectList::merge_volumes()
{
    std::vector<int> obj_idxs, vol_idxs;
    get_selection_indexes(obj_idxs, vol_idxs);
    if (obj_idxs.empty() && vol_idxs.empty())
        return;

    wxBusyCursor wait;
#if 0
    ModelObjectPtrs objects;
    for (int obj_idx : obj_idxs) {
        ModelObject* object = (*m_objects)[obj_idx];
        object->merge_volumes(vol_idxs);
        //changed_object(obj_idx);
        //remove();
    }
   /* wxGetApp().plater()->load_model_objects(objects);

    Selection& selection = p->view3D->get_canvas3d()->get_selection();
    size_t last_obj_idx = p->model.objects.size() - 1;

    if (vol_idxs.empty()) {
        for (size_t i = 0; i < objects.size(); ++i)
            selection.add_object((unsigned int)(last_obj_idx - i), i == 0);
    }
    else {
        for (int vol_idx : vol_idxs)
            selection.add_volume(last_obj_idx, vol_idx, 0, false);
    }#1#
#else
    wxGetApp().plater()->merge(obj_idxs[0], vol_idxs);
#endif
}*/

void ObjectList::layers_editing()
{
    const Selection& selection = scene_selection();
    const int        obj_idx   = selection.get_object_idx();
    wxDataViewItem   item      = obj_idx >= 0 && GetSelectedItemsCount() > 1 && selection.is_single_full_object() ?
                                     m_objects_model->GetItemById(obj_idx) :
                                     GetSelection();

    if (!item)
        return;

    const wxDataViewItem obj_item    = m_objects_model->GetObject(item);
    wxDataViewItem       layers_item = m_objects_model->GetLayerRootItem(obj_item);

    // if it doesn't exist now
    if (!layers_item.IsOk()) {
        t_layer_config_ranges& ranges = object(obj_idx)->layer_config_ranges;

        // set some default value
        if (ranges.empty()) {
            // BBS: remove snapshot name "Add layers"
            take_snapshot("Add layers");
            ranges[{0.0f, 2.0f}].assign_config(get_default_layer_config(obj_idx));
        }

        // create layer root item
        layers_item = add_layer_root_item(obj_item);
    }
    if (!layers_item.IsOk())
        return;

    // to correct visual hints for layers editing on the Scene, reset previous selection
    wxGetApp().obj_layers()->reset_selection();
    wxGetApp().plater()->canvas3D()->handle_sidebar_focus_event("", false);

    // select LayerRoor item and expand
    select_item(layers_item);
    Expand(layers_item);
}

// BBS: merge parts of a single object into one volume, similar to export_stl, but no need to export and then import
void ObjectList::boolean()
{
    std::vector<int> obj_idxs, vol_idxs;
    get_selection_indexes(obj_idxs, vol_idxs);
    if (obj_idxs.empty() && vol_idxs.empty())
        return;

    Plater::TakeSnapshot snapshot(wxGetApp().plater(), "boolean");

    ModelObject* object = (*m_objects)[obj_idxs.front()];
    TriangleMesh mesh   = Plater::combine_mesh_fff(*object, -1, [this](const std::string& msg) {
        return wxGetApp().notification_manager()->push_plater_error_notification(msg);
    });

    // add mesh to model as a new object, keep the original object's name and config
    Model*       model      = object->get_model();
    ModelObject* new_object = model->add_object();
    new_object->name        = object->name;
    new_object->config.assign_config(object->config);
    if (new_object->instances.empty())
        new_object->add_instance();
    ModelVolume* new_volume = new_object->add_volume(mesh);

    // BBS: ensure on bed but no need to ensure locate in the center around origin
    new_object->ensure_on_bed();
    new_object->center_around_origin();
    new_object->translate_instances(-new_object->origin_translation);
    new_object->origin_translation = Vec3d::Zero();

    // BBS: notify it before move
    notify_instance_updated(m_objects->size() - 1);

    // remove selected objects
    remove();

    // Add new object(UNION) to the object_list
    add_object_to_list(m_objects->size() - 1);
    select_item(m_objects_model->GetItemById(m_objects->size() - 1));
    update_selections_on_canvas();
}

wxDataViewItem ObjectList::add_layer_root_item(const wxDataViewItem obj_item)
{
    const int obj_idx = m_objects_model->GetIdByItem(obj_item);
    if (obj_idx < 0 || object(obj_idx)->layer_config_ranges.empty() || printer_technology() == ptSLA)
        return wxDataViewItem(nullptr);

    // create LayerRoot item
    wxDataViewItem layers_item = m_objects_model->AddLayersRoot(obj_item);

    // and create Layer item(s) according to the layer_config_ranges
    for (const auto& range : object(obj_idx)->layer_config_ranges)
        add_layer_item(range.first, layers_item);

    Expand(layers_item);
    return layers_item;
}

DynamicPrintConfig ObjectList::get_default_layer_config(const int obj_idx)
{
    DynamicPrintConfig config;
    coordf_t           layer_height = object(obj_idx)->config.has("layer_height") ?
                                          object(obj_idx)->config.opt_float("layer_height") :
                                          wxGetApp().preset_bundle->prints.get_edited_preset().config.opt_float("layer_height");
    config.set_key_value("layer_height", new ConfigOptionFloat(layer_height));
    // BBS
    int extruder = object(obj_idx)->config.has("extruder") ?
                       object(obj_idx)->config.opt_int("extruder") :
                       wxGetApp().preset_bundle->prints.get_edited_preset().config.opt_float("extruder");
    config.set_key_value("extruder", new ConfigOptionInt(0));

    return config;
}

bool ObjectList::get_volume_by_item(const wxDataViewItem& item, ModelVolume*& volume)
{
    auto obj_idx = get_selected_obj_idx();
    if (!item || obj_idx < 0)
        return false;
    const auto volume_id  = m_objects_model->GetVolumeIdByItem(item);
    const bool split_part = m_objects_model->GetItemType(item) == itVolume;

    // object is selected
    if (volume_id < 0) {
        if (split_part || (*m_objects)[obj_idx]->volumes.size() > 1)
            return false;
        volume = (*m_objects)[obj_idx]->volumes[0];
    }
    // volume is selected
    else
        volume = (*m_objects)[obj_idx]->volumes[volume_id];

    return true;
}

bool ObjectList::is_splittable(bool to_objects)
{
    const wxDataViewItem item = GetSelection();
    if (!item)
        return false;

    if (to_objects) {
        ItemType type = m_objects_model->GetItemType(item);
        if (type == itVolume)
            return false;
        if (type == itObject || m_objects_model->GetItemType(m_objects_model->GetParent(item)) == itObject) {
            auto obj_idx = get_selected_obj_idx();
            if (obj_idx < 0)
                return false;
            if ((*m_objects)[obj_idx]->volumes.size() > 1)
                return true;
            return (*m_objects)[obj_idx]->volumes[0]->is_splittable();
        }
        return false;
    }

    ModelVolume* volume;
    if (!get_volume_by_item(item, volume) || !volume)
        return false;

    return volume->is_splittable();
}

bool ObjectList::selected_instances_of_same_object()
{
    wxDataViewItemArray sels;
    GetSelections(sels);

    const int obj_idx = m_objects_model->GetObjectIdByItem(sels.front());

    for (auto item : sels) {
        if (!(m_objects_model->GetItemType(item) & itInstance) || obj_idx != m_objects_model->GetObjectIdByItem(item))
            return false;
    }
    return true;
}

bool ObjectList::can_split_instances()
{
    const Selection& selection = scene_selection();
    return selection.is_multiple_full_instance() || selection.is_single_full_instance();
}

bool ObjectList::has_selected_cut_object() const
{
    wxDataViewItemArray sels;
    GetSelections(sels);
    if (sels.IsEmpty())
        return false;

    for (wxDataViewItem item : sels) {
        const int obj_idx = m_objects_model->GetObjectIdByItem(item);
        // ys_FIXME: The obj_idx<size condition is a workaround for https://github.com/prusa3d/PrusaSlicer/issues/11186,
        // but not the correct fix. The deleted item probably should not be in sels in the first place.
        if (obj_idx >= 0 && obj_idx < int(m_objects->size()) && object(obj_idx)->is_cut())
            return true;
    }

    return false;
}

void ObjectList::invalidate_cut_info_for_selection()
{
    const wxDataViewItem item = GetSelection();
    if (item) {
        const int obj_idx = m_objects_model->GetObjectIdByItem(item);
        if (obj_idx >= 0)
            invalidate_cut_info_for_object(size_t(obj_idx));
    }
}

void ObjectList::invalidate_cut_info_for_object(int obj_idx)
{
    ModelObject* init_obj = object(obj_idx);
    if (!init_obj->is_cut())
        return;

    take_snapshot(_u8L("Invalidate cut info"));

    const CutObjectBase cut_id = init_obj->cut_id;
    // invalidate cut for related objects (which have the same cut_id)
    for (size_t idx = 0; idx < m_objects->size(); idx++)
        if (ModelObject* obj = object(int(idx)); obj->cut_id.is_equal(cut_id)) {
            obj->invalidate_cut();
            update_info_items(idx);
            add_volumes_to_object_in_list(idx);
        }

    update_lock_icons_for_model();
}

void ObjectList::delete_all_connectors_for_selection()
{
    const wxDataViewItem item = GetSelection();
    if (item) {
        const int obj_idx = m_objects_model->GetObjectIdByItem(item);
        if (obj_idx >= 0)
            delete_all_connectors_for_object(size_t(obj_idx));
    }
}

void ObjectList::delete_all_connectors_for_object(int obj_idx)
{
    ModelObject* init_obj = object(obj_idx);
    if (!init_obj->is_cut())
        return;

    take_snapshot(_u8L("Delete all connectors"));

    const CutObjectBase cut_id = init_obj->cut_id;
    // Delete all connectors for related objects (which have the same cut_id)
    Model& model = wxGetApp().plater()->model();
    for (int idx = int(m_objects->size()) - 1; idx >= 0; idx--)
        if (ModelObject* obj = object(idx); obj->cut_id.is_equal(cut_id)) {
            obj->delete_connectors();

            if (obj->volumes.empty() || !obj->has_solid_mesh()) {
                model.delete_object(idx);
                m_objects_model->Delete(m_objects_model->GetItemById(idx));
                continue;
            }

            update_info_items(idx);
            add_volumes_to_object_in_list(idx);
            changed_object(int(idx));
        }

    update_lock_icons_for_model();
}

bool ObjectList::can_merge_to_multipart_object() const
{
    if (has_selected_cut_object())
        return false;

    if (printer_technology() == ptSLA)
        return false;

    wxDataViewItemArray sels;
    GetSelections(sels);
    if (sels.IsEmpty())
        return false;

    // should be selected just objects
    for (wxDataViewItem item : sels)
        if (!(m_objects_model->GetItemType(item) & (itObject | itInstance)))
            return false;

    return true;
}

bool ObjectList::can_merge_to_single_object() const
{
    int obj_idx = get_selected_obj_idx();
    if (obj_idx < 0)
        return false;

    // selected object should be multipart
    return (*m_objects)[obj_idx]->volumes.size() > 1;
}

bool ObjectList::can_mesh_boolean() const
{
    int obj_idx = get_selected_obj_idx();
    if (obj_idx < 0)
        return false;

    // selected object should be multi mesh
    return (*m_objects)[obj_idx]->volumes.size() > 1 ||
           ((*m_objects)[obj_idx]->volumes.size() == 1 && (*m_objects)[obj_idx]->volumes[0]->is_splittable());
}

// NO_PARAMETERS function call means that changed object index will be determine from Selection()
void ObjectList::changed_object(const int obj_idx /* = -1*/) const
{
    wxGetApp().plater()->changed_object(obj_idx < 0 ? get_selected_obj_idx() : obj_idx);
}

void ObjectList::part_selection_changed()
{
    if (m_extruder_editor)
        m_extruder_editor->Hide();
    int obj_idx      = -1;
    int volume_id    = -1;
    m_config         = nullptr;
    wxString og_name = wxEmptyString;

    bool update_and_show_manipulations = false;
    bool update_and_show_settings      = false;
    bool update_and_show_layers        = false;

    bool enable_manipulation{true};
    bool disable_ss_manipulation{false};
    bool disable_ununiform_scale{false};

    const auto item = GetSelection();
    if (item && m_objects_model->GetItemType(item) == itInfo && m_objects_model->GetInfoItemType(item) == InfoItemType::CutConnectors) {
        og_name = _L("Cut Connectors information");

        update_and_show_manipulations = true;
        enable_manipulation           = false;
        disable_ununiform_scale       = true;
    } else if (item && (m_objects_model->GetItemType(item) & itPlate)) {
        // BBS
        // TODO: og for plate
    } else if (multiple_selection() || (item && m_objects_model->GetItemType(item) == itInstanceRoot)) {
        const Selection& selection = scene_selection();

        if (selection.is_single_full_object()) {
            og_name                       = _L("Object manipulation");
            update_and_show_manipulations = true;

            obj_idx                 = selection.get_object_idx();
            ModelObject* object     = (*m_objects)[obj_idx];
            m_config                = &object->config;
            disable_ss_manipulation = object->is_cut();
        } else {
            og_name = _L("Group manipulation");

            // don't show manipulation panel for case of all Object's parts selection
            update_and_show_manipulations = !selection.is_single_full_instance();

            if (int obj_idx = selection.get_object_idx(); obj_idx >= 0) {
                if (selection.is_any_volume() || selection.is_any_modifier())
                    enable_manipulation = !(*m_objects)[obj_idx]->is_cut();
                else // if (item && m_objects_model->GetItemType(item) == itInstanceRoot)
                    disable_ss_manipulation = (*m_objects)[obj_idx]->is_cut();
            } else {
                wxDataViewItemArray sels;
                GetSelections(sels);
                if (selection.is_single_full_object() || selection.is_multiple_full_instance()) {
                    int obj_idx             = m_objects_model->GetObjectIdByItem(sels.front());
                    disable_ss_manipulation = (*m_objects)[obj_idx]->is_cut();
                } else if (selection.is_mixed() || selection.is_multiple_full_object()) {
                    std::map<CutObjectBase, std::set<int>> cut_objects;

                    // find cut objects
                    for (auto item : sels) {
                        int                obj_idx = m_objects_model->GetObjectIdByItem(item);
                        const ModelObject* obj     = object(obj_idx);
                        if (obj && obj->is_cut()) {
                            if (cut_objects.find(obj->cut_id) == cut_objects.end())
                                cut_objects[obj->cut_id] = std::set<int>{obj_idx};
                            else
                                cut_objects.at(obj->cut_id).insert(obj_idx);
                        }
                    }

                    // check if selected cut objects are "full selected"
                    for (auto cut_object : cut_objects)
                        if (cut_object.first.check_sum() != cut_object.second.size()) {
                            disable_ss_manipulation = true;
                            break;
                        }
                    disable_ununiform_scale = !cut_objects.empty();
                }
            }
        }

        // BBS: multi config editing
        update_and_show_settings = true;
    } else {
        if (item) {
            // BBS
            const ItemType       type        = m_objects_model->GetItemType(item);
            const wxDataViewItem parent      = m_objects_model->GetParent(item);
            const ItemType       parent_type = m_objects_model->GetItemType(parent);
            obj_idx                          = m_objects_model->GetObjectIdByItem(item);

            if ((type & itObject) || type == itInfo) {
                m_config                      = &(*m_objects)[obj_idx]->config;
                update_and_show_manipulations = true;

                if (type == itInfo) {
                    InfoItemType info_type = m_objects_model->GetInfoItemType(item);
                    switch (info_type) {
                    case InfoItemType::CustomSupports:
                    // BBS: remove CustomSeam
                    // case InfoItemType::CustomSeam:
                    case InfoItemType::MmuSegmentation: {
                        GLGizmosManager::EType gizmo_type = info_type == InfoItemType::CustomSupports ?
                                                                GLGizmosManager::EType::FdmSupports :
                                                                /*info_type == InfoItemType::CustomSeam ? GLGizmosManager::EType::Seam :*/
                                                                GLGizmosManager::EType::MmuSegmentation;
                        GLGizmosManager&       gizmos_mgr = wxGetApp().plater()->get_view3D_canvas3D()->get_gizmos_manager();
                        if (gizmos_mgr.get_current_type() != gizmo_type)
                            gizmos_mgr.open_gizmo(gizmo_type);
                        break;
                    }
                    // BBS: remove Sinking
                    // case InfoItemType::Sinking: { break; }
                    default: {
                        break;
                    }
                    }
                } else {
                    // BBS: select object to edit config
                    m_config                 = &(*m_objects)[obj_idx]->config;
                    update_and_show_settings = true;
                    disable_ss_manipulation  = (*m_objects)[obj_idx]->is_cut();
                }
            } else {
                if (type & itSettings) {
                    if (parent_type & itObject) {
                        og_name  = _L("Object Settings to modify");
                        m_config = &(*m_objects)[obj_idx]->config;
                    } else if (parent_type & itVolume) {
                        og_name   = _L("Part Settings to modify");
                        volume_id = m_objects_model->GetVolumeIdByItem(parent);
                        m_config  = &(*m_objects)[obj_idx]->volumes[volume_id]->config;
                    } else if (parent_type & itLayer) {
                        og_name  = _L("Layer range Settings to modify");
                        m_config = &get_item_config(parent);
                    }
                    update_and_show_settings = true;
                } else if (type & itVolume) {
                    og_name                       = _L("Part manipulation");
                    volume_id                     = m_objects_model->GetVolumeIdByItem(item);
                    m_config                      = &(*m_objects)[obj_idx]->volumes[volume_id]->config;
                    update_and_show_manipulations = true;
                    m_config                      = &(*m_objects)[obj_idx]->volumes[volume_id]->config;
                    update_and_show_settings      = true;

                    const ModelVolume* volume = (*m_objects)[obj_idx]->volumes[volume_id];
                    enable_manipulation = !((*m_objects)[obj_idx]->is_cut() && (volume->is_cut_connector() || volume->is_model_part()));
                } else if (type & itInstance) {
                    og_name                       = _L("Instance manipulation");
                    update_and_show_manipulations = true;

                    // fill m_config by object's values
                    m_config                = &(*m_objects)[obj_idx]->config;
                    disable_ss_manipulation = (*m_objects)[obj_idx]->is_cut();
                } else if (type & (itLayerRoot | itLayer)) {
                    og_name                  = type & itLayerRoot ? _L("Height ranges") : _L("Settings for height range");
                    update_and_show_layers   = true;
                    update_and_show_settings = true;

                    if (type & itLayer)
                        m_config = &get_item_config(item);
                }
            }
        }
    }

    m_selected_object_id = obj_idx;

    if (update_and_show_manipulations) {
        // BBS
        // wxGetApp().obj_manipul()->get_og()->set_name(" " + og_name + " ");

        if (item) {
            // wxGetApp().obj_manipul()->get_og()->set_value("object_name", m_objects_model->GetName(item));
            // BBS
            // wxGetApp().obj_manipul()->update_item_name(m_objects_model->GetName(item));
            // wxGetApp().obj_manipul()->update_warning_icon_state(get_mesh_errors_info(obj_idx, volume_id));
        }

        GLGizmosManager& gizmos_mgr = wxGetApp().plater()->canvas3D()->get_gizmos_manager();

        if (GLGizmoScale3D* scale = dynamic_cast<GLGizmoScale3D*>(gizmos_mgr.get_gizmo(GLGizmosManager::Scale)))
            scale->enable_ununiversal_scale(!disable_ununiform_scale);
    }

#if !NEW_OBJECT_SETTING
    if (update_and_show_settings)
        wxGetApp().obj_settings()->get_og()->set_name(" " + og_name + " ");
#endif

    /*if (!this->IsShown())
        update_and_show_layers = false;*/
    if (printer_technology() == ptSLA)
        update_and_show_layers = false;
    else if (update_and_show_layers) {
        ; // wxGetApp().obj_layers()->get_og()->set_name(" " + og_name + " ");
    }

    update_min_height();

    Sidebar& panel = wxGetApp().sidebar();
    panel.Freeze();

    const ItemType type = m_objects_model->GetItemType(item);
    if (!(type & itLayer)) {
        wxGetApp().plater()->canvas3D()->handle_sidebar_focus_event("", false);
    }

    // if selected item is volume, update toolbar(hide the "Support" Page)
    wxPanel* atab    = wxGetApp().params_panel()->get_current_tab();
    Tab*     cur_tab = dynamic_cast<Tab*>(atab);
    wxGetApp().plater()->get_process_toolbar().on_params_panel_tab_changed(cur_tab);

    // BBS
    // wxGetApp().obj_manipul() ->UpdateAndShow(update_and_show_manipulations);
    wxGetApp().obj_settings()->UpdateAndShow(update_and_show_settings);
    wxGetApp().obj_layers()->UpdateAndShow(update_and_show_layers);
    wxGetApp().plater()->show_object_info();

    panel.Layout();
    panel.Thaw();
}

// Add new SettingsItem for parent_item if it doesn't exist, or just update a digest according to new config
wxDataViewItem ObjectList::add_settings_item(wxDataViewItem parent_item, const DynamicPrintConfig* config)
{
    wxDataViewItem ret = wxDataViewItem(nullptr);

    if (!parent_item)
        return ret;

    const bool is_object_settings = m_objects_model->GetItemType(parent_item) == itObject;
    const bool is_volume_settings = m_objects_model->GetItemType(parent_item) == itVolume;
    const bool is_layer_settings  = m_objects_model->GetItemType(parent_item) == itLayer;
    if (!is_object_settings) {
        ModelVolumeType volume_type = m_objects_model->GetVolumeType(parent_item);
        if (volume_type == ModelVolumeType::NEGATIVE_VOLUME || volume_type == ModelVolumeType::SUPPORT_BLOCKER ||
            volume_type == ModelVolumeType::SUPPORT_ENFORCER)
            return ret;
    }

    SettingsFactory::Bundle cat_options = SettingsFactory::get_bundle(config, is_object_settings, is_layer_settings);
    if (is_layer_settings) {
        auto tab_object = dynamic_cast<TabPrintModel*>(wxGetApp().get_model_tab());
        auto object_cfg = tab_object->get_config();
        if (config->opt_float("layer_height") == object_cfg->opt_float("layer_height")) {
            SettingsFactory::Bundle new_cat_options;
            for (auto cat_opt : cat_options) {
                std::vector<string> temp;
                for (auto value : cat_opt.second) {
                    if (value != "layer_height")
                        temp.push_back(value);
                }
                if (!temp.empty())
                    new_cat_options[cat_opt.first] = temp;
            }
            cat_options = new_cat_options;
        }
    }

    if (cat_options.empty()) {
#if NEW_OBJECT_SETTING
        ObjectDataViewModelNode* node = static_cast<ObjectDataViewModelNode*>(parent_item.GetID());
        if (node)
            node->set_action_icon(false);
        m_objects_model->ItemChanged(parent_item);
        return parent_item;
#else
        return ret;
#endif
    }

    std::vector<std::string> categories;
    categories.reserve(cat_options.size());
    for (auto& cat : cat_options)
        categories.push_back(cat.first);

    if (m_objects_model->GetItemType(parent_item) & itInstance)
        parent_item = m_objects_model->GetObject(parent_item);

#if NEW_OBJECT_SETTING
    ObjectDataViewModelNode* node = static_cast<ObjectDataViewModelNode*>(parent_item.GetID());
    if (node)
        node->set_action_icon(true);
    m_objects_model->ItemChanged(parent_item);
    return parent_item;
#else
    ret = m_objects_model->IsSettingsItem(parent_item) ? parent_item : m_objects_model->GetSettingsItem(parent_item);

    if (!ret)
        ret = m_objects_model->AddSettingsChild(parent_item);

    m_objects_model->UpdateSettingsDigest(ret, categories);
    Expand(parent_item);

    return ret;
#endif
}

void ObjectList::update_info_items(size_t obj_idx, wxDataViewItemArray* selections /* = nullptr*/, bool added_object /* = false*/)
{
    // BBS
    if (obj_idx >= m_objects->size())
        return;

    const ModelObject* model_object = (*m_objects)[obj_idx];
    wxDataViewItem     item_obj     = m_objects_model->GetItemById(obj_idx);
    assert(item_obj.IsOk());

    // Cut connectors
    {
        wxDataViewItem item        = m_objects_model->GetInfoItemByType(item_obj, InfoItemType::CutConnectors);
        bool           shows       = item.IsOk();
        bool           should_show = model_object->is_cut() && model_object->has_connectors() && model_object->volumes.size() > 1;

        if (!shows && should_show) {
            m_objects_model->AddInfoChild(item_obj, InfoItemType::CutConnectors);
            Expand(item_obj);
            if (added_object)
                wxGetApp().notification_manager()->push_updated_item_info_notification(InfoItemType::CutConnectors);
        } else if (shows && !should_show) {
            if (!selections)
                Unselect(item);
            m_objects_model->Delete(item);
            if (selections) {
                if (selections->Index(item) != wxNOT_FOUND) {
                    // If info item was deleted from the list,
                    // it's need to be deleted from selection array, if it was there
                    selections->Remove(item);
                    // Select item_obj, if info_item doesn't exist for item anymore, but was selected
                    if (selections->Index(item_obj) == wxNOT_FOUND)
                        selections->Add(item_obj);
                }
            } else
                Select(item_obj);
        }
    }

    {
        bool shows       = m_objects_model->IsSupportPainted(item_obj);
        bool should_show = printer_technology() == ptFFF &&
                           std::any_of(model_object->volumes.begin(), model_object->volumes.end(),
                                       [](const ModelVolume* mv) { return !mv->supported_facets.empty(); });
        if (shows && !should_show) {
            m_objects_model->SetSupportPaintState(false, item_obj);
        } else if (!shows && should_show) {
            m_objects_model->SetSupportPaintState(true, item_obj);
        }
    }

    {
        bool shows       = m_objects_model->IsColorPainted(item_obj);
        bool should_show = printer_technology() == ptFFF &&
                           std::any_of(model_object->volumes.begin(), model_object->volumes.end(),
                                       [](const ModelVolume* mv) { return !mv->mmu_segmentation_facets.empty(); });
        if (shows && !should_show) {
            m_objects_model->SetColorPaintState(false, item_obj);
        } else if (!shows && should_show) {
            m_objects_model->SetColorPaintState(true, item_obj);
        }
    }

    {
        bool shows       = m_objects_model->IsSinked(item_obj);
        bool should_show = printer_technology() == ptFFF && wxGetApp().plater()->canvas3D()->is_object_sinking(obj_idx);
        if (shows && !should_show) {
            m_objects_model->SetSinkState(false, item_obj);
        } else if (!shows && should_show) {
            m_objects_model->SetSinkState(true, item_obj);
        }
    }

    {
        bool shows       = this->GetColumn(colSupportPaint)->IsShown();
        bool should_show = false;
        for (ModelObject* mo : *m_objects) {
            for (ModelVolume* mv : mo->volumes) {
                if (!mv->supported_facets.empty()) {
                    should_show = true;
                    break;
                }
            }
            if (should_show)
                break;
        }

        if (shows && !should_show) {
            this->set_support_paint_hidden(true);
        } else if (!shows && should_show) {
            this->set_support_paint_hidden(false);
        }
    }

    {
        bool shows       = this->GetColumn(colColorPaint)->IsShown();
        bool should_show = false;
        for (ModelObject* mo : *m_objects) {
            for (ModelVolume* mv : mo->volumes) {
                if (!mv->mmu_segmentation_facets.empty()) {
                    should_show = true;
                    break;
                }
            }
            if (should_show)
                break;
        }

        if (shows && !should_show) {
            this->set_color_paint_hidden(true);
        } else if (!shows && should_show) {
            this->set_color_paint_hidden(false);
        }
    }

    {
        bool shows       = this->GetColumn(colSinking)->IsShown();
        bool should_show = false;
        for (int i = 0; i < m_objects->size(); ++i) {
            if (wxGetApp().plater()->canvas3D()->is_object_sinking(i)) {
                should_show = true;
                break;
            }
            if (should_show)
                break;
        }

        if (shows && !should_show) {
            this->set_sinking_hidden(true);
        } else if (!shows && should_show) {
            this->set_sinking_hidden(false);
        }
    }
}

void ObjectList::add_objects_to_list(std::vector<size_t> obj_idxs, bool call_selection_changed, bool notify_partplate, bool do_info_update)
{
#ifdef __WXOSX__
    AssociateModel(nullptr);
#endif
    for (const size_t idx : obj_idxs) {
        add_object_to_list(idx, call_selection_changed, notify_partplate, do_info_update);
    }
#ifdef __WXOSX__
    AssociateModel(m_objects_model);
#endif
}

void ObjectList::add_object_to_list(size_t obj_idx, bool call_selection_changed, bool notify_partplate, bool do_info_update)
{
    auto model_object = (*m_objects)[obj_idx];
    // BBS start add obj_idx for debug
    PartPlateList& list = wxGetApp().plater()->get_partplate_list();
    if (notify_partplate) {
        list.notify_instance_update(obj_idx, 0, true);
    }
    // int plate_idx = list.find_instance_belongs(obj_idx, 0);
    // std::string item_name_str = (boost::format("[P%1%][O%2%]%3%") % plate_idx % std::to_string(obj_idx) % model_object->name).str();
    // std::string item_name_str = (boost::format("[P%1%]%2%") % plate_idx  % model_object->name).str();
    // const wxString& item_name = from_u8(item_name_str);
    const wxString& item_name      = from_u8(model_object->name);
    std::string     warning_bitmap = get_warning_icon_name(model_object->mesh().stats());
    const auto      item           = m_objects_model->AddObject(model_object, warning_bitmap, model_object->is_cut());
    Expand(m_objects_model->GetParent(item));

    if (!do_info_update)
        return;

    update_info_items(obj_idx, nullptr, call_selection_changed);

    add_volumes_to_object_in_list(obj_idx);

    // add instances to the object, if it has those
    if (model_object->instances.size() > 1) {
        std::vector<bool> print_idicator(model_object->instances.size());
        std::vector<int>  plate_idicator(model_object->instances.size());
        for (size_t i = 0; i < model_object->instances.size(); ++i) {
            print_idicator[i] = model_object->instances[i]->printable;
            plate_idicator[i] = list.find_instance_belongs(obj_idx, i);
        }

        const wxDataViewItem object_item = m_objects_model->GetItemById(obj_idx);
        m_objects_model->AddInstanceChild(object_item, print_idicator, plate_idicator);
        Expand(m_objects_model->GetInstanceRootItem(object_item));
    } else
        m_objects_model->SetPrintableState(model_object->instances[0]->printable ? piPrintable : piUnprintable, obj_idx);

    // add settings to the object, if it has those
    add_settings_item(item, &model_object->config.get());

    // Add layers if it has
    add_layer_root_item(item);

#ifndef __WXOSX__
    if (call_selection_changed) {
        //         UnselectAll();//Add new geometry without affecting the state of existing geometry
        //         Select(item);
        //         selection_changed();
    }
#endif //__WXMSW__
}

static bool can_add_volumes_to_object(const ModelObject* object)
{
    bool can = object->volumes.size() > 1;

    if (can && object->is_cut()) {
        int no_connectors_cnt = 0;
        for (const ModelVolume* v : object->volumes)
            if (!v->is_cut_connector()) {
                if (!v->is_model_part())
                    return true;
                no_connectors_cnt++;
            }
        can = no_connectors_cnt > 1;
    }

    return can;
}

wxDataViewItemArray ObjectList::add_volumes_to_object_in_list(size_t                                  obj_idx,
                                                              std::function<bool(const ModelVolume*)> add_to_selection /* = nullptr*/)
{
    const bool is_prevent_list_events = m_prevent_list_events;
    m_prevent_list_events             = true;

    wxDataViewItem object_item = m_objects_model->GetItemById(int(obj_idx));
    m_objects_model->DeleteVolumeChildren(object_item);

    wxDataViewItemArray items;

    const ModelObject* object = (*m_objects)[obj_idx];
    // add volumes to the object
    if (can_add_volumes_to_object(object)) {
        if (object->volumes.size() > 1) {
            wxString obj_item_name = from_u8(object->name);
            if (m_objects_model->GetName(object_item) != obj_item_name)
                m_objects_model->SetName(obj_item_name, object_item);
        }

        int   volume_idx{-1};
        auto& ui_and_3d_volume_map = m_objects_model->get_ui_and_3d_volume_map();
        ui_and_3d_volume_map.clear();
        int ui_volume_idx = 0;
        for (const ModelVolume* volume : object->volumes) {
            ++volume_idx;
            if (object->is_cut() && volume->is_cut_connector())
                continue;

            const wxDataViewItem& vol_item      = m_objects_model->AddVolumeChild(object_item, from_u8(volume->name), volume->type(),
                                                                                  volume->is_text(), volume->is_svg(),
                                                                                  get_warning_icon_name(volume->mesh().stats()),
                                                                             volume->config.has("extruder") ? volume->config.extruder() : 0,
                                                                                  false);
            ui_and_3d_volume_map[ui_volume_idx] = volume_idx;
            ui_volume_idx++;
            add_settings_item(vol_item, &volume->config.get());

            if (add_to_selection && add_to_selection(volume))
                items.Add(vol_item);
        }
        Expand(object_item);
    }

    m_prevent_list_events = is_prevent_list_events;
    return items;
}

void ObjectList::delete_object_from_list()
{
    auto item = GetSelection();
    if (!item)
        return;
    if (m_objects_model->GetParent(item) == wxDataViewItem(nullptr))
        select_item([this, item]() { return m_objects_model->Delete(item); });
    else
        select_item([this, item]() { return m_objects_model->Delete(m_objects_model->GetParent(item)); });
}

void ObjectList::delete_object_from_list(const size_t obj_idx)
{
    select_item([this, obj_idx]() { return m_objects_model->Delete(m_objects_model->GetItemById(obj_idx)); });
}

void ObjectList::delete_volume_from_list(const size_t obj_idx, const size_t vol_idx)
{
    select_item([this, obj_idx, vol_idx]() { return m_objects_model->Delete(m_objects_model->GetItemByVolumeId(obj_idx, vol_idx)); });
}

void ObjectList::delete_instance_from_list(const size_t obj_idx, const size_t inst_idx)
{
    select_item([this, obj_idx, inst_idx]() { return m_objects_model->Delete(m_objects_model->GetItemByInstanceId(obj_idx, inst_idx)); });
}

void ObjectList::delete_from_model_and_list(const ItemType type, const int obj_idx, const int sub_obj_idx)
{
    if (!(type & (itObject | itVolume | itInstance)))
        return;

    take_snapshot("Delete selected");

    if (type & itObject) {
        bool was_cut = object(obj_idx)->is_cut();
        if (del_object(obj_idx)) {
            delete_object_from_list(obj_idx);
            if (was_cut)
                update_lock_icons_for_model();
        }
    } else {
        del_subobject_from_object(obj_idx, sub_obj_idx, type);

        type == itVolume ? delete_volume_from_list(obj_idx, sub_obj_idx) : delete_instance_from_list(obj_idx, sub_obj_idx);
    }
}

void ObjectList::delete_from_model_and_list(const std::vector<ItemForDelete>& items_for_delete)
{
    if (items_for_delete.empty())
        return;

    m_prevent_list_events = true;
    // BBS
    bool need_update = false;

    std::set<size_t> modified_objects_ids;
    for (std::vector<ItemForDelete>::const_reverse_iterator item = items_for_delete.rbegin(); item != items_for_delete.rend(); ++item) {
        if (!(item->type & (itObject | itVolume | itInstance)))
            continue;
        if (item->type & itObject) {
            // refresh after del_object
            need_update              = true;
            bool refresh_immediately = false;
            bool was_cut             = object(item->obj_idx)->is_cut();
            if (!del_object(item->obj_idx, refresh_immediately))
                return;
            m_objects_model->Delete(m_objects_model->GetItemById(item->obj_idx));
            if (was_cut)
                update_lock_icons_for_model();
        } else {
            if (!del_subobject_from_object(item->obj_idx, item->sub_obj_idx, item->type))
                return; // continue;
            if (item->type & itVolume) {
                m_objects_model->Delete(m_objects_model->GetItemByVolumeId(item->obj_idx, item->sub_obj_idx));
                // BBS
#if 0
                ModelObject* obj = object(item->obj_idx);
                if (obj->volumes.size() == 1) {
                    wxDataViewItem parent = m_objects_model->GetItemById(item->obj_idx);
                    if (obj->config.has("extruder")) {
                        const wxString extruder = wxString::Format("%d", obj->config.extruder());
                        m_objects_model->SetExtruder(extruder, parent);
                    }
                    // If last volume item with warning was deleted, unmark object item
                    m_objects_model->UpdateWarningIcon(parent, get_warning_icon_name(obj->get_object_stl_stats()));
                }
#endif
                wxGetApp().plater()->canvas3D()->ensure_on_bed(item->obj_idx, printer_technology() != ptSLA);
            } else
                m_objects_model->Delete(m_objects_model->GetItemByInstanceId(item->obj_idx, item->sub_obj_idx));
        }

        modified_objects_ids.insert(static_cast<size_t>(item->obj_idx));
    }

    if (need_update) {
        wxGetApp().plater()->update();
        wxGetApp().plater()->object_list_changed();
    }

    for (size_t id : modified_objects_ids) {
        update_info_items(id);
    }

    m_prevent_list_events = true;
    part_selection_changed();
}

void ObjectList::update_lock_icons_for_model()
{
    // update the icon for cut object
    for (size_t obj_idx = 0; obj_idx < (*m_objects).size(); ++obj_idx)
        if (!(*m_objects)[obj_idx]->is_cut())
            m_objects_model->UpdateCutObjectIcon(m_objects_model->GetItemById(int(obj_idx)), false);
}

void ObjectList::delete_all_objects_from_list()
{
    m_prevent_list_events = true;
    reload_all_plates();
    m_prevent_list_events = false;
    part_selection_changed();
}

void ObjectList::increase_object_instances(const size_t obj_idx, const size_t num)
{
    select_item([this, obj_idx, num]() { return m_objects_model->AddInstanceChild(m_objects_model->GetItemById(obj_idx), num); });
    selection_changed();
}

void ObjectList::decrease_object_instances(const size_t obj_idx, const size_t num)
{
    select_item([this, obj_idx, num]() { return m_objects_model->DeleteLastInstance(m_objects_model->GetItemById(obj_idx), num); });
}

void ObjectList::unselect_objects()
{
    if (!GetSelection())
        return;

    m_prevent_list_events = true;
    UnselectAll();
    part_selection_changed();
    m_prevent_list_events = false;
}

void ObjectList::select_object_item(bool is_msr_gizmo)
{
    if (wxDataViewItem item = GetSelection()) {
        ItemType type           = m_objects_model->GetItemType(item);
        bool     is_volume_item = type == itVolume ||
                              (type == itSettings && m_objects_model->GetItemType(m_objects_model->GetParent(item)) == itVolume);
        if ((is_msr_gizmo && is_volume_item) || type == itObject)
            return;

        if (wxDataViewItem obj_item = m_objects_model->GetTopParent(item)) {
            m_prevent_list_events = true;
            UnselectAll();
            Select(obj_item);
            part_selection_changed();
            m_prevent_list_events = false;
        }
    }
}

static void update_selection(wxDataViewItemArray& sels, ObjectList::SELECTION_MODE mode, ObjectDataViewModel* model)
{
    if (mode == ObjectList::smInstance) {
        for (auto& item : sels) {
            ItemType type = model->GetItemType(item);
            if (type == itObject)
                continue;
            if (type == itInstanceRoot) {
                wxDataViewItem obj_item = model->GetParent(item);
                sels.Remove(item);
                sels.Add(obj_item);
                update_selection(sels, mode, model);
                return;
            }
            if (type == itInstance) {
                wxDataViewItemArray instances;
                model->GetChildren(model->GetParent(item), instances);
                assert(instances.Count() > 0);
                size_t selected_instances_cnt = 0;
                for (auto& inst : instances) {
                    if (sels.Index(inst) == wxNOT_FOUND)
                        break;
                    selected_instances_cnt++;
                }

                if (selected_instances_cnt == instances.Count()) {
                    wxDataViewItem obj_item = model->GetObject(item);
                    for (auto& inst : instances)
                        sels.Remove(inst);
                    sels.Add(obj_item);
                    update_selection(sels, mode, model);
                    return;
                }
            } else
                return;
        }
    }
}

void ObjectList::remove()
{
    if (GetSelectedItemsCount() == 0)
        return;

    auto delete_item = [this](wxDataViewItem item) {
        wxDataViewItem parent = m_objects_model->GetParent(item);
        ItemType       type   = m_objects_model->GetItemType(item);
        if (type & itObject)
            delete_from_model_and_list(itObject, m_objects_model->GetIdByItem(item), -1);
        else {
            if (type & (itLayer | itInstance)) {
                // In case there is just one layer or two instances and we delete it, del_subobject_item will
                // also remove the parent item. Selection should therefore pass to the top parent (object).
                wxDataViewItemArray children;
                if (m_objects_model->GetChildren(parent, children) == (type & itLayer ? 1 : 2))
                    parent = m_objects_model->GetObject(item);
            }

            del_subobject_item(item);
        }

        return parent;
    };

    wxDataViewItemArray sels;
    GetSelections(sels);

    wxDataViewItem parent = wxDataViewItem(nullptr);

    if (sels.Count() == 1)
        parent = delete_item(GetSelection());
    else {
        SELECTION_MODE sels_mode = m_selection_mode;
        UnselectAll();
        update_selection(sels, sels_mode, m_objects_model);

        Plater::TakeSnapshot snapshot = Plater::TakeSnapshot(wxGetApp().plater(), "Delete selected");

        for (auto& item : sels) {
            if (m_objects_model->InvalidItem(item)) // item can be deleted for this moment (like last 2 Instances or Volumes)
                continue;
            parent = delete_item(item);
        }
    }

    if (parent && !m_objects_model->InvalidItem(parent)) {
        select_item(parent);
        update_selections_on_canvas();
    }
}

void ObjectList::del_layer_range(const t_layer_height_range& range)
{
    const int obj_idx = get_selected_obj_idx();
    if (obj_idx < 0)
        return;

    t_layer_config_ranges& ranges = object(obj_idx)->layer_config_ranges;

    wxDataViewItem selectable_item = GetSelection();

    if (ranges.size() == 1)
        selectable_item = m_objects_model->GetParent(selectable_item);

    wxDataViewItem layer_item = m_objects_model->GetItemByLayerRange(obj_idx, range);
    del_subobject_item(layer_item);

    select_item(selectable_item);
}

static double get_min_layer_height(const int extruder_idx)
{
    const DynamicPrintConfig& config = wxGetApp().preset_bundle->printers.get_edited_preset().config;
    return config.opt_float("min_layer_height", std::max(0, extruder_idx - 1));
}

static double get_max_layer_height(const int extruder_idx)
{
    const DynamicPrintConfig& config                  = wxGetApp().preset_bundle->printers.get_edited_preset().config;
    int                       extruder_idx_zero_based = std::max(0, extruder_idx - 1);
    double                    max_layer_height        = config.opt_float("max_layer_height", extruder_idx_zero_based);

    // In case max_layer_height is set to zero, it should default to 75 % of nozzle diameter:
    if (max_layer_height < EPSILON)
        max_layer_height = 0.75 * config.opt_float("nozzle_diameter", extruder_idx_zero_based);

    return max_layer_height;
}

// When editing this function, please synchronize the conditions with can_add_new_range_after_current().
void ObjectList::add_layer_range_after_current(const t_layer_height_range current_range)
{
    const int obj_idx = get_selected_obj_idx();
    assert(obj_idx >= 0);
    if (obj_idx < 0)
        // This should not happen.
        return;

    const wxDataViewItem layers_item = GetSelection();

    auto& ranges   = object(obj_idx)->layer_config_ranges;
    auto  it_range = ranges.find(current_range);
    assert(it_range != ranges.end());
    if (it_range == ranges.end())
        // This shoudl not happen.
        return;

    auto it_next_range = it_range;
    bool changed       = false;
    if (++it_next_range == ranges.end()) {
        // Adding a new layer height range after the last one.
        // BBS: remove snapshot name "Add Height Range"
        take_snapshot("");
        changed = true;

        const t_layer_height_range new_range = {current_range.second, current_range.second + 2.};
        ranges[new_range].assign_config(get_default_layer_config(obj_idx));
        add_layer_item(new_range, layers_item);
    } else if (const std::pair<coordf_t, coordf_t>& next_range = it_next_range->first; current_range.second <= next_range.first) {
        const int layer_idx = m_objects_model->GetItemIdByLayerRange(obj_idx, next_range);
        assert(layer_idx >= 0);
        if (layer_idx >= 0) {
            if (current_range.second == next_range.first) {
                // Splitting the next layer height range to two.
                const auto     old_config = ranges.at(next_range);
                const coordf_t delta      = next_range.second - next_range.first;
                // Layer height of the current layer.
                const coordf_t old_min_layer_height = get_min_layer_height(old_config.opt_int("extruder"));
                // Layer height of the layer to be inserted.
                const coordf_t new_min_layer_height = get_min_layer_height(0);
                if (delta >= old_min_layer_height + new_min_layer_height - EPSILON) {
                    const coordf_t       middle_layer_z = (new_min_layer_height > 0.5 * delta) ?
                                                              next_range.second - new_min_layer_height :
                                                              next_range.first + std::max(old_min_layer_height, 0.5 * delta);
                    t_layer_height_range new_range      = {middle_layer_z, next_range.second};

                    // BBS: remove snapshot name "Add Height Range"
                    Plater::TakeSnapshot snapshot(wxGetApp().plater(), "");
                    changed = true;

                    // create new 2 layers instead of deleted one
                    // delete old layer

                    wxDataViewItem layer_item = m_objects_model->GetItemByLayerRange(obj_idx, next_range);
                    del_subobject_item(layer_item);

                    ranges[new_range] = old_config;
                    add_layer_item(new_range, layers_item, layer_idx);

                    new_range = {current_range.second, middle_layer_z};
                    ranges[new_range].assign_config(get_default_layer_config(obj_idx));
                    add_layer_item(new_range, layers_item, layer_idx);
                }
            } else if (next_range.first - current_range.second >= get_min_layer_height(0) - EPSILON) {
                // Filling in a gap between the current and a new layer height range with a new one.
                // BBS: remove snapshot name "Add Height Range"
                take_snapshot("");
                changed = true;

                const t_layer_height_range new_range = {current_range.second, next_range.first};
                ranges[new_range].assign_config(get_default_layer_config(obj_idx));
                add_layer_item(new_range, layers_item, layer_idx);
            }
        }
    }

    if (changed)
        changed_object(obj_idx);

    // The layer range panel is updated even if this function does not change the layer ranges, as the panel update
    // may have been postponed from the "kill focus" event of a text field, if the focus was lost for the "add layer" button.
    // select item to update layers sizer
    select_item(layers_item);
}

// Returning an empty string means that the layer could be added after the current layer.
// Otherwise an error tooltip is returned.
// When editing this function, please synchronize the conditions with add_layer_range_after_current().
wxString ObjectList::can_add_new_range_after_current(const t_layer_height_range current_range)
{
    const int obj_idx = get_selected_obj_idx();
    assert(obj_idx >= 0);
    if (obj_idx < 0)
        // This should not happen.
        return "ObjectList assert";

    auto& ranges   = object(obj_idx)->layer_config_ranges;
    auto  it_range = ranges.find(current_range);
    assert(it_range != ranges.end());
    if (it_range == ranges.end())
        // This shoudl not happen.
        return "ObjectList assert";

    auto it_next_range = it_range;
    if (++it_next_range == ranges.end())
        // Adding a layer after the last layer is always possible.
        return "";

    // BBS: remove all layer range message

    // All right, new layer height range could be inserted.
    return "";
}

void ObjectList::add_layer_item(const t_layer_height_range& range, const wxDataViewItem layers_item, const int layer_idx /* = -1*/)
{
    const int obj_idx = m_objects_model->GetObjectIdByItem(layers_item);
    if (obj_idx < 0)
        return;

    const DynamicPrintConfig& config = object(obj_idx)->layer_config_ranges[range].get();
    if (!config.has("extruder"))
        return;

    const auto layer_item = m_objects_model->AddLayersChild(layers_item, range, config.opt_int("extruder"), layer_idx);
    add_settings_item(layer_item, &config);
}

bool ObjectList::edit_layer_range(const t_layer_height_range& range, coordf_t layer_height)
{
    // Use m_selected_object_id instead of get_selected_obj_idx()
    // because of get_selected_obj_idx() return obj_idx for currently selected item.
    // But edit_layer_range(...) function can be called, when Selection in ObjectList could be changed
    const int obj_idx = m_selected_object_id;
    if (obj_idx < 0)
        return false;

    ModelConfig* config = &object(obj_idx)->layer_config_ranges[range];
    if (fabs(layer_height - config->opt_float("layer_height")) < EPSILON)
        return false;

    const int extruder_idx = config->opt_int("extruder");

    if (layer_height >= get_min_layer_height(extruder_idx) && layer_height <= get_max_layer_height(extruder_idx)) {
        config->set_key_value("layer_height", new ConfigOptionFloat(layer_height));
        changed_object(obj_idx);
        return true;
    }

    return false;
}

bool ObjectList::edit_layer_range(const t_layer_height_range& range, const t_layer_height_range& new_range, bool dont_update_ui)
{
    // Use m_selected_object_id instead of get_selected_obj_idx()
    // because of get_selected_obj_idx() return obj_idx for currently selected item.
    // But edit_layer_range(...) function can be called, when Selection in ObjectList could be changed
    const int obj_idx = m_selected_object_id;
    if (obj_idx < 0)
        return false;

    // BBS: remove snapeshot name "Edit Height Range"
    take_snapshot("");

    const ItemType sel_type = m_objects_model->GetItemType(GetSelection());

    auto& ranges = object(obj_idx)->layer_config_ranges;

    {
        ModelConfig config = std::move(ranges[range]);
        ranges.erase(range);
        ranges[new_range] = std::move(config);
    }

    changed_object(obj_idx);

    wxDataViewItem root_item = m_objects_model->GetLayerRootItem(m_objects_model->GetItemById(obj_idx));
    // To avoid update selection after deleting of a selected item (under GTK)
    // set m_prevent_list_events to true
    m_prevent_list_events = true;
    m_objects_model->DeleteChildren(root_item);

    if (root_item.IsOk()) {
        // create Layer item(s) according to the layer_config_ranges
        for (const auto& r : ranges)
            add_layer_item(r.first, root_item);
    }

    // if this function was invoked from wxEVT_CHANGE_SELECTION selected item could be other than itLayer or itLayerRoot
    if (!dont_update_ui && (sel_type & (itLayer | itLayerRoot)))
        select_item(sel_type & itLayer ? m_objects_model->GetItemByLayerRange(obj_idx, new_range) : root_item);

    Expand(root_item);

    m_prevent_list_events = false;
    return true;
}

void ObjectList::init()
{
    m_objects_model->Init();
    m_objects = &wxGetApp().model().objects;
}

bool ObjectList::multiple_selection() const { return GetSelectedItemsCount() > 1; }

bool ObjectList::is_selected(const ItemType type) const
{
    const wxDataViewItem& item = GetSelection();
    if (item)
        return m_objects_model->GetItemType(item) == type;

    return false;
}

bool ObjectList::is_connectors_item_selected() const
{
    const wxDataViewItem& item = GetSelection();
    if (item)
        return m_objects_model->GetItemType(item) == itInfo && m_objects_model->GetInfoItemType(item) == InfoItemType::CutConnectors;

    return false;
}

bool ObjectList::is_connectors_item_selected(const wxDataViewItemArray& sels) const
{
    for (auto item : sels)
        if (m_objects_model->GetItemType(item) == itInfo && m_objects_model->GetInfoItemType(item) == InfoItemType::CutConnectors)
            return true;

    return false;
}

int ObjectList::get_selected_layers_range_idx() const
{
    const wxDataViewItem& item = GetSelection();
    if (!item)
        return -1;

    const ItemType type = m_objects_model->GetItemType(item);
    if (type & itSettings && m_objects_model->GetItemType(m_objects_model->GetParent(item)) != itLayer)
        return -1;

    return m_objects_model->GetLayerIdByItem(type & itLayer ? item : m_objects_model->GetParent(item));
}

void ObjectList::update_selections()
{
    const Selection&    selection = scene_selection();
    wxDataViewItemArray sels;

#if 0
    if (m_selection_mode == smUndef) {
        PartPlate* pp = wxGetApp().plater()->get_partplate_list().get_selected_plate();
        assert(pp != nullptr);
        wxDataViewItem sel_plate = m_objects_model->GetItemByPlateId(pp->get_index());
        sels.Add(sel_plate);
        select_items(sels);

        // Scroll selected Item in the middle of an object list
        ensure_current_item_visible();
        return;
    }
#endif

    if ((m_selection_mode & (smSettings | smLayer | smLayerRoot | smVolume)) == 0)
        m_selection_mode = smInstance;

    // We doesn't update selection if itSettings | itLayerRoot | itLayer Item for the current object/part is selected
    if (GetSelectedItemsCount() == 1 && m_objects_model->GetItemType(GetSelection()) & (itSettings | itLayerRoot | itLayer)) {
        const auto item = GetSelection();
        if (selection.is_single_full_object()) {
            if (m_objects_model->GetItemType(m_objects_model->GetParent(item)) & itObject &&
                m_objects_model->GetObjectIdByItem(item) == selection.get_object_idx())
                return;
            sels.Add(m_objects_model->GetItemById(selection.get_object_idx()));
        } else if (selection.is_single_volume() || selection.is_any_modifier()) {
            const auto gl_vol = selection.get_first_volume();
            if (m_objects_model->GetVolumeIdByItem(m_objects_model->GetParent(item)) == gl_vol->volume_idx())
                return;
        }
        // but if there is selected only one of several instances by context menu,
        // then select this instance in ObjectList
        else if (selection.is_single_full_instance())
            sels.Add(m_objects_model->GetItemByInstanceId(selection.get_object_idx(), selection.get_instance_idx()));
        // Can be the case, when we have selected itSettings | itLayerRoot | itLayer in the ObjectList and selected object/instance in the
        // Scene and then select some object/instance in 3DScene using Ctrt+left click
        else {
            // Unselect all items in ObjectList
            m_last_selected_item  = wxDataViewItem(nullptr);
            m_prevent_list_events = true;
            UnselectAll();
            m_prevent_list_events = false;
            // call this function again to update selection from the canvas
            update_selections();
            return;
        }
    } else if (selection.is_single_full_object() || selection.is_multiple_full_object()) {
        const Selection::ObjectIdxsToInstanceIdxsMap& objects_content = selection.get_content();
        // it's impossible to select Settings, Layer or LayerRoot for several objects
        if (!selection.is_multiple_full_object() && (m_selection_mode & (smSettings | smLayer | smLayerRoot))) {
            auto           obj_idx  = objects_content.begin()->first;
            wxDataViewItem obj_item = m_objects_model->GetItemById(obj_idx);
            if (m_selection_mode & smSettings) {
                if (m_selected_layers_range_idx < 0)
                    sels.Add(m_objects_model->GetSettingsItem(obj_item));
                else
                    sels.Add(m_objects_model->GetSettingsItem(m_objects_model->GetItemByLayerId(obj_idx, m_selected_layers_range_idx)));
            } else if (m_selection_mode & smLayerRoot)
                sels.Add(m_objects_model->GetLayerRootItem(obj_item));
            else if (m_selection_mode & smLayer) {
                if (m_selected_layers_range_idx >= 0)
                    sels.Add(m_objects_model->GetItemByLayerId(obj_idx, m_selected_layers_range_idx));
                else
                    sels.Add(obj_item);
            }
        } else {
            for (const auto& object : objects_content) {
                if (object.second.size() == 1) // object with 1 instance
                    sels.Add(m_objects_model->GetItemById(object.first));
                else if (object.second.size() > 1) // object with several instances
                {
                    wxDataViewItemArray current_sels;
                    GetSelections(current_sels);
                    const wxDataViewItem frst_inst_item = m_objects_model->GetItemByInstanceId(object.first, 0);

                    bool root_is_selected = false;
                    for (const auto& item : current_sels)
                        if (item == m_objects_model->GetParent(frst_inst_item) || item == m_objects_model->GetObject(frst_inst_item)) {
                            root_is_selected = true;
                            sels.Add(item);
                            break;
                        }
                    if (root_is_selected)
                        continue;

                    const Selection::InstanceIdxsList& instances = object.second;
                    for (const auto& inst : instances)
                        sels.Add(m_objects_model->GetItemByInstanceId(object.first, inst));
                }
            }
        }
    } else if (selection.is_any_volume() || selection.is_any_modifier()) {
        if (m_selection_mode & smSettings) {
            const auto gl_vol = selection.get_first_volume();
            if (gl_vol->volume_idx() >= 0) {
                // Only add GLVolumes with non-negative volume_ids. GLVolumes with negative volume ids
                // are not associated with ModelVolumes, but they are temporarily generated by the backend
                // (for example, SLA supports or SLA pad).
                wxDataViewItem vol_item = m_objects_model->GetItemByVolumeId(gl_vol->object_idx(), gl_vol->volume_idx());
                sels.Add(m_objects_model->GetSettingsItem(vol_item));
            }
        } else {
            for (auto idx : selection.get_volume_idxs()) {
                const auto gl_vol = selection.get_volume(idx);
                if (gl_vol->volume_idx() >= 0) {
                    // Only add GLVolumes with non-negative volume_ids. GLVolumes with negative volume ids
                    // are not associated with ModelVolumes, but they are temporarily generated by the backend
                    // (for example, SLA supports or SLA pad).
                    int obj_idx = gl_vol->object_idx();
                    int vol_idx = gl_vol->volume_idx();
                    assert(obj_idx >= 0 && vol_idx >= 0);
                    if (object(obj_idx)->volumes[vol_idx]->is_cut_connector())
                        sels.Add(m_objects_model->GetInfoItemByType(m_objects_model->GetItemById(obj_idx), InfoItemType::CutConnectors));
                    else {
                        vol_idx = m_objects_model->get_real_volume_index_in_ui(vol_idx);
                        sels.Add(m_objects_model->GetItemByVolumeId(obj_idx, vol_idx));
                    }
                }
            }
            m_selection_mode = smVolume;
        }
    } else if (selection.is_single_full_instance() || selection.is_multiple_full_instance()) {
        for (auto idx : selection.get_instance_idxs()) {
            sels.Add(m_objects_model->GetItemByInstanceId(selection.get_object_idx(), idx));
        }
    } else if (selection.is_mixed()) {
        const Selection::ObjectIdxsToInstanceIdxsMap& objects_content_list = selection.get_content();

        for (auto idx : selection.get_volume_idxs()) {
            const auto  gl_vol      = selection.get_volume(idx);
            const auto& glv_obj_idx = gl_vol->object_idx();
            const auto& glv_ins_idx = gl_vol->instance_idx();

            bool is_selected = false;

            for (auto obj_ins : objects_content_list) {
                if (obj_ins.first == glv_obj_idx) {
                    if (obj_ins.second.find(glv_ins_idx) != obj_ins.second.end() &&
                        !selection.is_from_single_instance()) // a case when volumes of different types are selected
                    {
                        if (glv_ins_idx == 0 && (*m_objects)[glv_obj_idx]->instances.size() == 1)
                            sels.Add(m_objects_model->GetItemById(glv_obj_idx));
                        else
                            sels.Add(m_objects_model->GetItemByInstanceId(glv_obj_idx, glv_ins_idx));

                        is_selected = true;
                        break;
                    }
                }
            }

            if (is_selected)
                continue;

            const auto& glv_vol_idx = gl_vol->volume_idx();
            if (glv_vol_idx == 0 && (*m_objects)[glv_obj_idx]->volumes.size() == 1)
                sels.Add(m_objects_model->GetItemById(glv_obj_idx));
            else
                sels.Add(m_objects_model->GetItemByVolumeId(glv_obj_idx, glv_vol_idx));
        }
    }

    if (sels.size() == 0 || m_selection_mode & smSettings)
        m_selection_mode = smUndef;

    if (fix_cut_selection(sels) || is_connectors_item_selected(sels)) {
        m_prevent_list_events = true;

        // If some part is selected, unselect all items except of selected parts of the current object
        UnselectAll();
        SetSelections(sels);

        m_prevent_list_events = false;

        // update object selection on Plater
        if (!m_prevent_canvas_selection_update)
            update_selections_on_canvas();

        // to update the toolbar and info sizer
        if (!GetSelection() || m_objects_model->GetItemType(GetSelection()) == itObject || is_connectors_item_selected()) {
            auto event = SimpleEvent(EVT_OBJ_LIST_OBJECT_SELECT);
            event.SetEventObject(this);
            wxPostEvent(this, event);
        }
        part_selection_changed();
    } else {
        select_items(sels);

        // Scroll selected Item in the middle of an object list
        ensure_current_item_visible();
    }
}

void ObjectList::update_selections_on_canvas()
{
    auto        canvas_type = wxGetApp().plater()->get_current_canvas3D()->get_canvas_type();
    GLCanvas3D* canvas      = canvas_type == GLCanvas3D::ECanvasType::CanvasAssembleView ? wxGetApp().plater()->get_current_canvas3D() :
                                                                                           wxGetApp().plater()->get_view3D_canvas3D();
    Selection&  selection   = canvas->get_selection();

    const int sel_cnt = GetSelectedItemsCount();
    if (sel_cnt == 0) {
        selection.remove_all();
        if (canvas_type != GLCanvas3D::ECanvasType::CanvasPreview)
            wxGetApp().plater()->get_current_canvas3D()->update_gizmos_on_off_state();
        return;
    }

    std::vector<unsigned int> volume_idxs;
    Selection::EMode          mode             = Selection::Volume;
    bool                      single_selection = sel_cnt == 1;
    auto add_to_selection = [this, &volume_idxs, &single_selection](const wxDataViewItem& item, const Selection& selection,
                                                                    int instance_idx, Selection::EMode& mode) {
        const ItemType& type    = m_objects_model->GetItemType(item);
        const int       obj_idx = m_objects_model->GetObjectIdByItem(item);

        if (type == itVolume) {
            int vol_idx                    = m_objects_model->GetVolumeIdByItem(item);
            vol_idx                        = m_objects_model->get_real_volume_index_in_3d(vol_idx);
            std::vector<unsigned int> idxs = selection.get_volume_idxs_from_volume(obj_idx, std::max(instance_idx, 0), vol_idx);
            volume_idxs.insert(volume_idxs.end(), idxs.begin(), idxs.end());
        } else if (type == itInstance) {
            const int inst_idx             = m_objects_model->GetInstanceIdByItem(item);
            mode                           = Selection::Instance;
            std::vector<unsigned int> idxs = selection.get_volume_idxs_from_instance(obj_idx, inst_idx);
            volume_idxs.insert(volume_idxs.end(), idxs.begin(), idxs.end());
        } else if (type == itInfo) {
            if (m_objects_model->GetInfoItemType(item) == InfoItemType::CutConnectors) {
                mode = Selection::Volume;

                // When selecting CutConnectors info item, select all object volumes, which are marked as a connector
                const ModelObject* obj = object(obj_idx);
                for (unsigned int vol_idx = 0; vol_idx < obj->volumes.size(); vol_idx++)
                    if (obj->volumes[vol_idx]->is_cut_connector()) {
                        std::vector<unsigned int> idxs = selection.get_volume_idxs_from_volume(obj_idx, std::max(instance_idx, 0), vol_idx);
                        volume_idxs.insert(volume_idxs.end(), idxs.begin(), idxs.end());
                    }
            } else {
                // When selecting an info item, select one instance of the
                // respective object - a gizmo may want to be opened.
                int inst_idx      = selection.get_instance_idx();
                int scene_obj_idx = selection.get_object_idx();
                mode              = Selection::Instance;
                // select first instance, unless an instance of the object is already selected
                if (scene_obj_idx == -1 || inst_idx == -1 || scene_obj_idx != obj_idx)
                    inst_idx = 0;
                std::vector<unsigned int> idxs = selection.get_volume_idxs_from_instance(obj_idx, inst_idx);
                volume_idxs.insert(volume_idxs.end(), idxs.begin(), idxs.end());
            }
        } else {
            mode = Selection::Instance;
            single_selection &= (obj_idx != selection.get_object_idx());
            std::vector<unsigned int> idxs = selection.get_volume_idxs_from_object(obj_idx);
            volume_idxs.insert(volume_idxs.end(), idxs.begin(), idxs.end());
        }
    };

    // stores current instance idx before to clear the selection
    int instance_idx = selection.get_instance_idx();

    if (sel_cnt == 1) {
        wxDataViewItem item = GetSelection();
        if (m_objects_model->GetInfoItemType(item) == InfoItemType::CutConnectors)
            selection.remove_all();

        if (m_objects_model->GetItemType(item) & (itSettings | itInstanceRoot | itLayerRoot | itLayer))
            add_to_selection(m_objects_model->GetParent(item), selection, instance_idx, mode);
        else
            add_to_selection(item, selection, instance_idx, mode);
    } else {
        wxDataViewItemArray sels;
        GetSelections(sels);

        // clear selection before adding new elements
        selection.clear(); // OR remove_all()?

        for (auto item : sels) {
            add_to_selection(item, selection, instance_idx, mode);
        }
    }

    if (selection.contains_all_volumes(volume_idxs)) {
        // remove
        volume_idxs = selection.get_missing_volume_idxs_from(volume_idxs);
        if (volume_idxs.size() > 0) {
            Plater::TakeSnapshot snapshot(wxGetApp().plater(), "Remove selected from list", UndoRedo::SnapshotType::Selection);
            selection.remove_volumes(mode, volume_idxs);
        }
    } else {
        // add
        // to avoid lost of some volumes in selection
        // check non-selected volumes only if selection mode wasn't changed
        // OR there is no single selection
        if (selection.get_mode() == mode || !single_selection)
            volume_idxs = selection.get_unselected_volume_idxs_from(volume_idxs);
        Plater::TakeSnapshot snapshot(wxGetApp().plater(), "Add selected to list", UndoRedo::SnapshotType::Selection);
        selection.add_volumes(mode, volume_idxs, single_selection);
    }

    if (canvas_type != GLCanvas3D::ECanvasType::CanvasPreview)
        wxGetApp().plater()->get_current_canvas3D()->on_object_list_selected();
    wxGetApp().plater()->canvas3D()->render();
}

void ObjectList::select_item(const wxDataViewItem& item)
{
    if (!item.IsOk()) {
        return;
    }

    m_prevent_list_events = true;

    UnselectAll();
    Select(item);
    part_selection_changed();

    m_prevent_list_events = false;
}

void ObjectList::select_item(std::function<wxDataViewItem()> get_item)
{
    if (!get_item)
        return;

    m_prevent_list_events = true;

    wxDataViewItem item = get_item();
    if (item.IsOk()) {
        UnselectAll();
        Select(item);
        part_selection_changed();
    }

    m_prevent_list_events = false;
}

// BBS
void ObjectList::select_item(const ObjectVolumeID& ov_id)
{
    std::vector<ObjectVolumeID> ov_ids;
    ov_ids.push_back(ov_id);
    select_items(ov_ids);
}

void ObjectList::select_items(const std::vector<ObjectVolumeID>& ov_ids)
{
    ModelObjectPtrs& objects = wxGetApp().model().objects;

    wxDataViewItemArray sel_items;
    for (auto ov_id : ov_ids) {
        if (ov_id.object == nullptr)
            continue;

        ModelObject* mo = ov_id.object;
        ModelVolume* mv = ov_id.volume;

        wxDataViewItem obj_item = m_objects_model->GetObjectItem(mo);
        if (mv != nullptr) {
            size_t vol_idx;
            for (vol_idx = 0; vol_idx < mo->volumes.size(); vol_idx++) {
                if (mo->volumes[vol_idx] == mv)
                    break;
            }
            assert(vol_idx < mo->volumes.size());

            wxDataViewItem vol_item = m_objects_model->GetVolumeItem(obj_item, vol_idx);
            if (vol_item.GetID() != nullptr) {
                sel_items.push_back(vol_item);
            } else {
                sel_items.push_back(obj_item);
            }
        } else {
            sel_items.push_back(obj_item);
        }
    }

    select_items(sel_items);
    selection_changed();
}

void ObjectList::select_items(const wxDataViewItemArray& sels)
{
    m_prevent_list_events = true;
    m_last_selected_item  = sels.empty() ? wxDataViewItem(nullptr) : sels.back();

    UnselectAll();

    if (!sels.empty()) {
        SetSelections(sels);
    } else {
        int curr_plate_idx = wxGetApp().plater()->get_partplate_list().get_curr_plate_index();
        on_plate_selected(curr_plate_idx);
    }

    part_selection_changed();

    m_prevent_list_events = false;
}

void ObjectList::select_all()
{
    SelectAll();
    selection_changed();
}

void ObjectList::select_item_all_children()
{
    wxDataViewItemArray sels;

    // There is no selection before OR some object is selected   =>  select all objects
    if (!GetSelection() || m_objects_model->GetItemType(GetSelection()) == itObject) {
        for (size_t i = 0; i < m_objects->size(); i++)
            sels.Add(m_objects_model->GetItemById(i));
        m_selection_mode = smInstance;
    } else {
        const auto     item      = GetSelection();
        const ItemType item_type = m_objects_model->GetItemType(item);
        // Some volume/layer/instance is selected    =>  select all volumes/layers/instances inside the current object
        if (item_type & (itVolume | itInstance | itLayer))
            m_objects_model->GetChildren(m_objects_model->GetParent(item), sels);

        m_selection_mode = item_type & itVolume ? smVolume : item_type & itLayer ? smLayer : smInstance;
    }

    SetSelections(sels);
    selection_changed();
}

// update selection mode for non-multiple selection
void ObjectList::update_selection_mode()
{
    m_selected_layers_range_idx = -1;
    // All items are unselected
    if (!GetSelection()) {
        m_last_selected_item = wxDataViewItem(nullptr);
        m_selection_mode     = smUndef;
        return;
    }

    const ItemType type = m_objects_model->GetItemType(GetSelection());
    m_selection_mode    = type & itSettings ? smUndef : type & itLayer ? smLayer : type & itVolume ? smVolume : smInstance;
}

// check last selected item. If is it possible to select it
bool ObjectList::check_last_selection(wxString& msg_str)
{
    if (!m_last_selected_item)
        return true;

    const bool is_shift_pressed = wxGetKeyState(WXK_SHIFT);

    /* We can't mix Volumes, Layers and Objects/Instances.
     * So, show information about it
     */
    const ItemType type = m_objects_model->GetItemType(m_last_selected_item);

    // check a case of a selection of the same type items from different Objects
    auto impossible_multi_selection = [type, this](const ItemType item_type, const SELECTION_MODE selection_mode) {
        if (!(type & item_type && m_selection_mode & selection_mode))
            return false;

        wxDataViewItemArray sels;
        GetSelections(sels);
        for (const auto& sel : sels)
            if (sel != m_last_selected_item && m_objects_model->GetObject(sel) != m_objects_model->GetObject(m_last_selected_item))
                return true;

        return false;
    };

    if (impossible_multi_selection(itVolume, smVolume) || impossible_multi_selection(itLayer, smLayer) || type & itSettings ||
        (type & itVolume && !(m_selection_mode & smVolume)) || (type & itLayer && !(m_selection_mode & smLayer)) ||
        (type & itInstance && !(m_selection_mode & smInstance))) {
        // Inform user why selection isn't completed
        // BBS: change "Object or Instance" to "Object"
        const wxString item_type = m_selection_mode & smInstance ? _(L("Object")) :
                                   m_selection_mode & smVolume   ? _(L("Part")) :
                                                                   _(L("Layer"));

        if (m_selection_mode == smInstance) {
            msg_str = wxString::Format(_(L("Selection conflicts")) + "\n\n" +
                                       _(L("If first selected item is an object, the second one should also be object.")) + "\n");
        } else {
            msg_str = wxString::Format(_(L("Selection conflicts")) + "\n\n" +
                                       _(L("If first selected item is a part, the second one should be part in the same object.")) + "\n");
        }

        // Unselect last selected item, if selection is without SHIFT
        if (!is_shift_pressed) {
            Unselect(m_last_selected_item);
            show_info(wxGetApp().plater(), msg_str, _(L("Info")));
        }

        return is_shift_pressed;
    }

    return true;
}

void ObjectList::fix_multiselection_conflicts()
{
    if (GetSelectedItemsCount() <= 1) {
        update_selection_mode();
        return;
    }

    wxString msg_string;
    if (!check_last_selection(msg_string))
        return;

    m_prevent_list_events = true;

    wxDataViewItemArray sels;
    GetSelections(sels);

    if (m_selection_mode & (smVolume | smLayer)) {
        // identify correct parent of the initial selected item
        const wxDataViewItem& parent = m_objects_model->GetParent(m_last_selected_item == sels.front() ? sels.back() : sels.front());

        sels.clear();
        wxDataViewItemArray children; // selected volumes from current parent
        m_objects_model->GetChildren(parent, children);

        const ItemType item_type = m_selection_mode & smVolume ? itVolume : itLayer;

        for (const auto& child : children)
            if (IsSelected(child) && m_objects_model->GetItemType(child) & item_type)
                sels.Add(child);

        // If some part is selected, unselect all items except of selected parts of the current object
        UnselectAll();
        SetSelections(sels);
    } else {
        for (const auto& item : sels) {
            if (!IsSelected(item)) // if this item is unselected now (from previous actions)
                continue;

            if (m_objects_model->GetItemType(item) & itSettings) {
                Unselect(item);
                continue;
            }

            const wxDataViewItem& parent = m_objects_model->GetParent(item);
            if (parent != wxDataViewItem(nullptr) && IsSelected(parent))
                Unselect(parent);
            else {
                wxDataViewItemArray unsels;
                m_objects_model->GetAllChildren(item, unsels);
                for (const auto& unsel_item : unsels)
                    Unselect(unsel_item);
            }

            if (m_objects_model->GetItemType(item) & itVolume)
                Unselect(item);

            m_selection_mode = smInstance;
        }
    }

    if (!msg_string.IsEmpty())
        show_info(wxGetApp().plater(), msg_string, _(L("Info")));

    if (!IsSelected(m_last_selected_item))
        m_last_selected_item = wxDataViewItem(nullptr);

    m_prevent_list_events = false;
}

void ObjectList::fix_cut_selection()
{
    wxDataViewItemArray sels;
    GetSelections(sels);
    if (fix_cut_selection(sels)) {
        m_prevent_list_events = true;

        // If some part is selected, unselect all items except of selected parts of the current object
        UnselectAll();
        SetSelections(sels);

        m_prevent_list_events = false;
    }
}

bool ObjectList::fix_cut_selection(wxDataViewItemArray& sels)
{
    if (wxGetApp().plater()->canvas3D()->get_gizmos_manager().get_current_type() == GLGizmosManager::Scale) {
        for (const auto& item : sels) {
            if (m_objects_model->GetItemType(item) & (itInstance | itObject) ||
                (m_objects_model->GetItemType(item) & itSettings &&
                 m_objects_model->GetItemType(m_objects_model->GetParent(item)) & itObject)) {
                bool is_instance_selection = m_objects_model->GetItemType(item) & itInstance;

                int object_idx = m_objects_model->GetObjectIdByItem(item);
                int inst_idx   = is_instance_selection ? m_objects_model->GetInstanceIdByItem(item) : 0;

                if (auto obj = object(object_idx); obj->is_cut()) {
                    sels.Clear();

                    auto cut_id = obj->cut_id;

                    int objects_cnt = int((*m_objects).size());
                    for (int obj_idx = 0; obj_idx < objects_cnt; ++obj_idx) {
                        auto object = (*m_objects)[obj_idx];
                        if (object->is_cut() && object->cut_id.has_same_id(cut_id))
                            sels.Add(is_instance_selection ? m_objects_model->GetItemByInstanceId(obj_idx, inst_idx) :
                                                             m_objects_model->GetItemById(obj_idx));
                    }
                    return true;
                }
            }
        }
    }
    return false;
}

ModelVolume* ObjectList::get_selected_model_volume()
{
    wxDataViewItem item = GetSelection();
    if (!item)
        return nullptr;
    if (m_objects_model->GetItemType(item) != itVolume) {
        if (m_objects_model->GetItemType(m_objects_model->GetParent(item)) == itVolume)
            item = m_objects_model->GetParent(item);
        else
            return nullptr;
    }

    const auto vol_idx = m_objects_model->GetVolumeIdByItem(item);
    const auto obj_idx = get_selected_obj_idx();
    if (vol_idx < 0 || obj_idx < 0)
        return nullptr;

    return (*m_objects)[obj_idx]->volumes[vol_idx];
}

bool ObjectList::can_change_part_type()
{
    ModelVolume* volume = get_selected_model_volume();
    if (!volume)
        return false;

    const int obj_idx = get_selected_obj_idx();
    if (obj_idx < 0)
        return false;

    const ModelVolumeType type = volume->type();
    if (type == ModelVolumeType::MODEL_PART) {
        int model_part_cnt = 0;
        for (auto vol : (*m_objects)[obj_idx]->volumes) {
            if (vol->type() == ModelVolumeType::MODEL_PART)
                ++model_part_cnt;
        }

        if (model_part_cnt == 1) {
            // Slic3r::GUI::show_error(nullptr, _(L("The type of the last solid object part is not to be changed.")));
            return false;
        }
    }

    return true;
}

void ObjectList::do_change_part_type(ModelVolumeType new_type)
{
    ModelVolume* volume = get_selected_model_volume();
    if (!volume)
        return;

    const int obj_idx = get_selected_obj_idx();
    if (obj_idx < 0)
        return;

    const ModelVolumeType type = volume->type();
    if (new_type == type || new_type == ModelVolumeType::INVALID)
        return;

    take_snapshot("Change part type");

    volume->set_type(new_type);
    wxDataViewItemArray sel = reorder_volumes_and_get_selection(obj_idx, [volume](const ModelVolume* vol) { return vol == volume; });
    if (!sel.IsEmpty())
        select_item(sel.front());
}

void ObjectList::change_part_type()
{
    ModelVolume* volume = get_selected_model_volume();
    if (!volume)
        return;

    const int obj_idx = get_selected_obj_idx();
    if (obj_idx < 0)
        return;

    const ModelVolumeType type = volume->type();
    if (type == ModelVolumeType::MODEL_PART) {
        int model_part_cnt = 0;
        for (auto vol : (*m_objects)[obj_idx]->volumes) {
            if (vol->type() == ModelVolumeType::MODEL_PART)
                ++model_part_cnt;
        }

        if (model_part_cnt == 1) {
            Slic3r::GUI::show_error(nullptr, _(L("The type of the last solid object part is not to be changed.")));
            return;
        }
    }

    // ORCA: Fix crash when changing type of svg / text modifier
    wxArrayString names;
    names.Add(_L("Part"));
    names.Add(_L("Negative Part"));
    names.Add(_L("Modifier"));
    if (!volume->is_svg() && !volume->is_text()) {
        names.Add(_L("Support Blocker"));
        names.Add(_L("Support Enforcer"));
    }

    SingleChoiceDialog dlg(_L("Type:"), _L("Choose part type"), names, int(type));
    auto               new_type = ModelVolumeType(dlg.GetSingleChoiceIndex());

    if (new_type == type || new_type == ModelVolumeType::INVALID)
        return;

    take_snapshot("Change part type");

    volume->set_type(new_type);
    wxDataViewItemArray sel = reorder_volumes_and_get_selection(obj_idx, [volume](const ModelVolume* vol) { return vol == volume; });
    if (!sel.IsEmpty())
        select_item(sel.front());
}

void ObjectList::last_volume_is_deleted(const int obj_idx)
{
    // BBS: object (obj_idx calc in obj list) is already removed from m_objects in Plater::priv::remove().
#if 0
    if (obj_idx < 0 || size_t(obj_idx) >= m_objects->size() || (*m_objects)[obj_idx]->volumes.size() != 1)
        return;

    auto volume = (*m_objects)[obj_idx]->volumes.front();

    // clear volume's config values
    volume->config.reset();

    // set a default extruder value, since user can't add it manually
    // BBS
    volume->config.set_key_value("extruder", new ConfigOptionInt(1));
#endif
}

void ObjectList::update_and_show_object_settings_item()
{
    // const wxDataViewItem item = GetSelection();
    // if (!item) return;

    // const wxDataViewItem& obj_item = m_objects_model->IsSettingsItem(item) ? m_objects_model->GetParent(item) : item;
    // select_item([this, obj_item](){ return add_settings_item(obj_item, &get_item_config(obj_item).get()); });
    part_selection_changed();
}

// Update settings item for item had it
void ObjectList::update_settings_item_and_selection(wxDataViewItem item, wxDataViewItemArray& selections)
{
    const wxDataViewItem old_settings_item = m_objects_model->GetSettingsItem(item);
    const wxDataViewItem new_settings_item = add_settings_item(item, &get_item_config(item).get());

    if (!new_settings_item && old_settings_item)
        m_objects_model->Delete(old_settings_item);

    // if ols settings item was is selected area
    if (selections.Index(old_settings_item) != wxNOT_FOUND) {
        // If settings item was just updated
        if (old_settings_item == new_settings_item) {
            Sidebar& panel = wxGetApp().sidebar();
            panel.Freeze();

            // update settings list
            wxGetApp().obj_settings()->UpdateAndShow(true);

            panel.Layout();
            panel.Thaw();
        } else
        // If settings item was deleted from the list,
        // it's need to be deleted from selection array, if it was there
        {
            selections.Remove(old_settings_item);

            // Select item, if settings_item doesn't exist for item anymore, but was selected
            if (selections.Index(item) == wxNOT_FOUND) {
                selections.Add(item);
                select_item(item); // to correct update of the SettingsList and ManipulationPanel sizers
            }
        }
    }
}

void ObjectList::update_object_list_by_printer_technology()
{
    m_prevent_canvas_selection_update = true;
    wxDataViewItemArray sel;
    GetSelections(sel); // stash selection

    wxDataViewItemArray object_items;
    m_objects_model->GetChildren(wxDataViewItem(nullptr), object_items);

    for (auto& object_item : object_items) {
        // update custom supports info
        update_info_items(m_objects_model->GetObjectIdByItem(object_item), &sel);

        // Update Settings Item for object
        update_settings_item_and_selection(object_item, sel);

        // Update settings for Volumes
        wxDataViewItemArray all_object_subitems;
        m_objects_model->GetChildren(object_item, all_object_subitems);
        for (auto item : all_object_subitems)
            if (m_objects_model->GetItemType(item) & itVolume)
                // update settings for volume
                update_settings_item_and_selection(item, sel);

        // Update Layers Items
        wxDataViewItem layers_item = m_objects_model->GetLayerRootItem(object_item);
        if (!layers_item)
            layers_item = add_layer_root_item(object_item);
        else if (printer_technology() == ptSLA) {
            // If layers root item will be deleted from the list, so
            // it's need to be deleted from selection array, if it was there
            wxDataViewItemArray del_items;
            bool                some_layers_was_selected = false;
            m_objects_model->GetAllChildren(layers_item, del_items);
            for (auto& del_item : del_items)
                if (sel.Index(del_item) != wxNOT_FOUND) {
                    some_layers_was_selected = true;
                    sel.Remove(del_item);
                }
            if (sel.Index(layers_item) != wxNOT_FOUND) {
                some_layers_was_selected = true;
                sel.Remove(layers_item);
            }

            // delete all "layers" items
            m_objects_model->Delete(layers_item);

            // Select object_item, if layers_item doesn't exist for item anymore, but was some of layer items was/were selected
            if (some_layers_was_selected)
                sel.Add(object_item);
        } else {
            wxDataViewItemArray all_obj_layers;
            m_objects_model->GetChildren(layers_item, all_obj_layers);

            for (auto item : all_obj_layers)
                // update settings for layer
                update_settings_item_and_selection(item, sel);
        }
    }

    // restore selection:
    SetSelections(sel);
    m_prevent_canvas_selection_update = false;
}

void ObjectList::instances_to_separated_object(const int obj_idx, const std::set<int>& inst_idxs)
{
    if ((*m_objects)[obj_idx]->instances.size() == inst_idxs.size()) {
        instances_to_separated_objects(obj_idx);
        return;
    }

    // create new object from selected instance
    ModelObject* model_object = (*m_objects)[obj_idx]->get_model()->add_object(*(*m_objects)[obj_idx]);
    for (int inst_idx = int(model_object->instances.size()) - 1; inst_idx >= 0; inst_idx--) {
        if (find(inst_idxs.begin(), inst_idxs.end(), inst_idx) != inst_idxs.end())
            continue;
        model_object->delete_instance(inst_idx);
    }

    // Add new object to the object_list
    const size_t new_obj_indx = static_cast<size_t>(m_objects->size() - 1);
    add_object_to_list(new_obj_indx);

    for (std::set<int>::const_reverse_iterator it = inst_idxs.rbegin(); it != inst_idxs.rend(); ++it) {
        // delete selected instance from the object
        del_subobject_from_object(obj_idx, *it, itInstance);
        delete_instance_from_list(obj_idx, *it);
    }

    // update printable state for new volumes on canvas3D
    wxGetApp().plater()->get_view3D_canvas3D()->update_instance_printable_state_for_object(new_obj_indx);
    update_info_items(new_obj_indx);
}

void ObjectList::instances_to_separated_objects(const int obj_idx)
{
    const int inst_cnt = (*m_objects)[obj_idx]->instances.size();

    std::vector<size_t> object_idxs;

    for (int i = inst_cnt - 1; i > 0; i--) {
        // create new object from initial
        ModelObject* object = (*m_objects)[obj_idx]->get_model()->add_object(*(*m_objects)[obj_idx]);
        for (int inst_idx = object->instances.size() - 1; inst_idx >= 0; inst_idx--) {
            if (inst_idx == i)
                continue;
            // delete unnecessary instances
            object->delete_instance(inst_idx);
        }

        // Add new object to the object_list
        const size_t new_obj_indx = static_cast<size_t>(m_objects->size() - 1);
        add_object_to_list(new_obj_indx);
        object_idxs.push_back(new_obj_indx);

        // delete current instance from the initial object
        del_subobject_from_object(obj_idx, i, itInstance);
        delete_instance_from_list(obj_idx, i);
    }

    // update printable state for new volumes on canvas3D
    wxGetApp().plater()->get_view3D_canvas3D()->update_instance_printable_state_for_objects(object_idxs);
    for (size_t object : object_idxs)
        update_info_items(object);
}

void ObjectList::split_instances()
{
    const Selection& selection = scene_selection();
    const int        obj_idx   = selection.get_object_idx();
    if (obj_idx == -1)
        return;

    Plater::TakeSnapshot snapshot(wxGetApp().plater(), "Instances to Separated Objects");

    if (selection.is_single_full_object()) {
        instances_to_separated_objects(obj_idx);
        return;
    }

    const int           inst_idx  = selection.get_instance_idx();
    const std::set<int> inst_idxs = inst_idx < 0 ? selection.get_instance_idxs() : std::set<int>{inst_idx};

    instances_to_separated_object(obj_idx, inst_idxs);
}

void ObjectList::rename_item()
{
    const wxDataViewItem item = GetSelection();
    if (!item || !(m_objects_model->GetItemType(item) & (itVolume | itObject)))
        return;

    const wxString new_name = wxGetTextFromUser(_(L("Enter new name")) + ":", _(L("Renaming")), m_objects_model->GetName(item), this);

    if (new_name.IsEmpty())
        return;

    if (Plater::has_illegal_filename_characters(new_name)) {
        Plater::show_illegal_characters_warning(this);
        return;
    }

    if (m_objects_model->SetName(new_name, item))
        update_name_in_model(item);
}

void GUI::ObjectList::rename_plate()
{
    const wxDataViewItem item = GetSelection();
    if (!item || !(m_objects_model->GetItemType(item) & (itPlate)))
        return;

    ObjectDataViewModelNode* node = (ObjectDataViewModelNode*) item.GetID();
    if (node->GetType() & itPlate) {
        int     plate_idx  = node->GetPlateIdx();
        Plater* the_plater = wxGetApp().plater();
        if (plate_idx >= 0 && the_plater) {
            the_plater->select_plate_by_hover_id(plate_idx * PartPlate::GRABBER_COUNT + PartPlate::e_at_edit, false, false);
            the_plater->get_current_canvas3D()->post_event(SimpleEvent(EVT_GLCANVAS_PLATE_NAME_CHANGE));
        }
    }
}

void ObjectList::fix_through_netfabb()
{
    // Do not fix anything when a gizmo is open. There might be issues with updates
    // and what is worse, the snapshot time would refer to the internal stack.
    if (!wxGetApp().plater()->get_view3D_canvas3D()->get_gizmos_manager().check_gizmos_closed_except(GLGizmosManager::Undefined))
        return;

    //          model_name
    std::vector<std::string> succes_models;
    //                   model_name     failing reason
    std::vector<std::pair<std::string, std::string>> failed_models;

    std::vector<int> obj_idxs, vol_idxs;
    get_selection_indexes(obj_idxs, vol_idxs);

    std::vector<std::string> model_names;

    // clear selections from the non-broken models if any exists
    // and than fill names of models to repairing
    if (vol_idxs.empty()) {
#if !FIX_THROUGH_NETFABB_ALWAYS
        for (int i = int(obj_idxs.size()) - 1; i >= 0; --i)
            if (object(obj_idxs[i])->get_repaired_errors_count() == 0)
                obj_idxs.erase(obj_idxs.begin() + i);
#endif // FIX_THROUGH_NETFABB_ALWAYS
        for (int obj_idx : obj_idxs)
            if (object(obj_idx))
                model_names.push_back(object(obj_idx)->name);
    } else {
        ModelObject* obj = object(obj_idxs.front());
        if (obj) {
#if !FIX_THROUGH_NETFABB_ALWAYS
            for (int i = int(vol_idxs.size()) - 1; i >= 0; --i)
                if (obj->get_repaired_errors_count(vol_idxs[i]) == 0)
                    vol_idxs.erase(vol_idxs.begin() + i);
#endif // FIX_THROUGH_NETFABB_ALWAYS
            for (int vol_idx : vol_idxs)
                model_names.push_back(obj->volumes[vol_idx]->name);
        }
    }

    auto plater = wxGetApp().plater();

    auto fix_and_update_progress = [this, plater, model_names](const int obj_idx, const int vol_idx, int model_idx,
                                                               ProgressDialog& progress_dlg, std::vector<std::string>& succes_models,
                                                               std::vector<std::pair<std::string, std::string>>& failed_models) {
        if (!object(obj_idx))
            return false;

        const std::string& model_name = model_names[model_idx];
        wxString           msg        = _L("Repairing model object");
        if (model_names.size() == 1)
            msg += ": " + from_u8(model_name) + "\n";
        else {
            msg += ":\n";
            for (int i = 0; i < int(model_names.size()); ++i)
                msg += (i == model_idx ? " > " : "   ") + from_u8(model_names[i]) + "\n";
            msg += "\n";
        }

        plater->clear_before_change_mesh(obj_idx);
        std::string res;
        if (!fix_model_by_win10_sdk_gui(*(object(obj_idx)), vol_idx, progress_dlg, msg, res))
            return false;
        // wxGetApp().plater()->changed_mesh(obj_idx);
        object(obj_idx)->ensure_on_bed();
        plater->changed_mesh(obj_idx);

        plater->get_partplate_list().notify_instance_update(obj_idx, 0);
        plater->sidebar().obj_list()->update_plate_values_for_items();

        if (res.empty())
            succes_models.push_back(model_name);
        else
            failed_models.push_back({model_name, res});

        update_item_error_icon(obj_idx, vol_idx);
        update_info_items(obj_idx);

        return true;
    };

    Plater::TakeSnapshot snapshot(plater, "Repairing model object");

    // Open a progress dialog.
    ProgressDialog progress_dlg(_L("Repairing model object"), "", 100, find_toplevel_parent(plater),
                                wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_CAN_ABORT, true);
    int            model_idx{0};
    if (vol_idxs.empty()) {
        int vol_idx{-1};
        for (int obj_idx : obj_idxs) {
#if !FIX_THROUGH_NETFABB_ALWAYS
            if (object(obj_idx)->get_repaired_errors_count(vol_idx) == 0)
                continue;
#endif // FIX_THROUGH_NETFABB_ALWAYS
            if (!fix_and_update_progress(obj_idx, vol_idx, model_idx, progress_dlg, succes_models, failed_models))
                break;
            model_idx++;
        }
    } else {
        int obj_idx{obj_idxs.front()};
        for (int vol_idx : vol_idxs) {
            if (!fix_and_update_progress(obj_idx, vol_idx, model_idx, progress_dlg, succes_models, failed_models))
                break;
            model_idx++;
        }
    }
    // Close the progress dialog
    progress_dlg.Update(100, "");

    // Show info notification
    wxString msg;
    wxString bullet_suf = "\n   - ";
    if (!succes_models.empty()) {
        msg = _L_PLURAL("Following model object has been repaired", "Following model objects have been repaired", succes_models.size()) +
              ":";
        for (auto& model : succes_models)
            msg += bullet_suf + from_u8(model);
        msg += "\n\n";
    }
    if (!failed_models.empty()) {
        msg += _L_PLURAL("Failed to repair folowing model object", "Failed to repair folowing model objects", failed_models.size()) + ":\n";
        for (auto& model : failed_models)
            msg += bullet_suf + from_u8(model.first) + ": " + _(model.second);
    }
    if (msg.IsEmpty())
        msg = _L("Repairing was canceled");
    plater->get_notification_manager()->push_notification(NotificationType::NetfabbFinished,
                                                          NotificationManager::NotificationLevel::PrintInfoShortNotificationLevel,
                                                          boost::nowide::narrow(msg));
}

void ObjectList::simplify()
{
    auto             plater     = wxGetApp().plater();
    GLGizmosManager& gizmos_mgr = plater->get_view3D_canvas3D()->get_gizmos_manager();

    // Do not simplify when a gizmo is open. There might be issues with updates
    // and what is worse, the snapshot time would refer to the internal stack.
    if (!gizmos_mgr.check_gizmos_closed_except(GLGizmosManager::EType::Simplify))
        return;

    if (gizmos_mgr.get_current_type() == GLGizmosManager::Simplify) {
        // close first
        gizmos_mgr.open_gizmo(GLGizmosManager::EType::Simplify);
    }
    gizmos_mgr.open_gizmo(GLGizmosManager::EType::Simplify);
}

void ObjectList::update_item_error_icon(const int obj_idx, const int vol_idx) const
{
    auto obj = object(obj_idx);
    if (wxDataViewItem obj_item = m_objects_model->GetItemById(obj_idx)) {
        const std::string& icon_name = get_warning_icon_name(obj->get_object_stl_stats());
        m_objects_model->UpdateWarningIcon(obj_item, icon_name);
    }

    if (vol_idx < 0)
        return;

    if (wxDataViewItem vol_item = m_objects_model->GetItemByVolumeId(obj_idx, vol_idx)) {
        const std::string& icon_name = get_warning_icon_name(obj->volumes[vol_idx]->mesh().stats());
        m_objects_model->UpdateWarningIcon(vol_item, icon_name);
    }
}

void ObjectList::msw_rescale()
{
    set_min_height();

    const int em = wxGetApp().em_unit();

    GetColumn(colName)->SetWidth(20 * em);
    GetColumn(colPrint)->SetWidth(3 * em);
    GetColumn(colFilament)->SetWidth(5 * em);
    // BBS
    GetColumn(colSupportPaint)->SetWidth(3 * em);
    GetColumn(colColorPaint)->SetWidth(3 * em);
    GetColumn(colSinking)->SetWidth(3 * em);
    GetColumn(colEditing)->SetWidth(3 * em);

    // rescale/update existing items with bitmaps
    m_objects_model->Rescale();

    Layout();
}

void ObjectList::sys_color_changed()
{
    wxGetApp().UpdateDVCDarkUI(this, true);

    msw_rescale();

    if (m_objects_model) {
        m_objects_model->sys_color_changed();
    }

    m_texture.init_svg_texture();
}

void ObjectList::ItemValueChanged(wxDataViewEvent& event)
{
    if (event.GetColumn() == colName)
        update_name_in_model(event.GetItem());
    else if (event.GetColumn() == colFilament) {
        wxDataViewItem item = event.GetItem();
        if (m_objects_model->GetItemType(item) == itObject)
            m_objects_model->UpdateVolumesExtruderBitmap(item, true);
        update_filament_in_config(item);
    }
}

void GUI::ObjectList::OnStartEditing(wxDataViewEvent& event)
{
    auto col  = event.GetColumn();
    auto item = event.GetItem();
    if (col == colName) {
        ObjectDataViewModelNode* node = (ObjectDataViewModelNode*) item.GetID();
        if (node->GetType() & itPlate) {
            int plate_idx = node->GetPlateIdx();
            if (plate_idx >= 0) {
                auto plate = wxGetApp().plater()->get_partplate_list().get_plate(plate_idx);
                m_objects_model->SetName(from_u8(plate->get_plate_name()), GetSelection());
            }
        }
    }
}

// Workaround for entering the column editing mode on Windows. Simulate keyboard enter when another column of the active line is selected.
// Here the last active column is forgotten, so when leaving the editing mode, the next mouse click will not enter the editing mode of the
// newly selected column.
void ObjectList::OnEditingStarted(wxDataViewEvent& event)
{
#ifdef __WXMSW__
    m_last_selected_column = -1;
#else
    event.Veto(); // Not edit with NSTableView's text
    auto col  = event.GetColumn();
    auto item = event.GetItem();
    if (col == colPrint) {
        toggle_printable_state();
        return;
    } else if (col == colSupportPaint) {
        ObjectDataViewModelNode* node = (ObjectDataViewModelNode*) item.GetID();
        if (node->HasSupportPainting()) {
            GLGizmosManager& gizmos_mgr = wxGetApp().plater()->get_view3D_canvas3D()->get_gizmos_manager();
            if (gizmos_mgr.get_current_type() != GLGizmosManager::EType::FdmSupports)
                gizmos_mgr.open_gizmo(GLGizmosManager::EType::FdmSupports);
            else
                gizmos_mgr.reset_all_states();
        }
        return;
    } else if (col == colColorPaint) {
        ObjectDataViewModelNode* node = (ObjectDataViewModelNode*) item.GetID();
        if (node->HasColorPainting()) {
            GLGizmosManager& gizmos_mgr = wxGetApp().plater()->get_view3D_canvas3D()->get_gizmos_manager();
            if (gizmos_mgr.get_current_type() != GLGizmosManager::EType::MmuSegmentation)
                gizmos_mgr.open_gizmo(GLGizmosManager::EType::MmuSegmentation);
            else
                gizmos_mgr.reset_all_states();
        }
        return;
    } else if (col == colSinking) {
        Plater*     plater = wxGetApp().plater();
        GLCanvas3D* cnv    = plater->canvas3D();
        Plater::TakeSnapshot(plater, "Shift objects to bed");
        int obj_idx, vol_idx;
        get_selected_item_indexes(obj_idx, vol_idx, item);
        (*m_objects)[obj_idx]->ensure_on_bed();
        cnv->reload_scene(true, true);
        update_info_items(obj_idx);
        notify_instance_updated(obj_idx);
    } else if (col == colEditing) {
        // show_context_menu(evt_context_menu);
        int obj_idx, vol_idx;

        get_selected_item_indexes(obj_idx, vol_idx, item);
        // wxGetApp().plater()->PopupObjectTable(obj_idx, vol_idx, mouse_pos);
        dynamic_cast<TabPrintModel*>(wxGetApp().get_model_tab(vol_idx >= 0))->reset_model_config();
        return;
    }
    if (col != colFilament && col != colName)
        return;
    auto       column   = GetColumn(col);
    const auto renderer = column->GetRenderer();
    if (!renderer->GetEditorCtrl()) {
        renderer->StartEditing(item, GetItemRect(item, column));
        if (col == colName) // TODO: for colName editing, disable shortcuts
            SetAcceleratorTable(wxNullAcceleratorTable);
    }
#ifdef __WXOSX__
    SetCustomRendererPtr(dynamic_cast<wxDataViewCustomRenderer*>(renderer));
#endif
#endif //__WXMSW__
}

void ObjectList::OnEditingDone(wxDataViewEvent& event)
{
    if (event.GetColumn() != colName)
        return;

    if (event.IsEditCancelled()) {
        if (m_objects_model->GetItemType(event.GetItem()) & itPlate) {
            int plate_idx = -1;
            m_objects_model->GetItemType(event.GetItem(), plate_idx);
            if (plate_idx >= 0) {
                auto plate = wxGetApp().plater()->get_partplate_list().get_plate(plate_idx);
                m_objects_model->SetCurSelectedPlateFullName(plate_idx, plate->get_plate_name());
            }
        }
    }

    const auto renderer = dynamic_cast<BitmapTextRenderer*>(GetColumn(colName)->GetRenderer());
#if __WXOSX__
    SetAcceleratorTable(m_accel);
#endif

    if (renderer->WasCanceled())
        wxTheApp->CallAfter([this] { Plater::show_illegal_characters_warning(this); });

#ifdef __WXMSW__
    // Workaround for entering the column editing mode on Windows. Simulate keyboard enter when another column of the active line is
    // selected. Here the last active column is forgotten, so when leaving the editing mode, the next mouse click will not enter the editing
    // mode of the newly selected column.
    m_last_selected_column = -1;
#endif //__WXMSW__

    Plater* plater = wxGetApp().plater();
    if (plater)
        plater->set_current_canvas_as_dirty();
}

// BBS: remove "const" qualifier
void ObjectList::set_extruder_for_selected_items(const int extruder)
{
    // BBS: check extruder id
    std::vector<std::string> colors = wxGetApp().plater()->get_extruder_colors_from_plater_config();
    if (extruder > colors.size())
        return;

    wxDataViewItemArray sels;
    GetSelections(sels);

    if (sels.empty())
        return;

    take_snapshot("Change Filaments");

    for (const wxDataViewItem& sel_item : sels) {
        /* We can change extruder for Object/Volume only.
         * So, if Instance is selected, get its Object item and change it
         */
        ItemType       sel_item_type = m_objects_model->GetItemType(sel_item);
        wxDataViewItem item          = (sel_item_type & itInstance) ? m_objects_model->GetObject(item) : sel_item;
        ItemType       type          = m_objects_model->GetItemType(item);
        if (type & itVolume) {
            const int obj_idx = m_objects_model->GetObjectIdByItem(item);
            const int vol_idx = m_objects_model->GetVolumeIdByItem(item);

            if ((obj_idx < m_objects->size()) && (obj_idx < (*m_objects)[obj_idx]->volumes.size())) {
                auto volume_type = (*m_objects)[obj_idx]->volumes[vol_idx]->type();
                if (volume_type != ModelVolumeType::MODEL_PART && volume_type != ModelVolumeType::PARAMETER_MODIFIER)
                    continue;
            }
        }

        if (type & itLayerRoot)
            continue;

        // BBS: handle extruder 0 for part, use it's parent extruder
        int new_extruder = extruder;
        if (extruder == 0) {
            if (type & itObject) {
                new_extruder = 1;
            } else if ((type & itVolume) && (m_objects_model->GetVolumeType(sel_item) == ModelVolumeType::MODEL_PART)) {
                new_extruder = m_objects_model->GetExtruderNumber(m_objects_model->GetParent(sel_item));
            }
        }

        ModelConfig& config = get_item_config(item);
        if (config.has("extruder"))
            config.set("extruder", new_extruder);
        else
            config.set_key_value("extruder", new ConfigOptionInt(new_extruder));

        // for object, clear all its part volume's extruder config
        if (type & itObject) {
            ObjectDataViewModelNode* node = (ObjectDataViewModelNode*) item.GetID();
            for (ModelVolume* mv : node->m_model_object->volumes) {
                if (mv->type() == ModelVolumeType::MODEL_PART && mv->config.has("extruder"))
                    mv->config.erase("extruder");
            }
        }

        const wxString extruder_str = wxString::Format("%d", new_extruder);
        m_objects_model->SetExtruder(extruder_str, item);
    }

    // update scene
    wxGetApp().plater()->update();

    // BBS: update extruder/filament column
    Refresh();
}

void ObjectList::on_plate_added(PartPlate* part_plate) { wxDataViewItem plate_item = m_objects_model->AddPlate(part_plate); }

void ObjectList::on_plate_deleted(int plate_idx)
{
    m_objects_model->DeletePlate(plate_idx);

    wxDataViewItemArray top_list;
    m_objects_model->GetChildren(wxDataViewItem(nullptr), top_list);
    for (wxDataViewItem item : top_list) {
        Expand(item);
    }
}

void ObjectList::reload_all_plates(bool notify_partplate)
{
    m_prevent_canvas_selection_update = true;

    // Unselect all objects before deleting them, so that no change of selection is emitted during deletion.

    /* To avoid execution of selection_changed()
     * from wxEVT_DATAVIEW_SELECTION_CHANGED emitted from DeleteAll(),
     * wrap this two functions into m_prevent_list_events *
     * */
    m_prevent_list_events = true;
    this->UnselectAll();
    m_objects_model->ResetAll();
    m_prevent_list_events = false;

    PartPlateList& ppl = wxGetApp().plater()->get_partplate_list();
    for (int i = 0; i < ppl.get_plate_count(); i++) {
        PartPlate* pp = ppl.get_plate(i);
        m_objects_model->AddPlate(pp, wxEmptyString);
    }

    size_t              obj_idx = 0;
    std::vector<size_t> obj_idxs;
    obj_idxs.reserve(m_objects->size());
    while (obj_idx < m_objects->size()) {
        add_object_to_list(obj_idx, false, notify_partplate);
        obj_idxs.push_back(obj_idx);
        ++obj_idx;
    }

    update_selections();

    m_prevent_canvas_selection_update = false;

    // update scene
    wxGetApp().plater()->update();
    // update printable states on canvas
    wxGetApp().plater()->get_view3D_canvas3D()->update_instance_printable_state_for_objects(obj_idxs);
}

void ObjectList::on_plate_selected(int plate_index)
{
    wxDataViewItem item = m_objects_model->GetItemByPlateId(plate_index);
    wxDataViewItem sel  = GetSelection();

    if (sel == item)
        return;

    UnselectAll();
    Select(item);
}

// BBS: notify partplate the instance added/updated
void ObjectList::notify_instance_updated(int obj_idx)
{
    const int      inst_cnt = (*m_objects)[obj_idx]->instances.size();
    PartPlateList& list     = wxGetApp().plater()->get_partplate_list();
    for (int index = 0; index < inst_cnt; index++)
        list.notify_instance_update(obj_idx, index);
}

void ObjectList::update_after_undo_redo()
{
    Plater::SuppressSnapshots suppress(wxGetApp().plater());
    // BBS: undo/redo will rebuild all the plates before
    // no need to notify instance to partplate
    reload_all_plates(false);
}

wxDataViewItemArray ObjectList::reorder_volumes_and_get_selection(int                                     obj_idx,
                                                                  std::function<bool(const ModelVolume*)> add_to_selection /* = nullptr*/)
{
    wxDataViewItemArray items;
    if (obj_idx < 0 || obj_idx >= (*m_objects).size())
        return items;

    ModelObject* object = (*m_objects)[obj_idx];
    if (object->volumes.size() <= 1)
        return items;

    object->sort_volumes(true);
    update_info_items(obj_idx, nullptr, true);
    items = add_volumes_to_object_in_list(obj_idx, std::move(add_to_selection));

    changed_object(obj_idx);
    return items;
}

void ObjectList::apply_volumes_order()
{
    if (!m_objects)
        return;

    for (size_t obj_idx = 0; obj_idx < m_objects->size(); obj_idx++)
        reorder_volumes_and_get_selection(obj_idx);
}

void ObjectList::update_printable_state(int obj_idx, int instance_idx)
{
    ModelObject* object = (*m_objects)[obj_idx];

    const PrintIndicator printable = object->instances[instance_idx]->printable ? piPrintable : piUnprintable;
    if (object->instances.size() == 1)
        instance_idx = -1;

    m_objects_model->SetPrintableState(printable, obj_idx, instance_idx);
}

void ObjectList::toggle_printable_state()
{
    wxDataViewItemArray sels;
    GetSelections(sels);
    if (sels.IsEmpty())
        return;

    wxDataViewItem frst_item = sels[0];

    ItemType type = m_objects_model->GetItemType(frst_item);
    if (!(type & (itObject | itInstance)))
        return;

    int  obj_idx   = m_objects_model->GetObjectIdByItem(frst_item);
    int  inst_idx  = type == itObject ? 0 : m_objects_model->GetInstanceIdByItem(frst_item);
    bool printable = !object(obj_idx)->instances[inst_idx]->printable;

    // BBS: remove snapshot name "Set Printable group", "Set Unprintable group", "Set Printable"...
    take_snapshot("");

    std::vector<size_t> obj_idxs;
    for (auto item : sels) {
        type = m_objects_model->GetItemType(item);
        if (!(type & (itObject | itInstance)))
            continue;

        obj_idx          = m_objects_model->GetObjectIdByItem(item);
        ModelObject* obj = object(obj_idx);

        obj_idxs.emplace_back(static_cast<size_t>(obj_idx));

        // set printable value for selected instance/instances in object
        if (type == itInstance) {
            inst_idx                                                              = m_objects_model->GetInstanceIdByItem(item);
            obj->instances[m_objects_model->GetInstanceIdByItem(item)]->printable = printable;
        } else
            for (auto inst : obj->instances)
                inst->printable = printable;

        // update printable state in ObjectList
        m_objects_model->SetObjectPrintableState(printable ? piPrintable : piUnprintable, item);
    }

    sort(obj_idxs.begin(), obj_idxs.end());
    obj_idxs.erase(unique(obj_idxs.begin(), obj_idxs.end()), obj_idxs.end());

    // update printable state on canvas
    wxGetApp().plater()->get_view3D_canvas3D()->update_instance_printable_state_for_objects(obj_idxs);

    // update scene
    wxGetApp().plater()->update();
    wxGetApp().plater()->reload_paint_after_background_process_apply();
}

ModelObject* ObjectList::object(const int obj_idx) const
{
    if (obj_idx < 0)
        return nullptr;

    return (*m_objects)[obj_idx];
}

bool ObjectList::has_paint_on_segmentation() { return m_objects_model->HasInfoItem(InfoItemType::MmuSegmentation); }

void ObjectList::apply_object_instance_transfrom_to_all_volumes(ModelObject* model_object, bool need_update_assemble_matrix)
{
    const Geometry::Transformation& instance_transformation  = model_object->instances[0]->get_transformation();
    Vec3d                           original_instance_center = instance_transformation.get_offset();

    if (need_update_assemble_matrix) {
        // apply the instance_transform(except offset) to assemble_transform
        Geometry::Transformation instance_transformation_copy = instance_transformation;
        instance_transformation_copy.set_offset(Vec3d(0, 0, 0)); // remove the effect of offset
        const Transform3d& instance_inverse_matrix = instance_transformation_copy.get_matrix().inverse();
        const Transform3d& assemble_matrix         = model_object->instances[0]->get_assemble_transformation().get_matrix();
        Transform3d        new_assemble_transform  = assemble_matrix * instance_inverse_matrix;
        model_object->instances[0]->set_assemble_from_transform(new_assemble_transform);
    }

    // apply the instance_transform to volumn
    const Transform3d& transformation_matrix = instance_transformation.get_matrix();
    for (ModelVolume* volume : model_object->volumes) {
        const Transform3d& volume_matrix = volume->get_matrix();
        Transform3d        new_matrix    = transformation_matrix * volume_matrix;
        volume->set_transformation(new_matrix);
    }
    model_object->instances[0]->set_transformation(Geometry::Transformation());

    model_object->ensure_on_bed();
    // keep new instance center the same as the original center
    model_object->translate(-original_instance_center);
    model_object->translate_instances(original_instance_center);

    // update the cache data in selection to keep the data of ModelVolume and GLVolume are consistent
    wxGetApp().plater()->update();
}

void ObjectList::render_plate_tree_by_ImGui()
{
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.0f, 0.0f, 0.0, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 0.0f, 0.0, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImGuiWrapper::COL_CREALITY); // use for TreeNode be selected

    wxDataViewItemArray all_plates;
    m_objects_model->GetChildren(wxDataViewItem(nullptr), all_plates);

    //****************************************************************************//

    float                   pos_y    = ImGui::GetCursorPosY();
    float                   canvas_h = wxGetApp().plater()->get_current_canvas3D()->get_canvas_size().get_height();
    GLCanvas3D::ECanvasType type     = wxGetApp().plater()->get_current_canvas3D()->get_canvas_type();
    float                   table_h  = 0.0f;
    if (type == GLCanvas3D::CanvasView3D) {
        table_h = canvas_h * 0.65f - pos_y;
    } else if (type == GLCanvas3D::CanvasPreview) {
        table_h = canvas_h * 0.4f - pos_y;
    }
    float view_scale = wxGetApp().plater()->get_current_canvas3D()->get_scale();

    // m_obj_list_window_focus = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    static ImGuiTableFlags flags = ImGuiTableFlags_None | ImGuiTableFlags_RowBg | ImGuiTableFlags_Hideable | ImGuiTableFlags_ScrollY;

    auto get_a_node_valid_column = [](ObjectDataViewModelNode* node) {
        if (node == NULL)
            return 0;

        int init_column = 0;

        if (node->HasSupportPainting()) {
            init_column++;
        }
        if (node->HasColorPainting()) {
            init_column++;
        }
        if (node->HasSinking()) {
            init_column++;
        }
        if (node->IsActionEnabled()) {
            init_column++;
        }
        return init_column;
    };

    std::function<int(ObjectDataViewModelNode * node)> get_node_and_children_max_valid_column;
    get_node_and_children_max_valid_column = [&](ObjectDataViewModelNode* node) {
        if (node == NULL)
            return 0;

        int sub_column = get_a_node_valid_column(node);

        if (node->get_open() && node->GetChildCount() > 0) {
            MyObjectTreeModelNodePtrArray& sub_nodes = node->GetChildren();
            for (int child_n = 0; child_n < sub_nodes.size(); child_n++) {
                int c      = get_node_and_children_max_valid_column(sub_nodes[child_n]);
                sub_column = std::max(c, sub_column);
            }
        }
        return sub_column;
    };

    int num_var_column = 0;

    for (size_t i = 0; i < all_plates.size(); i++) {
        wxDataViewItem&          p    = all_plates[i];
        ObjectDataViewModelNode* node = static_cast<ObjectDataViewModelNode*>(p.GetID());

        num_var_column = std::max(get_node_and_children_max_valid_column(node), num_var_column);
    }

    // HelpMarker("See \"Columns flags\" section to configure how indentation is applied to individual columns.");
    if (ImGui::BeginTable("##obj_table", num_var_column + 2, flags, ImVec2(ImGui::GetCurrentWindow()->Size.x - 15.0f * view_scale, table_h))) {
        // The first column will use the default _WidthStretch when ScrollX is Off and _WidthFixed when ScrollX is On
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch);

        ImGuiTableColumnFlags flags     = ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthFixed;
        const float           icon_size = 20.0f * view_scale;

        for (size_t i = 0; i < num_var_column; i++) {
            ImGui::TableSetupColumn(("Column_" + std::to_string(i)).c_str(), flags, icon_size);
        }

        // ImGui::TableSetupColumn("Support", flags, icon_size);
        ////ImGui::TableSetupColumn("Seam", flags, icon_size);
        // ImGui::TableSetupColumn("Mmu", flags, icon_size);
        // ImGui::TableSetupColumn("Sinking", flags, icon_size);
        // ImGui::TableSetupColumn("UndoSettings", flags, icon_size);

        ImGui::TableSetupColumn("Extruder", flags, 44.0f * view_scale);

        for (size_t i = 0; i < all_plates.size(); i++) {
            wxDataViewItem&          p    = all_plates[i];
            ObjectDataViewModelNode* node = static_cast<ObjectDataViewModelNode*>(p.GetID());
            render_plate(node);
        }

        ImGui::EndTable();
    }

    m_table_data.table_height   = ImGui::GetItemRectSize().y;
    m_table_data.table_offset_y = ImGui::GetItemRectMin().y;
    // ImGui::Text("Table Height: %.2f", table_size.y);
    ImGui::PopStyleColor(3);
}

void ObjectList::render_plate(ObjectDataViewModelNode* plate)
{
    if (plate == nullptr)
        return;
    wxDataViewItemArray sels;
    GetSelections(sels);

    bool plate_selected = false;
    {
        for (wxDataViewItem& item : sels) {
            ObjectDataViewModelNode* node = static_cast<ObjectDataViewModelNode*>(item.GetID());
            ItemType                 type = node->GetType();

            if (type & ItemType::itVolume) {
            } else if (type & ItemType::itObject) {
            } else if (type & ItemType::itPlate) {
                if (node == plate) {
                    plate_selected = true;
                }
            }
        }
    }

    MyObjectTreeModelNodePtrArray& objects = plate->GetChildren();

    ImGui::TableNextRow();

    if (plate_selected) {
        bool focused = get_object_list_window_focus();
        if (focused) {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiWrapper::COL_CREALITY));
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(ImGuiWrapper::COL_CREALITY));
        } else {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4{0.090f, 0.80f, 0.373, 0.15f}));
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(ImVec4{0.090f, 0.80f, 0.373, 0.15f}));
        }
    } else {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32_BLACK_TRANS);
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, IM_COL32_BLACK_TRANS);
    }

    ImGui::TableNextColumn();
    ImGuiTreeNodeFlags tree_node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (!objects.IsEmpty()) {
        tree_node_flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }
    /*if (plate_selected) {
        tree_node_flags |= ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_Selected;
    }*/

    ImGui::AlignTextToFramePadding();
    Plater*     the_plater      = wxGetApp().plater();
    std::string plate_full_name = plate->GetName().ToUTF8().data();

    int plate_idx = plate->GetPlateIdx();
    if (plate_idx >= 0) {
        auto the_plate = the_plater->get_partplate_list().get_plate(plate_idx);
        if (the_plate) {
            plate_full_name = _u8L("Plate");
            plate_full_name += " " + std::to_string(plate_idx + 1);
            std::string custom_name = the_plate->get_plate_name();
            if (!custom_name.empty()) {
                plate_full_name += " (" + custom_name + ")";
            }
        }
    }

    bool   is_dark    = wxGetApp().dark_mode();
    ImVec4 text_color = is_dark ? ImVec4(1.0f, 1.0f, 1.0, 1.0f) :
                                  (plate_selected ? ImVec4(1.0f, 1.0f, 1.0, 1.0f) : ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, text_color);

    bool open = ImGui::TreeNodeEx(plate_full_name.c_str(), tree_node_flags);

    ImGui::PopStyleColor(1);

    bool left_clicked  = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    bool right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);

    if (plate_idx >= 0 && (left_clicked || right_clicked) && !ImGui::IsItemToggledOpen()) {
        if (the_plater->is_preview_shown()) {
            the_plater->select_sliced_plate(plate_idx);
        } else {
            the_plater->select_plate(plate_idx);
        }

        the_plater->deselect_all();

        // rename plate
        if (plate_selected && left_clicked && sels.size() == 1) {
            rename_plate();
        }

        if (right_clicked) {
            // show_context_menu(true);
            auto event = IntEvent(EVT_OBJ_LIST_COLUMN_SELECT, 0);
            event.SetEventObject(this);
            wxPostEvent(this, event);
        }
    }

    float  view_scale = wxGetApp().plater()->get_current_canvas3D()->get_scale();
    ImVec2 icon_size  = ImVec2(20 * view_scale, 20 * view_scale);

#if 0
    // Support
    ImGui::TableNextColumn();

    // Seam
    // ImGui::TableNextColumn();

    // Mmu
    ImGui::TableNextColumn();

    // Sinking
    ImGui::TableNextColumn();

    // extruder
    ImGui::TableNextColumn();
#endif //  0

    for (size_t i = 0; i < ImGui::TableGetColumnCount() - 2; i++) {
        ImGui::TableNextColumn();
    }

    // undo settings
    ImGui::TableNextColumn();
    if (plate->IsActionEnabled()) {
        ImTextureID normal_id = m_texture.get_texture_id();

        ImGui::PushID((plate_full_name + "undo_settings" + std::to_string(ObjList_Texture::IM_TEXTURE_NAME::texEditing)).c_str());

        if (ImGui::ImageButton(normal_id, icon_size, m_texture.get_texture_uv0(ObjList_Texture::IM_TEXTURE_NAME::texEditing, false),
                               m_texture.get_texture_uv1(ObjList_Texture::IM_TEXTURE_NAME::texEditing, false), 0)) {
            if (wxGetApp().plater()->get_current_canvas3D()->get_canvas_type() != GLCanvas3D::CanvasAssembleView) {
                if (plate && plate->IsActionEnabled()) {
                    // select_node_func(plate);

                    int            obj_idx, vol_idx;
                    wxDataViewItem item = wxDataViewItem(plate);
                    get_selected_item_indexes(obj_idx, vol_idx, item);
                    // wxGetApp().plater()->PopupObjectTable(obj_idx, vol_idx, mouse_pos);
                    if (m_objects_model->GetItemType(item) & itPlate)
                        dynamic_cast<TabPrintPlate*>(wxGetApp().get_plate_tab())->reset_model_config();
                    else if (m_objects_model->GetItemType(item) & itLayer)
                        dynamic_cast<TabPrintLayer*>(wxGetApp().get_layer_tab())->reset_model_config();
                    else
                        dynamic_cast<TabPrintModel*>(wxGetApp().get_model_tab(vol_idx >= 0))->reset_model_config();
                }
            }
        }

        if (ImGui::IsItemHovered()) {
            ImVec4 text_color = is_dark ? ImVec4(1.0, 1.0, 1.0, 1.0) : ImVec4(0.2, 0.2, 0.2, 1.0);
            ImGui::PushStyleColor(ImGuiCol_Text, text_color);
            ImGui::SetTooltip(_u8L("Click the icon to reset all settings of the object").c_str());
            ImGui::PopStyleColor();
        }

        ImGui::PopID();
    }

    if (plate_selected) {
        ensure_current_item_visible_imgui();
    }

    if (open) {
        for (int child_n = 0; child_n < objects.size(); child_n++)
            render_object(objects[child_n]);
        ImGui::TreePop();
    }

    plate->set_open(open);
}

void ObjectList::render_object(ObjectDataViewModelNode* object)
{
    if (object == nullptr)
        return;

    MyObjectTreeModelNodePtrArray& volumes = object->GetChildren();
    size_t                         count   = volumes.size();
    if (count == 0) {
        render_volume(object);
        return;
    }

    ImGui::TableNextRow();
    render_generic_columns(object);
}

void ObjectList::render_volume(ObjectDataViewModelNode* volume)
{
    ImGui::TableNextRow();
    render_generic_columns(volume);
}

void ObjectList::render_generic_columns(ObjectDataViewModelNode* node)
{
    if (node == nullptr)
        return;

    if (!m_texture.valid()) {
        m_texture.init_svg_texture();
    }
    if (!m_texture.valid())
        return;

    float  view_scale = wxGetApp().plater()->get_current_canvas3D()->get_scale();
    ImVec2 icon_size  = ImVec2(20 * view_scale, 20 * view_scale);

    ImGuiContext& g = *GImGui;
    // ImGuiWindow* window = g.CurrentWindow;
    const ImGuiStyle& style = g.Style;

    ImTextureID normal_id = m_texture.get_texture_id();

    char unique_label[128] = {"##"};
    sprintf(&unique_label[2], "%p_", node);
    const std::string node_label(unique_label);

    bool is_view3D = wxGetApp().plater()->get_current_canvas3D()->get_canvas_type() == GLCanvas3D::CanvasView3D;

    wxDataViewItemArray sels;
    GetSelections(sels);

    bool node_selected = false;
    {
        for (wxDataViewItem& item : sels) {
            ObjectDataViewModelNode* sel  = static_cast<ObjectDataViewModelNode*>(item.GetID());
            ItemType                 type = sel->GetType();

            if (sel == node) {
                node_selected = true;
            }
            /*if (type & ItemType::itVolume) {
                if (sel == node) {
                    node_selected = true;
                }
            } else if (type & ItemType::itObject) {
                if (sel == node) {
                    node_selected = true;
                }
            } else if (type & ItemType::itPlate) {
            }*/
        }
    }

    bool   is_dark    = wxGetApp().dark_mode();
    ImVec4 text_color = is_dark ? ImVec4(1.0f, 1.0f, 1.0, 1.0f) :
                                  (node_selected ? ImVec4(1.0f, 1.0f, 1.0, 1.0f) : ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, text_color);

    if (node_selected) {
        bool focused = get_object_list_window_focus();
        if (focused) {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiWrapper::COL_CREALITY));
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(ImGuiWrapper::COL_CREALITY));
        } else {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4{0.090f, 0.80f, 0.373, 0.15f}));
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(ImVec4{0.090f, 0.80f, 0.373, 0.15f}));
        }

    } else {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32_BLACK_TRANS);
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, IM_COL32_BLACK_TRANS);
    }

    MyObjectTreeModelNodePtrArray& children = node->GetChildren();
    size_t                         count    = children.size();

    ImGui::TableNextColumn();

    ImGui::AlignTextToFramePadding();

    ImGuiTreeNodeFlags tree_node_flags = ImGuiTreeNodeFlags_None;
    if (count == 0) {
        // tree_node_flags |= ImGuiTreeNodeFlags_Leaf;
    } else {
        tree_node_flags |= ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow;
    }

    if (node_selected) {
        // tree_node_flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool open = false;
    if (tree_node_flags != ImGuiTreeNodeFlags_None) {
        open = ImGui::TreeNodeEx((node_label + "tree_node").c_str(), tree_node_flags);
        ImGui::SameLine();
    }

    /// render printable icon
    PrintIndicator                   p              = node->IsPrintable();
    ObjList_Texture::IM_TEXTURE_NAME printable_name = ObjList_Texture::IM_TEXTURE_NAME::texCount;
    switch (p) {
    case Slic3r::GUI::piUndef: break;
    case Slic3r::GUI::piPrintable: {
        printable_name = ObjList_Texture::IM_TEXTURE_NAME::texPrintable;
    } break;
    case Slic3r::GUI::piUnprintable: {
        printable_name = ObjList_Texture::IM_TEXTURE_NAME::texUnprintable;
    } break;
    default: break;
    }

    if (printable_name != ObjList_Texture::IM_TEXTURE_NAME::texCount) {
        ImGui::PushID((node_label + "printable").c_str());
        bool result = ImGui::ImageButton(normal_id, icon_size, m_texture.get_texture_uv0(printable_name, false),
                                         m_texture.get_texture_uv1(printable_name, false), 0);
        if (result) {
            select_node(node, node_selected);
            toggle_printable_state();
        }
        ImGui::PopID();
        if (ImGui::IsItemHovered()) {
            ImVec4 text_color = is_dark ? ImVec4(1.0, 1.0, 1.0, 1.0) : ImVec4(0.2, 0.2, 0.2, 1.0);
            ImGui::PushStyleColor(ImGuiCol_Text, text_color);
            ImGui::SetTooltip(_u8L("Click the icon to toggle printable property of the object").c_str());
            ImGui::PopStyleColor();
        }
        ImGui::SameLine(0, 0);
    }

    // render repair icon
    if (node->has_warning_icon()) {
        ImGui::PushID((node_label + "repair").c_str());

        const ObjList_Texture::IM_TEXTURE_NAME repair_name = ObjList_Texture::IM_TEXTURE_NAME::texMeshRepair;
        bool result = ImGui::ImageButton(normal_id, icon_size, m_texture.get_texture_uv0(repair_name, true),
                                         m_texture.get_texture_uv1(repair_name, true), 0);
        if (result) {
            select_node(node, node_selected);

            if (is_windows10())
                fix_through_netfabb();
        }
        ImGui::PopID();

        wxDataViewItem item    = wxDataViewItem(node);
        wxString       tooltip = "";
        if (const ItemType type = m_objects_model->GetItemType(item); type & (itObject | itVolume)) {
            int obj_idx = m_objects_model->GetObjectIdByItem(item);
            int vol_idx = type & itVolume ? m_objects_model->GetVolumeIdByItem(item) : -1;
            tooltip     = get_mesh_errors_info(obj_idx, vol_idx).tooltip;
        }

        if (tooltip.length() > 0 && ImGui::IsItemHovered()) {
            ImVec4 text_color = is_dark ? ImVec4(1.0, 1.0, 1.0, 1.0) : ImVec4(0.2, 0.2, 0.2, 1.0);
            ImGui::PushStyleColor(ImGuiCol_Text, text_color);
            ImGui::SetTooltip("%s", tooltip.ToUTF8().data());
            ImGui::PopStyleColor();
        }
        ImGui::SameLine(0, 0);
    }

    // render part type icon + name
    ModelVolumeType volume_type = ModelVolumeType::INVALID;
    ItemType        type        = node->GetType();
    if (type & ItemType::itVolume) {
        volume_type = node->GetVolumeType();
    }

    const char* node_name                    = node->GetName().ToUTF8().data();
    auto        make_drag_drop_payload_label = [type, volume_type](ObjectDataViewModelNode* n) {
        std::string label;
        if (n == nullptr)
            return label;

        label += "plate" + std::to_string(n->GetPlateIdx()) + "_";
        if (type == ItemType::itObject) {
            label += "object";
        } else if (type == ItemType::itVolume) {
            label += "volume";
            label += std::to_string((int) volume_type);
        } else {
            // assert(0, "unsuport type!");
        }

        return label;
    };

    const bool shift_press  = wxGetKeyState(WXK_SHIFT);
    bool       left_clicked = false;
    if (type & ItemType::itVolume && volume_type != ModelVolumeType::INVALID) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0, 0.0));

        if (m_texture.valid()) {
            ImTextureID                      normal_id = m_texture.get_texture_id();
            int                              off       = int(volume_type) - int(ModelVolumeType::MODEL_PART);
            int                              idx       = ObjList_Texture::texNormalPart + off;
            ObjList_Texture::IM_TEXTURE_NAME name      = ObjList_Texture::IM_TEXTURE_NAME(idx);

            ImGui::PushID((node_label + "name_button").c_str());

            left_clicked       = ImGui::ImageTextButton(icon_size, node_name, normal_id, icon_size,
                                                        m_texture.get_texture_uv0(name, node_selected),
                                                        m_texture.get_texture_uv1(name, node_selected), 0);
            bool right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
            bool left_draged   = false;

            // Our buttons are both drag sources and drag targets here!
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                // Set payload to carry the index of our item (could be anything)
                ImGui::SetDragDropPayload(make_drag_drop_payload_label(node).c_str(), &node, sizeof(node));

                // Display preview (could be anything, e.g. when dragging an image we could decide to display
                // the filename and a small preview of the image, etc.)
                ImGui::ImageTextButton(icon_size, node_name, normal_id, icon_size, m_texture.get_texture_uv0(name, false),
                                       m_texture.get_texture_uv1(name, false), 0);

                ImGui::EndDragDropSource();

                wxDataViewEvent ev;
                ev.SetItem(wxDataViewItem(node));
                OnBeginDrag(ev);

                left_draged = true;
            }

            if (ImGui::BeginDragDropTarget()) {
                const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(make_drag_drop_payload_label(node).c_str());
                if (payload) {
                    wxDataViewEvent ev;
                    ev.SetItem(wxDataViewItem(node));
                    OnDrop(ev);
                }
                ImGui::EndDragDropTarget();
            }

            if (ImGui::IsItemHovered()) {
                ImVec4 text_color = is_dark ? ImVec4(1.0, 1.0, 1.0, 1.0) : ImVec4(0.2, 0.2, 0.2, 1.0);
                ImGui::PushStyleColor(ImGuiCol_Text, text_color);
                ImGui::SetTooltip("%s", node_name);
                ImGui::PopStyleColor();
            }

            ImGui::PopID();

            handle_obj_list_select_event(left_clicked, right_clicked, left_draged, shift_press, node_selected, node, sels);
        }

        ImGui::PopStyleVar(1);

    } else if (type & ItemType::itLayerRoot || type & ItemType::itLayer || type & ItemType::itInfo) {
        ObjList_Texture::IM_TEXTURE_NAME name;
        if (type & ItemType::itLayerRoot) {
            name = ObjList_Texture::texLayerRoot;
        } else if (type & ItemType::itLayer) {
            name = ObjList_Texture::texLayerLeaf;
        } else {
            name = ObjList_Texture::texCutConnector;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0, 0.0));

        ImTextureID normal_id = m_texture.get_texture_id();

        ImGui::PushID((node_label + "name_button").c_str());

        ImGui::ImageTextButton(icon_size, node_name, normal_id, icon_size, m_texture.get_texture_uv0(name, node_selected),
                               m_texture.get_texture_uv1(name, node_selected), 0);

        left_clicked       = ImGui::IsItemClicked(ImGuiMouseButton_Left);
        bool right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);

        if (ImGui::IsItemHovered()) {
            ImVec4 text_color = is_dark ? ImVec4(1.0, 1.0, 1.0, 1.0) : ImVec4(0.2, 0.2, 0.2, 1.0);
            ImGui::PushStyleColor(ImGuiCol_Text, text_color);
            ImGui::SetTooltip("%s", node_name);
            ImGui::PopStyleColor();
        }

        ImGui::PopID();

        if (left_clicked || right_clicked) {
            select_node(node, node_selected);
        }
        ImGui::PopStyleVar(1);

    } else {
        // name

        ImVec2 fp = style.FramePadding;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, fp.y));

        if (type & ItemType::itObject && node->has_lock()) {
            ImGui::PushID((node_label + "cut_button").c_str());

            ObjList_Texture::IM_TEXTURE_NAME name = ObjList_Texture::IM_TEXTURE_NAME::texCut;
            left_clicked                          = ImGui::ImageTextButton(icon_size, node_name, normal_id, icon_size,
                                                                           m_texture.get_texture_uv0(name, node_selected),
                                                                           m_texture.get_texture_uv1(name, node_selected), 0);
            ImGui::PopID();
        } 
        else {
            // Preserve the original alignment
            ImVec2 originalAlign = ImGui::GetStyle().ButtonTextAlign;
            // Set left alignment (0.0f = left alignment, 0.5f = center)
            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
            // The button size is based on the window size.
            left_clicked = ImGui::Button((node_name + node_label + "name_button").c_str(),
                                          ImVec2(ImGui::GetWindowSize().x, 0));
        }

        if (ImGui::IsItemHovered()) {
            ImVec4 text_color = is_dark ? ImVec4(1.0, 1.0, 1.0, 1.0) : ImVec4(0.2, 0.2, 0.2, 1.0);
            ImGui::PushStyleColor(ImGuiCol_Text, text_color);
            ImGui::SetTooltip("%s", node_name);
            ImGui::PopStyleColor();
        }

        bool right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
        bool left_draged   = false;
        if (type & ItemType::itObject) {
            // Our buttons are both drag sources and drag targets here!
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                // Set payload to carry the index of our item (could be anything)
                ImGui::SetDragDropPayload(make_drag_drop_payload_label(node).c_str(), &node, sizeof(node));

                // Display preview (could be anything, e.g. when dragging an image we could decide to display
                // the filename and a small preview of the image, etc.)

                if (printable_name != ObjList_Texture::IM_TEXTURE_NAME::texCount) {
                    ImGui::ImageTextButton(icon_size, node_name, normal_id, icon_size, m_texture.get_texture_uv0(printable_name, false),
                                           m_texture.get_texture_uv1(printable_name, false), 0);
                }

                ImGui::EndDragDropSource();

                wxDataViewEvent ev;
                ev.SetItem(wxDataViewItem(node));
                OnBeginDrag(ev);

                left_draged = true;
            }

            if (ImGui::BeginDragDropTarget()) {
                const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(make_drag_drop_payload_label(node).c_str());
                if (payload) {
                    wxDataViewEvent ev;
                    ev.SetItem(wxDataViewItem(node));
                    OnDrop(ev);
                }
                ImGui::EndDragDropTarget();
            }
        }

        handle_obj_list_select_event(left_clicked, right_clicked, left_draged, shift_press, node_selected, node, sels);

        ImGui::PopStyleVar(1);
    }

    int column_count = 2;
    if (node->HasSupportPainting()) {
        column_count++;
    }
    if (node->HasColorPainting()) {
        column_count++;
    }
    if (node->HasSinking()) {
        column_count++;
    }
    if (node->IsActionEnabled()) {
        column_count++;
    }

    if (ImGui::TableGetColumnCount() > column_count) {
        for (size_t i = 0; i < ImGui::TableGetColumnCount() - column_count; i++) {
            ImGui::TableNextColumn();
        }
    }

    // Support

    if (node->HasSupportPainting()) {
        ImGui::TableNextColumn();

        if (m_texture.valid()) {
            ImTextureID normal_id = m_texture.get_texture_id();

            ImGui::PushID((node_label + "support" + std::to_string(ObjList_Texture::IM_TEXTURE_NAME::texSupportPainting)).c_str());

            if (ImGui::ImageButton(normal_id, icon_size,
                                   m_texture.get_texture_uv0(ObjList_Texture::IM_TEXTURE_NAME::texSupportPainting, node_selected),
                                   m_texture.get_texture_uv1(ObjList_Texture::IM_TEXTURE_NAME::texSupportPainting, node_selected), 0) &&
                is_view3D) {
                select_node(node, node_selected);

                auto event = IntEvent(EVT_OBJ_LIST_COLUMN_SELECT, ObjList_Texture::IM_TEXTURE_NAME::texSupportPainting);
                event.SetEventObject(this);
                wxPostEvent(this, event);
            }

            if (is_view3D && ImGui::IsItemHovered()) {
                ImVec4 text_color = is_dark ? ImVec4(1.0, 1.0, 1.0, 1.0) : ImVec4(0.2, 0.2, 0.2, 1.0);
                ImGui::PushStyleColor(ImGuiCol_Text, text_color);
                ImGui::SetTooltip(_u8L("Click the icon to edit support painting of the object").c_str());
                ImGui::PopStyleColor();
            }

            ImGui::PopID();
        }
    }

    // Seam
    /*ImGui::TableNextColumn();
    {
        if (ImGui::Button("Seam")) {


        }
    }*/

    // Mmu
    if (node->HasColorPainting()) {
        ImGui::TableNextColumn();

        if (m_texture.valid()) {
            ImTextureID normal_id = m_texture.get_texture_id();

            ImGui::PushID((node_label + "Mmu" + std::to_string(ObjList_Texture::IM_TEXTURE_NAME::texColorPainting)).c_str());

            if (ImGui::ImageButton(normal_id, icon_size,
                                   m_texture.get_texture_uv0(ObjList_Texture::IM_TEXTURE_NAME::texColorPainting, node_selected),
                                   m_texture.get_texture_uv1(ObjList_Texture::IM_TEXTURE_NAME::texColorPainting, node_selected), 0)) {
                if (is_view3D) {
                    if (node && node->HasColorPainting()) {
                        select_node(node, node_selected);

                        auto event = IntEvent(EVT_OBJ_LIST_COLUMN_SELECT, ObjList_Texture::IM_TEXTURE_NAME::texColorPainting);
                        event.SetEventObject(this);
                        wxPostEvent(this, event);
                    }
                }
            }

            if (is_view3D && ImGui::IsItemHovered()) {
                ImVec4 text_color = is_dark ? ImVec4(1.0, 1.0, 1.0, 1.0) : ImVec4(0.2, 0.2, 0.2, 1.0);
                ImGui::PushStyleColor(ImGuiCol_Text, text_color);
                ImGui::SetTooltip(_u8L("Click the icon to edit color painting of the object").c_str());
                ImGui::PopStyleColor();
            }

            ImGui::PopID();
        }
    }

    // Sinking
    if (node->HasSinking()) {
        ImGui::TableNextColumn();

        ImTextureID normal_id = m_texture.get_texture_id();

        ImGui::PushID((node_label + "Sinking" + std::to_string(ObjList_Texture::IM_TEXTURE_NAME::texSinking)).c_str());

        if (ImGui::ImageButton(normal_id, icon_size, m_texture.get_texture_uv0(ObjList_Texture::IM_TEXTURE_NAME::texSinking, node_selected),
                               m_texture.get_texture_uv1(ObjList_Texture::IM_TEXTURE_NAME::texSinking, node_selected), 0)) {
            if (is_view3D) {
                if (node && node->HasSinking()) {
                    select_node(node, node_selected);

                    /* auto event = IntEvent(EVT_OBJ_LIST_COLUMN_SELECT, ObjList_Texture::IM_TEXTURE_NAME::texColorPainting);
                     event.SetEventObject(this);
                     wxPostEvent(this, event);*/
                    wxDataViewItem item   = wxDataViewItem(node);
                    Plater*        plater = wxGetApp().plater();
                    GLCanvas3D*    cnv    = plater->canvas3D();
                    int            obj_idx, vol_idx;
                    get_selected_item_indexes(obj_idx, vol_idx, item);
                    if (obj_idx != -1) {
                        Plater::TakeSnapshot(plater, "Shift objects to bed");
                        (*m_objects)[obj_idx]->ensure_on_bed();
                        cnv->reload_scene(true, true);
                        update_info_items(obj_idx);
                        notify_instance_updated(obj_idx);
                    }
                }
            }
        }

        if (is_view3D && ImGui::IsItemHovered()) {
            ImVec4 text_color = is_dark ? ImVec4(1.0, 1.0, 1.0, 1.0) : ImVec4(0.2, 0.2, 0.2, 1.0);
            ImGui::PushStyleColor(ImGuiCol_Text, text_color);
            ImGui::SetTooltip(_u8L("Click the icon to shift this object to the bed").c_str());
            ImGui::PopStyleColor();
        }

        ImGui::PopID();
    }

    // undo settings

    if (node->IsActionEnabled()) {
        ImGui::TableNextColumn();

        ImTextureID normal_id = m_texture.get_texture_id();

        ImGui::PushID((node_label + "undo_settings" + std::to_string(ObjList_Texture::IM_TEXTURE_NAME::texEditing)).c_str());

        if (ImGui::ImageButton(normal_id, icon_size, m_texture.get_texture_uv0(ObjList_Texture::IM_TEXTURE_NAME::texEditing, node_selected),
                               m_texture.get_texture_uv1(ObjList_Texture::IM_TEXTURE_NAME::texEditing, node_selected), 0)) {
            if (is_view3D) {
                if (node && node->IsActionEnabled()) {
                    select_node(node, node_selected);

                    /* auto event = IntEvent(EVT_OBJ_LIST_COLUMN_SELECT, ObjList_Texture::IM_TEXTURE_NAME::texColorPainting);
                     event.SetEventObject(this);
                     wxPostEvent(this, event);*/

                    int            obj_idx, vol_idx;
                    wxDataViewItem item = wxDataViewItem(node);
                    get_selected_item_indexes(obj_idx, vol_idx, item);
                    // wxGetApp().plater()->PopupObjectTable(obj_idx, vol_idx, mouse_pos);
                    if (m_objects_model->GetItemType(item) & itPlate)
                        dynamic_cast<TabPrintPlate*>(wxGetApp().get_plate_tab())->reset_model_config();
                    else if (m_objects_model->GetItemType(item) & itLayer)
                        dynamic_cast<TabPrintLayer*>(wxGetApp().get_layer_tab())->reset_model_config();
                    else
                        dynamic_cast<TabPrintModel*>(wxGetApp().get_model_tab(vol_idx >= 0))->reset_model_config();
                }
            }
        }

        if (is_view3D && ImGui::IsItemHovered()) {
            ImVec4 text_color = is_dark ? ImVec4(1.0, 1.0, 1.0, 1.0) : ImVec4(0.2, 0.2, 0.2, 1.0);
            ImGui::PushStyleColor(ImGuiCol_Text, text_color);
            ImGui::SetTooltip(_u8L("Click the icon to reset all settings of the object").c_str());
            ImGui::PopStyleColor();
        }

        ImGui::PopID();
    }

    // extruder
    ImGui::TableNextColumn();
    {
        wxString ext = node->GetExtruder();
        if (!ext.empty()) {
            std::vector<std::string> extruder_colors;
            std::vector<std::string> ext_names;
            std::vector<ImVec4>      color_values;
            std::vector<ImVec4>      text_colors;

            if (volume_type == ModelVolumeType::PARAMETER_MODIFIER || type & ItemType::itLayer) {
                extruder_colors.emplace_back("#ffffff");
                ext_names.emplace_back(_u8L("default"));
                color_values.emplace_back(ImVec4(0.0, 0.0, 0.0, 0.0));
                text_colors.emplace_back(ImVec4(0.0, 0.0, 0.0, 1.0));
            }
            std::vector<std::string> config_extruder_colors =
                wxGetApp().plater()->get_extruder_colors_from_plater_config(); // like "#0834f1", ...
            for (size_t i = 0; i < config_extruder_colors.size(); i++) {
                const std::string& sub_string = config_extruder_colors.at(i);
                extruder_colors.emplace_back(sub_string);

                size_t t = i + 1;
                ext_names.emplace_back(std::to_string(t));

                wxColor wxc(sub_string);
                ImVec4  color_v4 = ImVec4(wxc.Red() / 255.0, wxc.Green() / 255.0, wxc.Blue() / 255.0, 1.0);
                color_values.emplace_back(color_v4);

                float gray = color_v4.x * 0.299f + color_v4.y * 0.587f + color_v4.z * 0.114f;
                if (gray > 0.5f) {
                    gray = 0.0f;
                } else {
                    gray = 1.0f;
                }
                text_colors.emplace_back(ImVec4(gray, gray, gray, 1.0f));
            }

            int ext_idx = 0;

            if (volume_type == ModelVolumeType::PARAMETER_MODIFIER || type & ItemType::itLayer) {
                if (ext == _(L("default"))) {
                    ext_idx = 0;
                } else {
                    int k   = atoi(ext.c_str());
                    ext_idx = std::clamp<int>(k, 1, (int) extruder_colors.size() - 1);
                }

            } else {
                int k   = atoi(ext.c_str());
                ext_idx = k > 0 ? (k - 1) : 0;
                ext_idx = std::min<int>(ext_idx, (int) extruder_colors.size() - 1);
            }

            ImVec2 pos   = ImGui::GetCursorPos();
            ImVec2 p_min = ImGui::GetCursorScreenPos();

            const ImVec4& filament_color = color_values.at(ext_idx);
            const ImVec4& title_color    = text_colors.at(ext_idx);

            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, filament_color);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, filament_color);
            ImGui::PushStyleColor(ImGuiCol_Text, title_color);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, ImGui::GetFrameHeight() / 2.0);

            const float ext_column_width = ImGui::GetColumnWidth();
            ImGui::PushItemWidth(ext_column_width);

            if (ImGui::BeginCombo((std::string(unique_label) + "_combo").c_str(), "",
                                  ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_HeightLargest)) {
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

                for (int n = 0; n < ext_names.size(); n++) {
                    const std::string& cur_name = ext_names[n];

                    const bool is_selected = (ext_idx == n);

                    const ImVec4  sub_color  = cur_name == _u8L("default") ? ImVec4(0, 0, 0, 0) : color_values.at(n);
                    const ImVec4& text_color = text_colors.at(n);

                    ImGui::PushStyleColor(ImGuiCol_Button, sub_color);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sub_color);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, sub_color);
                    ImGui::PushStyleColor(ImGuiCol_Text, text_color);
                    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));

                    float w     = ImGui::CalcTextSize(_u8L("default").c_str()).x + ImGui::GetStyle().FramePadding.x * 2.0;
                    w           = std::max(w, 50.0f * view_scale);
                    ImVec2 size = ImVec2(w, ImGui::GetFrameHeight());

                    ImVec2 sel_pos = ImGui::GetCursorPos();

                    if (ImGui::Selectable(("##" + cur_name).c_str(), is_selected, ImGuiSelectableFlags_None, size)) {
                        if (cur_name == _u8L("default")) {
                            node->SetExtruder(_(L("default")));
                        } else {
                            node->SetExtruder(ext_names[n]);
                        }

                        wxDataViewItem item = wxDataViewItem(node);
                        if (m_objects_model->GetItemType(item) == ItemType::itObject)
                            m_objects_model->UpdateVolumesExtruderBitmap(item, true);
                        update_filament_in_config(item);
                    }

                    ImGui::SetCursorPos(sel_pos);

                    if (cur_name == _u8L("default")) {
                        ImVec2 pp     = ImGui::GetCursorPos();
                        auto   tex_id = DispConfig().getTextureId(DispConfig::e_tt_retangle_transparent, false, false);
                        ImGui::Image(tex_id, size);
                        ImGui::SetCursorPos(pp);
                    }

                    ImGui::Button(cur_name.c_str(), size);

                    ImGui::PopStyleColor(5);

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::PopStyleVar(1);
                ImGui::EndCombo();
            }

            // draw border for default extruder
            if (ext == _(L("default"))) {
                /*ImVec4 border_color = ImVec4(110.0f / 255.0f, 110.0f / 255.0f, 114.0f / 255.0f, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_Border, border_color);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0);

                float rounding = 9;

                ImVec2 p_max = ImVec2(p_min.x + 40.0f * view_scale, p_min.y + ImGui::GetFrameHeight());
                ImGui::RenderFrameBorder(p_min, p_max, rounding);*/

                ImGui::SetCursorPos(pos);
                auto tex_id = DispConfig().getTextureId(DispConfig::e_tt_rounding_transparent, false, false);
                ImGui::Image(tex_id, ImVec2(ext_column_width, ImGui::GetFrameHeight()));

                /*ImGui::PopStyleColor(1);
                ImGui::PopStyleVar(1);*/
            }

            // draw text
            {
                float       item_width    = ImGui::CalcItemWidth();
                std::string ext_name      = ext_names[ext_idx];
                ImVec2      text_size     = ImGui::CalcTextSize(ext_name.c_str());
                bool        need_tooltips = false;
                if (text_size.x > item_width) {
                    const char* ellipsis       = "...";
                    const float ellipsis_width = ImGui::CalcTextSize(ellipsis).x;
                    size_t      name_length    = ext_name.length();
                    for (size_t i = name_length - 1; i >= 0; i--) {
                        std::string sub_name           = ext_name.substr(0, i);
                        std::string name_with_ellipsis = sub_name + ellipsis;
                        const float sub_width          = ImGui::CalcTextSize((name_with_ellipsis).c_str()).x;
                        if (sub_width <= item_width) {
                            ext_name      = name_with_ellipsis;
                            text_size.x   = sub_width;
                            need_tooltips = true;
                            break;
                        }
                    }
                }

                float x = pos.x + (item_width - text_size.x) * 0.5f;
                ImGui::SetCursorPos(ImVec2(x, pos.y + style.FramePadding.y));

                ImGui::Text("%s", ext_name.c_str());

                if (need_tooltips) {
                    if (ImGui::IsItemHovered()) {
                        ImVec4 text_color = is_dark ? ImVec4(1.0, 1.0, 1.0, 1.0) : ImVec4(0.2, 0.2, 0.2, 1.0);
                        ImGui::PushStyleColor(ImGuiCol_Text, text_color);

                        ImGui::SetTooltip("%s", ext_names[ext_idx].c_str());

                        ImGui::PopStyleColor();
                    }
                }

                ImGui::PopItemWidth();
                ImGui::PopStyleVar(1);
                ImGui::PopStyleColor(3);
            }
        }
    }

    if (node_selected) {
        ensure_current_item_visible_imgui();
    }

    if (open) {
        auto type = node->GetType();

        if (type & ItemType::itObject || type & ItemType::itLayerRoot) {
            for (int child_n = 0; child_n < count; child_n++)
                render_volume(children[child_n]);
        }

        ImGui::TreePop();
    }

    ImGui::PopStyleColor(1);

    node->set_open(open);
}

void ObjectList::handle_obj_list_select_event(bool                     left_clicked,
                                              bool                     right_clicked,
                                              bool                     left_draged,
                                              bool                     shift_press,
                                              bool                     node_selected,
                                              ObjectDataViewModelNode* node,
                                              wxDataViewItemArray&     selecteds)
{
    if ((left_clicked || left_draged) && !shift_press) {
        select_node(node, node_selected);
    }

    // multi selection when press shift key
    if (left_clicked && shift_press && !node_selected && m_last_selected_item) {
        // has same parent
        ObjectDataViewModelNode* sel        = static_cast<ObjectDataViewModelNode*>(m_last_selected_item.GetID());
        ObjectDataViewModelNode* the_parent = node->GetParent();
        if (the_parent && the_parent == sel->GetParent()) {
            int idx0 = the_parent->GetChildIndex(sel);
            int idx1 = the_parent->GetChildIndex(node);

            int  start       = std::min(idx0, idx1);
            int  end         = std::max(idx0, idx1);
            bool need_update = false;
            for (int j = start; j <= end; j++) {
                ObjectDataViewModelNode* sub_child = the_parent->GetNthChild(j);
                wxDataViewItem           sub_item  = wxDataViewItem(sub_child);
                if (std::find(selecteds.begin(), selecteds.end(), sub_item) == selecteds.end()) {
                    selecteds.push_back(sub_item);
                    need_update = true;
                }
            }
            if (need_update) {
                select_items(selecteds);
                selection_changed();
            }
        }
    }

    if (right_clicked) {
        if (node_selected) {
        } else {
            select_node(node, node_selected);
        }

        // show_context_menu(true);
        auto event = IntEvent(EVT_OBJ_LIST_COLUMN_SELECT, 0);
        event.SetEventObject(this);
        wxPostEvent(this, event);
    }

    if (left_clicked && node_selected && selecteds.size() == 1 && !shift_press) {
        rename_item();
    }
}

void ObjectList::select_node(ObjectDataViewModelNode* node, bool node_select_state)
{
    if (node != nullptr) {
        m_last_selected_item = wxDataViewItem(node);
#ifdef __WXMSW__
        m_last_selected_column = -1;
#endif //__WXMSW__

        // multi select when CONTROL key press
        if (wxGetKeyState(WXK_CONTROL)) {
            if (node_select_state) {
                Unselect(m_last_selected_item);
            } else {
                Select(m_last_selected_item);
            }
            selection_changed();
        } else {
            // selected_object(n); // avoid set wxwidget focus
            select_item(wxDataViewItem(node));
            // ensure_current_item_visible();
            selection_changed();
        }
    }
}

void GUI::ObjectList::render_current_device_name(const float max_right)
{
    const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
    if (!current_device.valid)
        return;

    std::string device_name             = current_device.name;
    auto        remake_text_to_fit_size = [max_right](const std::string& input_text) {
        const float device_label_max_right   = max_right;
        const float device_label_max_width   = device_label_max_right - ImGui::GetCursorPosX() - ImGui::GetStyle().ItemSpacing.x;
        const float origin_device_name_width = ImGui::CalcTextSize(input_text.c_str()).x;

        if (origin_device_name_width <= device_label_max_width) {
            return input_text;
        } else {
            const char* ellipsis       = "...";
            const float ellipsis_width = ImGui::CalcTextSize(ellipsis).x;
            size_t      name_length    = input_text.length();

            for (size_t i = name_length - 1; i >= 0; i--) {
                std::string sub_name           = input_text.substr(0, i);
                std::string name_with_ellipsis = sub_name + ellipsis;
                const float sub_width          = ImGui::CalcTextSize((name_with_ellipsis).c_str()).x;
                if (sub_width <= device_label_max_width) {
                    return name_with_ellipsis;
                }
            }
        }
        return input_text;
    };

    // display current device name
    /*std::string device_name = RemotePrint::DeviceDB::getInstance().get_current_device_name();
    if (device_name.empty()) {
        std::string tips        = _u8L("No current device");
        std::string remake_text = remake_text_to_fit_size(tips);

        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), remake_text.c_str());

        if (remake_text != tips && ImGui::IsItemHovered()) {
            ImGui::SetTooltip(tips.c_str());
        }

        return;
    }*/

    std::string show_text   = _u8L("Current device:") + " " + device_name;
    std::string remake_text = remake_text_to_fit_size(show_text);

    ImGui::TextColored(ImGuiWrapper::COL_CREALITY, "%s", remake_text.c_str());

    if (ImGui::IsItemHovered()) {
        bool   is_dark    = wxGetApp().dark_mode();
        ImVec4 text_color = is_dark ? ImVec4(1.0, 1.0, 1.0, 1.0) : ImVec4(0.2, 0.2, 0.2, 1.0);
        ImGui::PushStyleColor(ImGuiCol_Text, text_color);
        ImGui::SetTooltip(_u8L("Can be set in the \"Worktop\" or \"Device\"").c_str());
        ImGui::PopStyleColor();
    }
}

void ObjectList::render_printer_preset_by_ImGui()
{
    float  scale     = wxGetApp().plater()->get_current_canvas3D()->get_scale();
    ImVec2 icon_size = ImVec2(22 * scale, 22 * scale);

    const ImVec4 transparent(0.0f, 0.0f, 0.0f, 0.0f);
    bool         is_dark = wxGetApp().dark_mode();
    ImGui::PushStyleColor(ImGuiCol_Header, ImGuiWrapper::COL_CREALITY);
    ImVec4 text_color = is_dark ? ImVec4(1.0, 1.0, 1.0, 1.0) : ImVec4(0.2, 0.2, 0.2, 1.0);
    ImGui::PushStyleColor(ImGuiCol_Text, text_color);

    // Title
    ImGui::AlignTextToFramePadding();
    // ImGuiWrapper::title(_u8L("Printer"));
    ImGui::SetWindowFontScale(1.2f);
    ImGuiWrapper& imgui = *wxGetApp().imgui();
    imgui.title(_u8L("Printer"), true);
    ImGui::SetWindowFontScale(1.0f);

    ImGui::SameLine();

    const float device_label_max_right = 240.0f * scale;
    ImGui::SetCursorPosX(device_label_max_right);

    // Setting
    if (!m_texture.valid()) {
        m_texture.init_svg_texture();
    }
    ImTextureID normal_id = m_texture.get_texture_id();

    if (m_texture.valid()) {
        ImGui::PushID(ObjList_Texture::IM_TEXTURE_NAME::texSetting);

        if (ImGui::ImageButton(normal_id, icon_size, m_texture.get_texture_uv0(ObjList_Texture::IM_TEXTURE_NAME::texSetting, false),
                               m_texture.get_texture_uv1(ObjList_Texture::IM_TEXTURE_NAME::texSetting, false), 0)) {
            wxGetApp().run_wizard(ConfigWizard::RR_USER, ConfigWizard::SP_PRINTERS);
        }

        ImGui::PopID();
    }

    ImGui::SameLine();

    // collapse button
    ImGui::PushID(ObjList_Texture::IM_TEXTURE_NAME::texCollapse);

    if (ImGui::ImageButton(normal_id, icon_size, m_texture.get_texture_uv0(ObjList_Texture::IM_TEXTURE_NAME::texCollapse, false),
                           m_texture.get_texture_uv1(ObjList_Texture::IM_TEXTURE_NAME::texCollapse, false), 0)) {
        m_left_panel_fold = true;
        wxGetApp().plater()->get_current_canvas3D()->set_left_panel_fold(GLCanvas3D::CanvasView3D, true);
    }

    ImGui::PopID();

    const float device_name_max_right = 300.0f * scale;
    //render_current_device_name(device_name_max_right);

    ImGui::Dummy(ImVec2(0, 0));

    /**************************************************************************************/
    // is Creality vendor
    bool wifi_icon_show = wxGetApp().preset_bundle->is_cx_vendor();
    ImVec2 wifi_icon_size = wifi_icon_show ? icon_size : ImVec2{0, 0};

    // printer model img
    update_printer_model_texture();
    auto& printerTextures = m_png_textures->get(ObjList_Png_Texture_Wrapper::pngTexPrinterModel);
    ImVec2 modelIconSize   = {24 * scale, 32 * scale};
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, 0});
    if (printerTextures->vaild())
    {
        float printer_scale         = std::min(
            modelIconSize.x / printerTextures->get_width(), 
            modelIconSize.y / printerTextures->get_height());
        float scaled_width  = printerTextures->get_width() * printer_scale;
        float scaled_height = printerTextures->get_height() * printer_scale;
        modelIconSize       = {scaled_width, scaled_height};
        ImGui::Image((ImTextureID) printerTextures->get_id(), modelIconSize);
    }
    else
        ImGui::Dummy(modelIconSize);
    ImGui::PopStyleVar(1);
        
    ImGui::SameLine();

    // Printer
    SidebarPrinter&          bar               = wxGetApp().plater()->sidebar_printer();
    std::vector<std::string> items             = bar.texts_of_combo_printer();
    int                      item_selected_idx = bar.get_selection_combo_printer(); // Here we store our selection data as an index.

    // Pass in the preview value visible before opening the combo (it could technically be different contents or not pulled from items[])

    ImGuiContext&     g      = *GImGui;
    ImGuiWindow*      window = g.CurrentWindow;
    const ImGuiStyle& style  = g.Style;
    ImVec4 border_color = ImVec4(110.0f / 255.0f, 110.0f / 255.0f, 114.0f / 255.0f, 1.0f);
    // ImGui::PushStyleColor(ImGuiCol_Border, border_color);

    float  rounding            = 4.0f * scale;
    float  item_height         = 22.0f * scale;
    float  pading              = 2.0f * scale;
    float  printer_combo_width = (250.0f * scale - modelIconSize[0] - wifi_icon_size[0]);

    window->DC.CursorPos += {0, modelIconSize[1] / 2 - (item_height + pading * 2.0f) / 2};
    ImVec2 p_min = window->DC.CursorPos; 
    ImVec2 p_max = ImVec2(p_min.x + (280 * scale - modelIconSize[0] - wifi_icon_size[0]), p_min.y + item_height + pading * 2.0f);
    // ImGui::RenderFrameBorder(p_min, p_max, rounding);

    window->DrawList->AddLine(ImVec2(p_min.x + printer_combo_width + 1, p_min.y), ImVec2(p_min.x + printer_combo_width + 1, p_max.y - 1.0),
                              ImGui::GetColorU32(border_color), 1.0f);

    window->DC.CursorPos = window->DC.CursorPos + ImVec2(pading, pading);

    const char* combo_preview_value = "";
    if (0 <= item_selected_idx && item_selected_idx < items.size()) {
        combo_preview_value = items[item_selected_idx].c_str();
    }

    ImGui::PushItemWidth(printer_combo_width);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);
    ImGui::PushStyleColor(ImGuiCol_Header, ImGuiWrapper::COL_CREALITY);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGuiWrapper::COL_CREALITY);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGuiWrapper::COL_CREALITY);

    static bool next_loop_open = false;
    static int  selected_idx   = -1;
    if (next_loop_open && selected_idx >= 0 && selected_idx < items.size()) {
        next_loop_open = false;
        bar.select_printer_preset(items[selected_idx], selected_idx);
        selected_idx = -1;
    }
    // set preset bundle device by mac
    static std::string last_preset_name;
    auto               cur_frame_preset_name = wxGetApp().preset_bundle->printers.get_selected_preset_name();
    if (!cur_frame_preset_name.empty() && last_preset_name != cur_frame_preset_name)
    {
        last_preset_name = cur_frame_preset_name;
        set_cur_device_by_cur_preset();
    }

    bool selected_match = false;

    auto match_func = [](const std::string& device_model, const std::string& preset_model) {
        bool result = (device_model == preset_model);
        if (!result) {
            if (preset_model.length() > device_model.length()) {
                std::string sub = preset_model.substr(preset_model.length() - device_model.length(), device_model.length());
                result          = (sub == device_model);
            }
        }
        return result;
    };

    const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
    std::string       model_name     = current_device.modelName;

    std::string selected_preset_model = (&Slic3r::GUI::wxGetApp().preset_bundle->printers.get_selected_preset().config)
                                            ->opt_string("printer_model");

    if (!model_name.empty() && !selected_preset_model.empty()) {
        selected_match = match_func(model_name, selected_preset_model);
    }

    if (selected_match) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGuiWrapper::COL_CREALITY);
    }

    auto remake_text_to_fit_size = [scale](const std::string& input_text) {
        const float device_label_max_right   = 225.0f * scale;
        const float device_label_max_width   = device_label_max_right - ImGui::GetCursorPosX() - ImGui::GetStyle().ItemSpacing.x;
        const float origin_device_name_width = ImGui::CalcTextSize(input_text.c_str()).x;

        if (origin_device_name_width <= device_label_max_width) {
            return input_text;
        } else {
            const char* ellipsis       = "...";
            const float ellipsis_width = ImGui::CalcTextSize(ellipsis).x;
            wxString wxText = from_u8(input_text);
            size_t      name_length    = wxText.length();

            for (size_t i = name_length - 1; i >= 0; i--) {
                wxText.erase(wxText.end() - 1);
                wxString name_with_ellipsis = wxText + ellipsis;
                const float sub_width          = ImGui::CalcTextSize(name_with_ellipsis.ToUTF8().data()).x;
                if (sub_width <= device_label_max_width) {
                    return name_with_ellipsis.ToStdString();
                }
            }
        }
        return input_text;
    };

    auto   p  = ImGui::GetCursorScreenPos(); // ImGui::GetCursorPos();
    #ifdef __APPLE__
    p = ImVec2(p.x/scale,p.y/scale);
    auto   p2 = ImVec2(p.x + 200, p.y + 30.0f);
    wxRect rect(p.x, p.y + 34, p2.x - p.x, p2.y - p.y);
    #else
    auto   p2 = ImVec2(p.x + 200 * scale, p.y + 30.0f * scale);
    wxRect rect(p.x, p.y + 34 * scale, p2.x - p.x, p2.y - p.y);
    #endif
  /*  UITour::Instance().AddStep(0, rect, _L("Here you can select and add your printer presets"), "", "userGuide_step1", "", wxRIGHT);*/
    bool click = ImGui::CPBBLBeginCombo("##combo_printer", remake_text_to_fit_size(combo_preview_value).c_str(),
                                        ImGuiComboFlags_HeightLargest, 225.0f * scale, 30.0f * scale);
    m_PrintCombo = rect;
    if (click) {
        if (selected_match) {
            ImGui::PopStyleColor();
        }

        int item_count = items.size();
        for (int n = 0; n < item_count; n++) {
            const bool is_selected = (item_selected_idx == n);
            if (ImGui::CPSelectable(items[n].c_str(), is_selected)) {
                // bar.select_printer_preset(items[n], n);
                next_loop_open = true;
                selected_idx   = n;
            }

            /*if (is_selected) {
                if (selected_match) {
                    ImGui::SameLine();
                    ImGui::Image(normal_id, ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize()),
                                 m_texture.get_texture_uv0(ObjList_Texture::IM_TEXTURE_NAME::texPresetMatch, true),
                                 m_texture.get_texture_uv1(ObjList_Texture::IM_TEXTURE_NAME::texPresetMatch, true));
                }
            } else {
                bool tmp_match = false;
                if (!model_name.empty()) {
                    Preset* a_preset = Slic3r::GUI::wxGetApp().preset_bundle->printers.find_preset(items[n]);

                    if (a_preset) {
                        std::string a_preset_model = a_preset->config.opt_string("printer_model");
                        if (!a_preset_model.empty()) {
                            tmp_match = match_func(model_name, a_preset_model);
                        }
                    }
                }

                if (tmp_match) {
                    ImGui::SameLine();
                    ImGui::Image(normal_id, ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize()),
                                 m_texture.get_texture_uv0(ObjList_Texture::IM_TEXTURE_NAME::texPresetMatch, true),
                                 m_texture.get_texture_uv1(ObjList_Texture::IM_TEXTURE_NAME::texPresetMatch, true));
                }
            }*/

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    } else {
        if (selected_match) {
            ImGui::PopStyleColor();
        }
    }

    ImGui::PopStyleColor(4);
    ImGui::PopItemWidth();

    {
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", combo_preview_value);
            ImGui::PushStyleColor(ImGuiCol_Border, ImGuiWrapper::COL_CREALITY);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Border, border_color);
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0);
        ImGui::RenderFrameBorder(p_min, p_max, rounding);

        ImGui::PopStyleVar(1);
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();
    auto editWidth = p_max.x - p_min.x - printer_combo_width;
    window->DC.CursorPos.x = p_max.x - editWidth / 2 - icon_size.x / 2;
    ImGui::PushID(ObjList_Texture::IM_TEXTURE_NAME::texEdit);

    
    auto itemSize1 = ImGui::GetCursorScreenPos(); // ImGui::GetCursorPos();
    if (ImGui::ImageButton(normal_id, icon_size, m_texture.get_texture_uv0(ObjList_Texture::IM_TEXTURE_NAME::texEdit, false),
                           m_texture.get_texture_uv1(ObjList_Texture::IM_TEXTURE_NAME::texEdit, false), 0)) {
        SidebarPrinter& bar = wxGetApp().plater()->sidebar_printer();
        bar.edit_filament();
    }

    
    if (ImGui::IsItemHovered()) {
        ImVec4 text_color = is_dark ? ImVec4(1.0, 1.0, 1.0, 1.0) : ImVec4(0.2, 0.2, 0.2, 1.0);
        ImGui::PushStyleColor(ImGuiCol_Text, text_color);
        ImGui::SetTooltip(_u8L("Click to edit preset").c_str());
        ImGui::PopStyleColor();
    }

    ImGui::PopID();
    ImVec2 itemSitemSizeize = ImGui::GetItemRectSize();
    #ifdef __APPLE__
    itemSize1 = ImVec2(itemSize1.x/scale,itemSize1.y/scale);
    itemSitemSizeize = ImVec2(itemSitemSizeize.x/scale,itemSitemSizeize.y/scale);
    rect                    = wxRect(itemSize1.x+28 , itemSize1.y + 37 , itemSitemSizeize.x, itemSitemSizeize.y);
    #else
    rect                    = wxRect(itemSize1.x+28 * scale, itemSize1.y + 37 * scale, itemSitemSizeize.x, itemSitemSizeize.y);
    #endif

    m_WifiBtn = rect; 
    // wifi

    auto& wifiTexture = m_png_textures->get(ObjList_Png_Texture_Wrapper::pngTexOnlineDevice);
    if (!wifiTexture->vaild())
    {
        wifiTexture->load_from_png_file(Slic3r::resources_dir() + "/images/wifi.png", true, GLTexture::None, false);
        update_printer_device_list_data("Creality", true);
    }

    auto  texture_type = is_dark ? ObjList_Png_Texture_Wrapper::pngTexOnlineDeviceDarkGray :
                                   ObjList_Png_Texture_Wrapper::pngTexOnlineDeviceGray;
    auto  png_path = is_dark ? Slic3r::resources_dir() + "/images/wifi_dark_gray.png" : Slic3r::resources_dir() + "/images/wifi_gray.png";
    auto& wifiTextureGray = m_png_textures->get(texture_type);
    if (!wifiTextureGray->vaild()) {
        wifiTextureGray->load_from_png_file(png_path, true, GLTexture::None, false);
    }
     
    auto deviceListID = "##DeviceList";
    if (wifi_icon_show)
    { 
        update_printer_device_list_data("Creality");
        ImGui::SameLine();
        auto originCursorY     = window->DC.CursorPos.y;
        window->DC.CursorPos.x = p_max.x + 2.0f * scale;
        window->DC.CursorPos.y = p_min.y + (p_max.y - p_min.y) / 2 - (20.0f * scale)/2;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, 0});
        auto wifi_id = (ImTextureID) wifiTexture->get_id();
        if (m_device_list_data.datas.empty() || 
            !(current_device.valid && !current_device.address.empty()))
            wifi_id = (ImTextureID) wifiTextureGray->get_id();
        
        if (ImGui::ImageButton(wifi_id, {20.0f * scale, 20.0f * scale}))
        {
            ADD_TEST_RESPONE("WIFI", "ENTRY", 0, "");
            // update printer list
            ImGui::OpenPopup(deviceListID);
        }

        if (m_device_list_popup_open_request)
        {
            if (!(current_device.valid && !current_device.address.empty()))
                ImGui::OpenPopup(deviceListID);
            m_device_list_popup_open_request = false;
        }
   
        if (m_device_list_popup_opened)
        {
            ImVec4 mask_col    = is_dark ? ImVec4{0.8588, 0.8588, 0.8588, 0.1} : ImVec4{0.5294, 0.5569, 0.6039, 0.1};
            ImU32  mask_colu32 = IM_COL32(mask_col.x * 255, mask_col.y * 255, mask_col.z * 255, mask_col.w * 255);
            ImGui::GetCurrentWindow()->DrawList->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), mask_colu32, 4.0 * scale);
        }
        ImGui::PopStyleVar(1);
        if (ImGui::IsItemHovered())
            if (current_device.valid)
                ImGui::SetTooltip(("%s", current_device.name.empty() ? current_device.address : current_device.name).c_str());
            else
                ImGui::SetTooltip(_u8L("Click to bind the device").c_str());
        
        // draw device list popup
        ImVec2 popupSize{(336 + 15) * scale, 360 * scale};
        ImGui::SetNextWindowPos({p_max.x, window->DC.CursorPos.y - 5 * scale});
        if (m_device_list_data.datas.empty())
            ImGui::SetNextWindowSize({popupSize.x, 80 * scale});
        else
            ImGui::SetNextWindowSize(popupSize);

        ImVec4 dark_sep_color(80 / 255.0, 80 / 255.0, 82 / 255.0, 1.0);
        ImVec4 light_sep_color(214.0 / 255, 214.0 / 255, 220.0 / 255, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Separator, is_dark ? dark_sep_color : light_sep_color);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, is_dark ? ImVec4{0.1922f, 0.1922f, 0.1922f, 1.0f} : ImVec4{1.0f, 1.0f, 1.0f, 1.0f});
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6 * scale);
        if (m_device_list_popup_opened = ImGui::BeginPopup(deviceListID)) {
            draw_device_list_popup();
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(1);

        window->DC.CursorPos.y = originCursorY;
        ImGui::Dummy(ImVec2{0, p_max.y - p_min.y});
    }

    /*ImGui::SameLine();

    if (ImGui::Button("##connection_btn", ImVec2(15, 15))) {
        wxWindow* w = dynamic_cast<wxWindow*>(Slic3r::GUI::wxGetApp().mainframe);
        if (w) {
            PhysicalPrinterDialog dlg(w);
            dlg.ShowModal();
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(_u8L("Connection").c_str());
    }*/

    ImGui::Dummy(ImVec2(0, 0));

    ImVec2 t_pos         = window->DC.CursorPos;
    window->DC.CursorPos = window->DC.CursorPos + ImVec2(0.0f, pading);

    /**************************************************************************************/

    std::vector<std::string> bed_types = bar.texts_of_bed_type_list();
    if (!bed_types.empty()) {
        // Bed type
        ImGui::AlignTextToFramePadding();
        ImGui::Text(_u8L("Bed type").c_str());
        ImGui::SameLine();
        float bed_type_combo_width = 280 * scale - ImGui::GetItemRectSize().x - style.FramePadding.x * 2;
        {
            ImGui::PushItemWidth(bed_type_combo_width);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);
            ImGui::PushStyleColor(ImGuiCol_Header, ImGuiWrapper::COL_CREALITY);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGuiWrapper::COL_CREALITY);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGuiWrapper::COL_CREALITY);

            int bed_type_selected_idx = bar.get_selection_bed_type(); // Here we store our selection data as an index.
            // Pass in the preview value visible before opening the combo (it could technically be different contents or not pulled
            // from items[])
            const char* combo_preview_value = "";
            void*       original_data_ptr   = bed_types.data();     // 记录原始数据指针
            size_t      original_capacity   = bed_types.capacity(); // 记录原始容量
            if (0 <= bed_type_selected_idx && bed_type_selected_idx < bed_types.size()) {
                combo_preview_value = bed_types[bed_type_selected_idx].c_str();
            } else {
                BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " bed_type_selected_idx out of bed_types.size!!"
                                         << "bed_types.size=" << bed_types.size();
                for (int i = 0; i < bed_types.size(); i++) {
                    BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " bed_types[" << i << "]=" << bed_types[i];
                }
                boost::log::core::get()->flush();
            }

            bool bed_type_enabled = bar.get_bed_type_enable_status();
            if (!bed_type_enabled) {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4, 0.4, 0.4, 1.0));
            }

            if (ImGui::BBLBeginCombo("##combo_bed_type", remake_text_to_fit_size(combo_preview_value).c_str(),
                                     ImGuiComboFlags_HeightLargest, 200.0f * scale, 30.0f * scale)) {
                for (int n = 0; n < bed_types.size(); n++) {
                    const bool is_selected = (bed_type_selected_idx == n);
                    if (ImGui::Selectable(bed_types[n].c_str(), is_selected)) {
                        BOOST_LOG_TRIVIAL(warning) << "n=" << n << "; bed_types=" << bed_types[n].c_str();
                        bar.select_bed_type(n);
                    }

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }

                ImGui::EndCombo();
            }
            if (!bed_type_enabled) {
                ImGui::PopStyleColor();
                ImGui::PopItemFlag();
            }

            ImGui::PopStyleColor(4);
            ImGui::PopItemWidth();

            ImVec2 min  = ImGui::GetItemRectMin();
            ImVec2 size = ImGui::GetItemRectSize();
            // render border
            if (ImGui::IsItemHovered()) {
                if (bed_types.data() != original_data_ptr || bed_types.capacity() != original_capacity) {
                    BOOST_LOG_TRIVIAL(error) << __FUNCTION__
                                             << " [BED_TYPE_DEBUG] Container reallocated! Original data=" << original_data_ptr
                                             << ", Current data=" << bed_types.data() << ", Original capacity=" << original_capacity
                                             << ", Current capacity=" << bed_types.capacity();
                    boost::log::core::get()->flush();
                }
                const char* current_ptr = "";
                if (0 <= bed_type_selected_idx && bed_type_selected_idx < bed_types.size()) {
                    current_ptr = bed_types[bed_type_selected_idx].c_str();
                }

                if (combo_preview_value != current_ptr) {
                    BOOST_LOG_TRIVIAL(error) << __FUNCTION__
                                             << " [BED_TYPE_DEBUG] Pointer mismatch detected! Original=" << (void*) combo_preview_value
                                             << ";Current=" << (void*) current_ptr;
                    boost::log::core::get()->flush();
                }
                if (!combo_preview_value) {
                    BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " combo_preview_value is null!!!" ;
                    boost::log::core::get()->flush();
                }

                ImGui::SetTooltip("%s", combo_preview_value);
                ImGui::PushStyleColor(ImGuiCol_Border, ImGuiWrapper::COL_CREALITY);
            } else {
                ImGui::PushStyleColor(ImGuiCol_Border, border_color);
            }

            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0);

            ImVec2 p_min = ImVec2(min.x - 1, t_pos.y);
            ImVec2 p_max = ImVec2(p_min.x + bed_type_combo_width + 2, p_min.y + item_height + pading * 2.0f);
            ImGui::RenderFrameBorder(p_min, p_max, rounding);

            ImGui::PopStyleVar(1);
            ImGui::PopStyleColor(1);
        }
    }

    ImGui::PopStyleColor(2);
}

void ObjectList::render_unfold_button()
{
    float scale = wxGetApp().plater()->get_current_canvas3D()->get_scale();
    if (!m_texture.valid()) {
        m_texture.init_svg_texture();
    }

    if (m_texture.valid()) {
        ImTextureID normal_id = m_texture.get_texture_id();

        ImGui::PushID(ObjList_Texture::IM_TEXTURE_NAME::texCollapse);

        ImVec2 uv0 = m_texture.get_texture_uv0(ObjList_Texture::IM_TEXTURE_NAME::texCollapse, false);
        ImVec2 uv1 = m_texture.get_texture_uv1(ObjList_Texture::IM_TEXTURE_NAME::texCollapse, false);
        // mirror:left to right
        if (ImGui::ImageButton(normal_id, ImVec2(22 * scale, 22 * scale), ImVec2(uv1.x, uv0.y), ImVec2(uv0.x, uv1.y), 0)) {
            m_left_panel_fold = false;
            wxGetApp().plater()->get_current_canvas3D()->set_left_panel_fold(GLCanvas3D::CanvasView3D, false);
        }

        ImGui::PopID();
    }
}

bool ObjectList::get_object_list_window_focus() { return m_obj_list_window_focus; }

void ObjectList::set_object_list_window_focus(bool f)
{
    if (m_obj_list_window_focus != f)
        m_obj_list_window_focus = f;
}

bool ObjectList::on_char(wxKeyEvent& evt)
{
    bool can_edit = wxGetApp().plater()->get_current_canvas3D()->get_canvas_type() != GLCanvas3D::CanvasAssembleView;
    if (!can_edit)
        return false;

    if (!get_object_list_window_focus())
        return false;

    int keyCode   = evt.GetKeyCode();
    int ctrlMask  = wxMOD_CONTROL;
    int shiftMask = wxMOD_SHIFT;

    // CTRL is pressed
    bool CTRL_pressed = ((evt.GetModifiers() & ctrlMask) != 0);
    bool processed    = false;
    switch (keyCode) {
    case WXK_RETURN:
    case WXK_SPACE: {
        wxDataViewItemArray sels;
        GetSelections(sels);
        if (!CTRL_pressed && sels.size() == 1) {
            rename_plate();
            rename_item();
            processed = true;
        }
        break;
    }

    case WXK_DELETE: {
        // process item(itLayer, itLayerRoot) delete event
        // others will pass by: EVT_GLTOOLBAR_DELETE -> Plater::remove_selected -> GLCanvas3D::delete_selected -> Selection::erase
        wxDataViewItemArray sels;
        GetSelections(sels);
        if (!sels.IsEmpty()) {
            auto& f = sels.front();
            if (f.IsOk()) {
                ItemType type = m_objects_model->GetItemType(f);
                if (type == ItemType::itLayer || type == ItemType::itLayerRoot) {
                    remove();
                    processed = true;
                }
            }
        }
    }
    }

    return processed;
}

void ObjectList::on_key(wxKeyEvent& evt)
{
    bool can_edit = wxGetApp().plater()->get_current_canvas3D()->get_canvas_type() != GLCanvas3D::CanvasAssembleView;
    if (!can_edit)
        return;

    if (!get_object_list_window_focus())
        return;

    const int keyCode = evt.GetKeyCode();

    bool node_selected    = false;
    auto select_node_func = [this, node_selected](ObjectDataViewModelNode* n) {
        if (n != nullptr) {
            m_last_selected_item = wxDataViewItem(n);
#ifdef __WXMSW__
            m_last_selected_column = -1;
#endif //__WXMSW__

            {
                // selected_object(n); // will set wxwidget focus
                select_item(wxDataViewItem(n));
                // ensure_current_item_visible();
                selection_changed();
            }
        }
    };

    auto select_plate_by_key = [this, keyCode](ObjectDataViewModelNode* plate) {
        if (plate->GetType() & ItemType::itPlate) {
            int     new_plate_idx = keyCode == WXK_UP ? plate->GetPlateIdx() - 1 : plate->GetPlateIdx() + 1;
            Plater* the_plater    = wxGetApp().plater();
            if (the_plater->is_preview_shown()) {
                the_plater->select_sliced_plate(new_plate_idx);
            } else {
                the_plater->select_plate(new_plate_idx);
            }

            the_plater->deselect_all();
        }
    };

    if (keyCode == WXK_UP || keyCode == WXK_DOWN) {
        // up or down to active item

        wxDataViewItemArray sels;
        GetSelections(sels);

        if (sels.IsEmpty())
            return;

        wxDataViewItem&          the_item = sels.Last();
        ObjectDataViewModelNode* the_node = static_cast<ObjectDataViewModelNode*>(the_item.GetID());

        wxDataViewItemArray children;
        m_objects_model->GetChildren(the_item, children);
        wxDataViewItem           parent      = m_objects_model->GetParent(the_item);
        ObjectDataViewModelNode* parent_node = nullptr;
        if (parent != nullptr && parent.IsOk()) {
            parent_node = static_cast<ObjectDataViewModelNode*>(parent.GetID());
        }
        int current_idx = -1;
        if (parent_node) {
            current_idx = parent_node->GetChildIndex(the_node);
        }

        if (keyCode == WXK_DOWN) {
            // selecte first child
            if (!children.IsEmpty() && the_node->get_open()) {
                wxDataViewItem&          front_item = children.front();
                ObjectDataViewModelNode* node       = static_cast<ObjectDataViewModelNode*>(front_item.GetID());
                select_node_func(node);
                return;
            }

            if (parent_node) {
                size_t next_idx = current_idx + 1;
                if (next_idx < parent_node->GetChildCount()) {
                    // select next brother
                    ObjectDataViewModelNode* new_node = parent_node->GetNthChild(next_idx);
                    select_node_func(new_node);

                } else {
                    // select next parent
                    ObjectDataViewModelNode* tmp_parent = parent_node;
                    ObjectDataViewModelNode* tmp_child  = the_node;
                    while (tmp_parent->GetParent() && next_idx >= tmp_parent->GetChildCount()) {
                        tmp_child  = tmp_parent;
                        tmp_parent = tmp_child->GetParent();
                        next_idx   = tmp_parent->GetChildIndex(tmp_child) + 1;
                    }

                    if (next_idx < tmp_parent->GetChildCount()) {
                        ObjectDataViewModelNode* node = tmp_parent->GetNthChild(next_idx);
                        select_node_func(node);
                    } else if (tmp_parent->GetType() & ItemType::itPlate) {
                        select_plate_by_key(tmp_parent);
                    }
                }
            } else {
                select_plate_by_key(the_node);
            }
        } else {
            // WXK_UP
            if (parent_node) {
                int pre_idx = current_idx - 1;
                if (pre_idx >= 0) {
                    // select pre brother
                    ObjectDataViewModelNode* new_node = parent_node->GetNthChild(pre_idx);
                    int                      n        = new_node->GetChildCount();
                    if (n > 0 && new_node->get_open()) {
                        ObjectDataViewModelNode* last_child = new_node->GetNthChild(n - 1);
                        while (last_child->GetChildCount() > 0 && last_child->get_open()) {
                            ObjectDataViewModelNode* new_last_node = last_child->GetNthChild(last_child->GetChildCount() - 1);
                            last_child                             = new_last_node;
                        }
                        select_node_func(last_child);

                    } else {
                        select_node_func(new_node);
                    }
                } else {
                    select_node_func(parent_node);
                }
            } else {
                if (the_node->GetType() & ItemType::itPlate) {
                    // pre plate
                    wxDataViewItemArray all_plates;
                    m_objects_model->GetChildren(wxDataViewItem(nullptr), all_plates);

                    ObjectDataViewModelNode* pre_plate = nullptr;
                    int                      pre_idx   = the_node->GetPlateIdx() - 1;
                    if (pre_idx >= 0 && pre_idx < all_plates.size()) {
                        wxDataViewItem& p = all_plates[pre_idx];
                        pre_plate         = static_cast<ObjectDataViewModelNode*>(p.GetID());
                        if (pre_plate->GetChildCount() == 0 || !pre_plate->get_open()) {
                            select_plate_by_key(the_node);
                        } else {
                            // last item
                            ObjectDataViewModelNode* last_node = pre_plate->GetNthChild(pre_plate->GetChildCount() - 1);
                            while (last_node->GetChildCount() > 0 && last_node->get_open()) {
                                ObjectDataViewModelNode* new_last_node = last_node->GetNthChild(last_node->GetChildCount() - 1);
                                last_node                              = new_last_node;
                            }
                            select_node_func(last_node);
                        }
                    }
                }
            }
        }
    }
}

void ObjectList::ensure_current_item_visible_imgui()
{
    // if (wxGetKeyState(WXK_DOWN) || wxGetKeyState(WXK_UP)) {
    if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_UpArrow)) || ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_DownArrow))) {
    } else {
        return;
    }

    if (!m_table_data.valid())
        return;

    const float table_start_y = m_table_data.table_offset_y;
    const float table_height  = m_table_data.table_height;
    const float table_endy    = table_start_y + table_height;
    {
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();

        float scroll_y = ImGui::GetScrollY();
        float maxy     = ImGui::GetScrollMaxY();

        float table_content_height = maxy + table_height;
        float item_content_off     = min.y + scroll_y - table_start_y;

        if (min.y < table_start_y) {
            ImGui::SetScrollY(table_start_y + item_content_off - table_start_y);

        } else if (max.y > table_endy) {
            const float k = table_start_y + table_height - (max.y - min.y);
            ImGui::SetScrollY(table_start_y + item_content_off - k);
        }
    }
}

// updating model texture may fail�� call GLTexture.vaild() check it
void ObjectList::update_printer_model_texture()
{ 
    PresetBundle& preset_bundle = *wxGetApp().preset_bundle;
    auto          preset        = preset_bundle.printers.get_edited_preset();
    auto          printer_model = preset.config.opt_string("printer_model");

    if (printer_model.empty())
        return;

    // update printer texture when first init or printer model changed
    auto& printer_texture = m_png_textures->get(ObjList_Png_Texture_Wrapper::pngTexPrinterModel);
    if (printer_model != m_last_printer_model)
    {
        m_last_printer_model = printer_model;
        printer_texture->reset();

        // update texture
        const auto& model2CoverMap = wxGetApp().app_config->get_model2cover_path();
        auto        key            = wxGetApp().app_config->make_model2cover_path_key("", printer_model);
        std::string coverPath; 
        if (model2CoverMap.find(key) != model2CoverMap.end())
        {
            coverPath = model2CoverMap.at(key);
        }
        if (coverPath.empty())
        {
            coverPath = Slic3r::resources_dir() + "/images/printer_default.png";
        }
        printer_texture->load_from_png_file(coverPath, true, GLTexture::None, false);
    }
}

void ObjectList::update_printer_device_list_data(std::string vendor, bool bForce)
{
    if (bForce == false && m_device_list_dirty_mark == false)
        return;

    std::srand(std::time(nullptr));
    m_device_list_dirty_mark = false;
    auto devicesData = DM::DataCenter::Ins().GetData();
    auto printerData         = devicesData["data"];
    const auto& model2CoverMap = wxGetApp().app_config->get_model2cover_path();
    PresetBundle& preset_bundle       = *wxGetApp().preset_bundle;
    auto          preset              = preset_bundle.printers.get_edited_preset();
    auto          current_printer_model       = preset.config.opt_string("printer_model");

    m_device_list_data.clear();
    for (const auto& group : printerData["printerList"]) 
    {
        if (!group.contains("list")) 
        {
            continue;
        }
        for (const auto& printer : group["list"]) 
        {
            DM::Device device = DM::Device::deserialize(const_cast<nlohmann::json&>(printer));
            // model img
            auto printer_model = vendor + " " + device.modelName;

            if (printer_model != current_printer_model &&
                device.modelName != current_printer_model)
            {
                if ((device.oldPrinter) && (device.name == "Morandi") && (current_printer_model == "Creality Sermoon D3 Pro"))
                {
                    //device.modelName = Creality Sermoon D3 Pro;
                }
                else
                {
                    continue;
                }
            }
            auto key  = wxGetApp().app_config->make_model2cover_path_key("", current_printer_model);
            auto iter = model2CoverMap.find(key);
            auto coverPath = Slic3r::resources_dir() + "/images/printer_default.png";
            if (iter != model2CoverMap.end())
            {
                coverPath = iter->second;
            }
            bool              is_current     = false;
            const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
            if (current_device.valid)
            {
                is_current = current_device.mac == device.mac;
            }
            auto key_name = device.name.empty() ? (device.modelName + device.mac + device.address) : device.name;
            if (device.deviceType == 1)
            {
                key_name += "_##CXYDevice##_" + std::to_string(std::rand());
            }
            m_device_list_data.push(key_name, 
                {device.name, coverPath, device.modelName, 
                device.address, device.mac, device.deviceState, 
                device.online, is_current, device.deviceType});
        }
    }
}

void ObjectList::draw_device_list_popup()
{
    float  scale     = wxGetApp().plater()->get_current_canvas3D()->get_scale();
    bool   is_dark   = wxGetApp().dark_mode();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});
    auto originY = ImGui::GetCursorPosY();
    // Title
    ImGui::Dummy({0, 11 * scale});
    ImGui::SameLine(32 * scale);
    ImGui::AlignTextToFramePadding();
    ImGuiWrapper& imgui = *wxGetApp().imgui();
    imgui.title(_u8L("Device Lists"), true);
    
    if (m_device_list_data.datas.empty())
    {
        std::string label1 = _u8L("There is no device of this type available at present.");
        std::string label11 = _u8L("Please go to the device page to");
        std::string label2      = _u8L(" add printer");
        auto label_size1 = ImGui::CalcTextSize(label1.c_str());
        auto label_size11 = ImGui::CalcTextSize(label11.c_str());
        auto label_size2 = ImGui::CalcTextSize(label2.c_str());
        auto winWidth = ImGui::GetCurrentWindow()->Size.x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + winWidth / 2 - label_size1.x / 2);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 40 * scale / 2 - ImGui::GetFontSize() / 2);
        ImGui::Text("%s", label1.c_str());
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + winWidth / 2 - (label_size11.x + label_size2.x) / 2);
        ImGui::Text("%s", label11.c_str());
        ImGui::SameLine();
        
        ImVec4 label_color = {0.0824, 0.7529, 0.3490, 1.0000};
        ImGui::PushStyleColor(ImGuiCol_Text, label_color);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, 0});
        if (ImGui::Button(label2.c_str()))
        {
            // jump to device page
            wxGetApp().mainframe->select_tab((int) MainFrame::tpDeviceMgr);
        }
        ImGui::PopStyleColor(1);
        ImGui::PopStyleVar(1);
        ImGui::SameLine();

        ImGuiWindow* window = ImGui::GetCurrentWindow(); 
        ImVec2       startPos{window->DC.CursorPos.x - label_size2.x, window->DC.CursorPos.y + label_size2.y};
        ImVec2       endPos{window->DC.CursorPos.x, window->DC.CursorPos.y + label_size2.y};
        ImU32        c      = IM_COL32(label_color.x * 255, label_color.y * 255, label_color.z * 255, label_color.w * 255);
        window->DrawList->AddLine(startPos, endPos, c, 1.0);
    }
    else
    {
        draw_device_list_content();
    }
    ImGui::PopStyleVar(1);
}

void ObjectList::draw_device_list_content()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, 0});

    float scale   = wxGetApp().plater()->get_current_canvas3D()->get_scale();
    bool  is_dark = wxGetApp().dark_mode();
    // radio texture
    auto& radio_sel_texture   = m_png_textures->get(ObjList_Png_Texture_Wrapper::pngTexRadioSel);
    auto& radio_unsel_texture = m_png_textures->get(ObjList_Png_Texture_Wrapper::pngTexRadioUnSel);
    if (!radio_sel_texture->vaild())
        radio_sel_texture->load_from_png_file(Slic3r::resources_dir() + "/images/radio_sel.png", true, GLTexture::None, false);
    if (!radio_unsel_texture->vaild())
        radio_unsel_texture->load_from_png_file(Slic3r::resources_dir() + "/images/radio_unsel.png", true, GLTexture::None, false);

    auto rowHeight               = 64 * scale;
    auto max_width               = 152.0f * scale;
    auto remake_text_to_fit_size = [max_width](const std::string& input_text) {
        const float device_label_max_width   = max_width;
        const float origin_device_name_width = ImGui::CalcTextSize(input_text.c_str()).x;

        if (origin_device_name_width <= device_label_max_width) {
            return input_text;
        } else {
            const char* ellipsis       = "...";
            const float ellipsis_width = ImGui::CalcTextSize(ellipsis).x;
            size_t      name_length    = input_text.length();

            for (size_t i = name_length - 1; i >= 0; i--) {
                std::string sub_name           = input_text.substr(0, i);
                std::string name_with_ellipsis = sub_name + ellipsis;
                const float sub_width          = ImGui::CalcTextSize((name_with_ellipsis).c_str()).x;
                if (sub_width <= device_label_max_width) {
                    return name_with_ellipsis;
                }
            }
        }
        return input_text;
    };

    // device not connect warning
    const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
    if (!(current_device.valid && !current_device.address.empty())) {
        auto originCursorY = ImGui::GetCursorPosY();
        auto label         = _u8L("Device List Not Connect Warning").c_str();

        // ver center
        ImVec2 lab_size = ImGui::CalcTextSize(label);
        float  wrap_width    = (336 + 15 - 2.0f) * scale;
        float  line_height   = ImGui::GetTextLineHeight();                         
        int    line_count    = 1 + (int) (lab_size.x / wrap_width);
        float  text_height   = line_count * line_height;
        ImGui::SetCursorPosY(originCursorY + rowHeight / 2 - text_height / 2);

        auto text_color = is_dark
                            ? ImVec4{1.0f, 0.5294f, 0.0392f, 1.0f} 
                            : ImVec4{1.0f, 0.4902f, 0.0f, 1.0f};
        ImGui::PushStyleColor(ImGuiCol_Text, text_color);
        ImGui::TextWrapped(label);
        ImGui::PopStyleColor(1);
        ImGui::SetCursorPosY(originCursorY + rowHeight);
        ImGui::Separator();
    }

    //******************Device Info, Scroll Area*********************
    ImGui::BeginChild("##ScrollableContent");
    ImGui::SetItemDefaultFocus();
    
    float colDesigneWidth[] = {56.0f * scale, 56.0f * scale, 152.0f * scale, 72.0f * scale};
    int   col               = 0;
    float curWidth          = 0;
    int   count             = 0;
    
    std::set<std::string>* device_list[2] = {&m_device_list_data.online_device_list, &m_device_list_data.offline_device_list};
    for (auto& current_list : device_list)
    {
        for (auto key : *current_list) {
            auto it            = *(m_device_list_data.datas.find(key));
            auto originCursorY = ImGui::GetCursorPosY();
            curWidth           = 0;
            count++;

            if (it.second.visible == false)
                continue;

            if (ImGui::InvisibleButton("##invisibleBtn" + count, { ImVec2{(336 + 15) * scale, rowHeight} }))
            {
                // set current device
                nlohmann::json commandJson;
                nlohmann::json dataJson;
                dataJson["device_id"]  = it.second.mac;
                commandJson["command"] = "set_current_device";
                commandJson["data"]    = dataJson;
                auto jsonStr           = RemotePrint::Utils::url_encode(commandJson.dump(-1, ' ', true));
                wxGetApp().mainframe->get_printer_mgr_view()->ExecuteScriptCommand(jsonStr);

                auto& printer_collection = wxGetApp().preset_bundle->printers;
                if (printer_collection.get_edited_preset().is_system) {
                    // use local cache
                    auto& cache         = EasyCache::get_instance().data();
                    auto  preset_name   = printer_collection.get_edited_preset().name;
                    auto  printer_model = printer_collection.get_edited_preset().config.opt_string("printer_model");
                    auto  nozzle_dia    = printer_collection.get_edited_preset().config.opt_serialize("nozzle_diameter");
                    if (nozzle_dia.empty()) {
                        cache["system_preset_bundle_deivce"][printer_model]["unique"] = it.second.mac;
                    } else {
                        cache["system_preset_bundle_deivce"][printer_model][nozzle_dia] = it.second.mac;
                    }
                } else {
                    auto& seled_config = printer_collection.get_edited_preset().config;
                    seled_config.set_key_value("printer_select_mac", new ConfigOptionString(it.second.mac));
                    auto preset_name = printer_collection.get_edited_preset().name;
                    printer_collection.save_current_preset(preset_name, false, true);
                    auto new_preset       = printer_collection.find_preset(preset_name, false, true);
                    new_preset->sync_info = "update";
                    new_preset->save_info();
                }
            }

            col            = 0;
            auto radioSize = ImVec2{(float) 20, (float) 20} * scale;
            ImGui::SameLine(colDesigneWidth[col] / 2 - radioSize.x / 2);
            ImGui::SetCursorPosY(originCursorY + rowHeight / 2 - radioSize.y / 2);

            std::string uid = "##selBtn" + count;
            ImGui::PushID(uid.c_str());
            if (it.second.isCurrent) {
                ImGui::ImageButton((ImTextureID) radio_sel_texture->get_id(), radioSize);
            } else {
                if (ImGui::ImageButton((ImTextureID) radio_unsel_texture->get_id(), radioSize)) {
                }
            }
            ImGui::PopID();
            curWidth += colDesigneWidth[col];

            ++col;
            ImVec2 imageSize = {(56 - 4) * scale, (64 - 4) * scale};
            ImGui::SameLine(curWidth + colDesigneWidth[col] / 2 - imageSize.x / 2);
            ImGui::SetCursorPosY(originCursorY + rowHeight / 2 - imageSize.y / 2);
            auto texId = m_device_list_data.get_texture_id(it.second.cover_path);
            if (texId != GLTexture::INVAILD_ID)
                ImGui::Image((ImTextureID) texId, imageSize);
            else
                ImGui::Dummy(imageSize);
            curWidth += colDesigneWidth[col];

            ++col;
            auto device_name = remake_text_to_fit_size(it.second.name);
            auto showText    = device_name.c_str();
            auto showText2 = it.second.address.c_str();
            auto textSize  = ImGui::CalcTextSize(showText);
            auto textSize2  = ImGui::CalcTextSize(showText2);
            ImGui::SameLine(curWidth + colDesigneWidth[col] / 2 - textSize.x / 2);
            ImGui::SetCursorPosY(originCursorY + rowHeight / 2 - textSize.y);
            ImGui::TextColored(is_dark ? ImVec4{1.0f, 1.0f, 1.0f, 1.0f} : ImVec4{0.1686f, 0.1686f, 0.1765f, 1.0f}, "%s", showText);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", it.second.name.c_str());
            }
            ImGui::SameLine(curWidth + colDesigneWidth[col] / 2 - textSize2.x / 2);
            ImGui::SetCursorPosY(originCursorY + rowHeight / 2);
            ImGui::TextColored(is_dark ? ImVec4{0.6824, 0.6824, 0.6980, 1.0f} : ImVec4{0.3059f, 0.3490f, 0.4118f, 1.0f}, "%s", showText2);
            curWidth += colDesigneWidth[col];

            ++col;
            // convert state
            std::string stateList[] = {_u8L("Online"), _u8L("Offline")};
            ImVec4      colorList[] = {ImVec4{0.2195, 0.4902, 1.0000, 1.0}, ImVec4{1.0000, 0.4902, 0.0000, 1.0000}};
            int         stateText;
            int         stateColor;
            if (it.second.online) {
                stateText  = 0;
                stateColor = 0;
            } else {
                stateText  = 1;
                stateColor = 1;
            }
            textSize = ImGui::CalcTextSize(stateList[stateText].c_str());
            ImGui::SameLine(curWidth + colDesigneWidth[col] / 2 - ImGui::CalcTextSize(stateList[stateText].c_str()).x / 2);
            ImGui::SetCursorPosY(originCursorY + rowHeight / 2 - textSize.y / 2);
            ImGui::TextColored(colorList[stateColor], "%s", stateList[stateText].c_str());

            ImGui::SetCursorPosY(originCursorY + rowHeight);
            ImGui::Separator();
        }
    }    
    ImGui::EndChild();
    ImGui::PopStyleVar(1);
}

bool ObjectList::set_cur_device_by_cur_preset()
{
    std::string selected_mac;
    auto        cur_preset        = wxGetApp().preset_bundle->printers.get_selected_preset();
    auto        cur_preset_config = cur_preset.config;
    if (wxGetApp().preset_bundle->printers.get_selected_preset().is_system) {
        auto cache         = EasyCache::get_instance().data();
        auto printer_model = cur_preset_config.opt_string("printer_model");
        if (cache.contains("system_preset_bundle_deivce") && cache["system_preset_bundle_deivce"].contains(printer_model)) {
            std::string json_key   = "unique";
            auto        nozzle_dia = cur_preset_config.opt_serialize("nozzle_diameter");
            if (!nozzle_dia.empty())
                json_key = nozzle_dia;

            if (cache["system_preset_bundle_deivce"][printer_model].contains(json_key))
                selected_mac = cache["system_preset_bundle_deivce"][printer_model][json_key];
        }
    } else {
        if (cur_preset_config.has("printer_select_mac"))
            selected_mac = cur_preset_config.opt_string("printer_select_mac");
    }

    nlohmann::json commandJson;
    nlohmann::json dataJson;
    dataJson["device_id"]  = selected_mac;
    commandJson["command"] = "set_current_device";
    commandJson["data"]    = dataJson;
    auto jsonStr           = RemotePrint::Utils::url_encode(commandJson.dump(-1, ' ', true));
    wxGetApp().mainframe->get_printer_mgr_view()->ExecuteScriptCommand(jsonStr);

    return !selected_mac.empty();
}

ObjectList::ObjList_Png_Texture_Wrapper::ObjList_Png_Texture_Wrapper()
{
    for (auto i = 0; i < ObjList_Png_Texture_Wrapper::pngTexCount; ++i) {
        textures.push_back(std::move(std::unique_ptr<GLTexture>{new GLTexture}));
    }
}

ObjectList::ObjList_Png_Texture_Wrapper::~ObjList_Png_Texture_Wrapper() 
{ 
    textures.clear(); 
}

void ObjectList::device_list_data::push(std::string name, device_list_item_data item)
{ 
    if (cover2textureId.find(item.cover_path) == cover2textureId.end())
    {
        auto& texture = objPtr->m_png_textures->get(ObjList_Png_Texture_Wrapper::pngTexDeviceListIItem);
        texture->load_from_png_file(item.cover_path, true, GLTexture::None, false);
        cover2textureId.insert({item.cover_path, texture->get_id()});
        texture->reset(true); // delay until ~device_list_data() releases the id
    }
    datas.insert({name, item}); 
    if (item.online)
        online_device_list.insert(name);
    else
        offline_device_list.insert(name);

    if (mac_2_key_map.find(item.mac) == mac_2_key_map.end())
    {
        mac_2_key_map[item.mac] = item.device_type == 1 ? std::pair{name, std::vector<std::string>{}} :
                                                          std::pair{"", std::vector<std::string>{name}};
    }
    else
    {
        if (item.device_type == 1)
            mac_2_key_map[item.mac].first = name;
        else
            mac_2_key_map[item.mac].second.push_back(name);
        manager_duplicate_deivce(mac_2_key_map[item.mac]);
    }
}

// manager duplicate_deivce, such as WAN��LAN device both exists
void ObjectList::device_list_data::manager_duplicate_deivce(std::pair<std::string, std::vector<std::string>>& cloud_local_pair)
{
    // duplicate removal
    std::string online_local_device_key;
    std::string cloud_key = cloud_local_pair.first;
    for (auto local_key : cloud_local_pair.second) {
        if (datas[local_key].online) {
            online_local_device_key = local_key;
        } else {
            datas[local_key].visible = false;
        }
    }
    if (online_local_device_key.empty())
    {
        auto local_key = cloud_local_pair.second[0];
        datas[local_key].visible = true;
        if (!cloud_key.empty())
        {
            datas[cloud_key].visible = true;
            datas[local_key].visible = false;
        }
    }
    else
    {
        datas[online_local_device_key].visible = true;
        if (!cloud_key.empty())
            datas[cloud_key].visible = false;
    }
}

ObjectList::device_list_data::~device_list_data()
{ 
    for (auto it : cover2textureId)
    {
        if (it.second != GLTexture::INVAILD_ID)
            glsafe(::glDeleteTextures(1, &it.second));
    }
}

} //namespace GUI
} //namespace Slic3r
