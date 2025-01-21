#pragma once

#include "wx/wxprec.h"
#include "wx/aui/auibar.h"

#include "SelectMachine.hpp"
#include "DeviceManager.hpp"


using namespace Slic3r::GUI;

class BBLTopbar : public wxAuiToolBar
{
public:
    BBLTopbar(wxWindow* pwin, wxFrame* parent);
    BBLTopbar(wxFrame* parent);
    void Init(wxFrame *parent);
    ~BBLTopbar();
    void UpdateToolbarWidth(int width);
    void Rescale(bool isResize);
    void OnIconize(wxAuiToolBarEvent& event);
    void OnUpload3mf(wxAuiToolBarEvent& event);
    void OnFullScreen(wxAuiToolBarEvent& event);
    void OnCloseFrame(wxAuiToolBarEvent& event);
    void OnFileToolItem(wxAuiToolBarEvent& evt);
    void OnDropdownToolItem(wxAuiToolBarEvent& evt);
    void OnCalibToolItem(wxAuiToolBarEvent &evt);
    void OnMouseLeftDClock(wxMouseEvent& mouse);
    void OnMouseLeftDown(wxMouseEvent& event);
    void OnMouseLeftUp(wxMouseEvent& event);
    void OnMouseMotion(wxMouseEvent& event);
    void OnMouseCaptureLost(wxMouseCaptureLostEvent& event);
    void OnMenuClose(wxMenuEvent& event);
    void OnOpenProject(wxAuiToolBarEvent& event);
    void show_publish_button(bool show);
    void OnSaveProject(wxAuiToolBarEvent& event);
    void OnUndo(wxAuiToolBarEvent& event);
    void OnRedo(wxAuiToolBarEvent& event);
    void OnModelStoreClicked(wxAuiToolBarEvent& event);
    void OnPublishClicked(wxAuiToolBarEvent &event);
    void OnPreferences(wxAuiToolBarEvent& evt);
    void OnConfigRelate(wxAuiToolBarEvent& evt);
    void OnLogo(wxAuiToolBarEvent& evt);
    void OnDownMgr(wxAuiToolBarEvent& evt);
    void OnLogin(wxAuiToolBarEvent& evt);
    wxAuiToolBarItem* FindToolByCurrentPosition();
	
    void SetFileMenu(wxMenu* file_menu);
    void AddDropDownSubMenu(wxMenu* sub_menu, const wxString& title);
    void AddDropDownMenuItem(wxMenuItem* menu_item);
    wxMenu *GetTopMenu();
    wxMenu *GetCalibMenu();
    void SetTitle(wxString title);
    void SetMaximizedSize();
    void SetWindowSize();

    void EnableUndoRedoItems();
    void DisableUndoRedoItems();

    void SaveNormalRect();

    void EnableUpload3mf();

    void ShowCalibrationButton(bool show = true);
    void SetSelection(size_t index);

    void update_mode(int mode);

    bool GetSaveProjectItemEnabled();
private:
    wxFrame* m_frame;
    wxAuiToolBarItem* m_file_menu_item;
    wxAuiToolBarItem* m_dropdown_menu_item;
    wxRect m_normalRect;
    wxPoint m_delta;
    wxMenu m_top_menu;
    wxMenu* m_file_menu;
    wxMenu m_calib_menu;
    wxMenuItem*       m_relationsItem{NULL};   
    wxAuiToolBarItem* m_relationsItem1{NULL};   

    wxAuiToolBarItem* m_title_item;
    wxAuiToolBarItem* m_account_item;
    wxAuiToolBarItem* m_model_store_item;
    
    wxAuiToolBarItem *m_publish_item;
    wxAuiToolBarItem* m_undo_item;
    wxAuiToolBarItem* m_redo_item;
    wxAuiToolBarItem* m_calib_item;
    wxAuiToolBarItem* maximize_btn;
    wxAuiToolBarItem* m_save_project_item;
    wxAuiToolBarItem* m_upload_btn;
    wxControl* m_tabCtrol;

    wxBitmap m_publish_bitmap;
    wxBitmap m_publish_disable_bitmap;

    wxBitmap maximize_bitmap;
    wxBitmap window_bitmap;

    wxBitmap m_model_store_bitmap;
    wxBitmap m_model_store_hover_bitmap;

    int m_toolbar_h;
    bool m_skip_popup_file_menu;
    bool m_skip_popup_dropdown_menu;
    bool m_skip_popup_calib_menu;
};
