#include "ConfigRelateDialog.hpp"
#include "OptionsGroup.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"
#include "Plater.hpp"
#include "MsgDialog.hpp"
#include "I18N.hpp"
#include "libslic3r/AppConfig.hpp"
#include <string>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/language.h>
#include <wx/colour.h>
#include <wx/window.h>
#include <wx/notebook.h>
#include "Notebook.hpp"
#include "OG_CustomCtrl.hpp"
#include "wx/graphics.h"
#include "Widgets/CheckBox.hpp"
#include "Widgets/ComboBox.hpp"
#include "Widgets/RadioBox.hpp"
#include "Widgets/TextInput.hpp"
#include "Widgets/TextInputCtrl.hpp"
#include "Widgets/TextDisplay.hpp"
#include "Tab.hpp"
#include <wx/listimpl.cpp>
#include <map>
#include <wx/string.h>

#include <wx/notebook.h>
#include <wx/listctrl.h>
#include <wx/treectrl.h>
#include <wx/button.h>
#include <wx/listbook.h>
#include <wx/choicebk.h>
#include <wx/simplebook.h>
#include <wx/toolbook.h>
#include <wx/treebook.h>
#include <wx/aui/auibook.h>

#include <iostream>
#include <string>
#include <locale>
#include <codecvt>

#ifdef __WINDOWS__
#ifdef _MSW_DARK_MODE
#include "dark_mode.hpp"
#endif // _MSW_DARK_MODE
#endif //__WINDOWS__


namespace ConfigRelateGUI {
    static wxColour green = wxColour("#17CC5F");
    static wxColour white = wxColour("#FFFFFF");
    static wxSize   windowSize = wxSize(1112, 734);
    static wxSize   contentSize = wxSize(1112, 706);
    static wxSize   treeSize    = wxSize(250, 600);
    static wxSize   rightTabSize = wxSize(800, 600);
    static wxSize   rightTabContentSize     = wxSize(800, 577);
    static wxSize   listSize     = wxSize(800 + 4, 511 + 4 - 50);

    static void setFont(wxWindow* win, int size);

    class _TreeView : public wxDataViewTreeCtrl
    {
    public:
        _TreeView(wxWindow* parent)
            : wxDataViewTreeCtrl(parent, wxID_ANY, wxDefaultPosition, treeSize, wxDV_NO_HEADER | wxBORDER_SIMPLE)
        {
            Bind(wxEVT_DATAVIEW_ITEM_EDITING_STARTED, [=](wxDataViewEvent& e) {
                wxDataViewColumn* c2 = GetCurrentColumn();
                if (c2) {
                    wxDataViewRenderer* renderer = c2->GetRenderer();
                    renderer->SetMode(wxDATAVIEW_CELL_INERT);
                    renderer->CancelEditing();
                }
            });
        }

        void sync()
        {
            wxDataViewColumn* c2 = GetCurrentColumn();
            if (c2) {
                wxDataViewRenderer* renderer = c2->GetRenderer();
                renderer->SetMode(wxDATAVIEW_CELL_INERT);
                renderer->CancelEditing();
            }
        }
    };

    class _CheckBox : public wxCheckBox
    {
    public:
        _CheckBox(wxWindow* parent, const std::string& text) : wxCheckBox(parent, wxID_ANY, text) 
        {
            SetSize(wxSize(400, 20));
            setFont(this, 12);
        }
    };

    class _Button : public Button
    {
    public:
        _Button(wxWindow* parent, wxString text, wxSize size = wxSize(120,24)) : Button(parent, text)
        {
            SetBackgroundColour(wxColour(*wxWHITE));
            SetMinSize(size);
            SetMaxSize(size);
            SetFont(::Label::Body_12);
        
            bool is_dark = wxGetApp().app_config->get("dark_color_mode") == "1";
            if (is_dark) 
            {
                m_defaultColor = StateColor(std::pair<wxColour, int>(green, StateColor::Pressed),
                                            std::pair<wxColour, int>(green, StateColor::Hovered),
                                            std::pair<wxColour, int>(wxColour("#6E6E73"), StateColor::Normal),
                                            std::pair<wxColour, int>(green, StateColor::Checked));
                SetBackgroundColor(m_defaultColor);
                SetForegroundColour(white);
                SetBorderWidth(0);
                //SetValue
            } else {
                m_defaultColor = StateColor(std::pair<wxColour, int>(green, StateColor::Pressed),
                                            std::pair<wxColour, int>(green, StateColor::Hovered),
                                   std::pair<wxColour, int>(*wxWHITE, StateColor::Normal));
                SetBackgroundColor(m_defaultColor);
                SetForegroundColour(wxColour("#A0A0A0"));
                SetBorderWidth(1);
            }

            m_checkedColor = StateColor(std::pair<wxColour, int>(green, StateColor::Pressed),
                                        std::pair<wxColour, int>(green, StateColor::Hovered),
                                        std::pair<wxColour, int>(green, StateColor::Normal));

        }

        void setChecked(bool checked)
        { 
            SetBackgroundColor(checked ? m_checkedColor : m_defaultColor);

        }

    private:
        StateColor m_defaultColor;
        StateColor m_checkedColor;
    };
    
    using SelectedChangedFunc = std::function<void(int)>; 
    class _Tab : public wxWindow
    {
    public:
        _Tab(wxWindow* parent, wxSize size = wxSize(100, 26)) : wxWindow(parent, wxID_ANY)
        { 
            m_tabSize = size;
            SetBackgroundColour(wxColour(*wxWHITE));

            m_sizer = new wxBoxSizer(wxHORIZONTAL);
        }

        _Button* appendTab(wxString text) { 
            _Button* tabButton = new _Button(this, text, m_tabSize);
            tabButton->SetFont(::Label::Body_14);
            tabButton->SetCornerRadius(0);
            m_sizer->Add(tabButton, 0, wxEXPAND);
            SetSizerAndFit(m_sizer);

            m_tabButtons.push_back(tabButton);
            tabButton->Bind(wxEVT_LEFT_DOWN, [=](wxMouseEvent& e) { onSelectedChanged(tabButton); });

            //SetSize(wxSize(m_tabSize.GetWidth() * m_tabButtons.size(), m_tabSize.GetHeight()));

            return tabButton;
        }

        void bind(SelectedChangedFunc func)
        { 
            m_func = func;
        }

        void select(int index)
        {
            if (index < 0 || index >= m_tabButtons.size())
                return;

            for (int i = 0, count = m_tabButtons.size(); i < count; ++i) {
                if (i == index) {
                    m_tabButtons[i]->setChecked(true);
                    m_currentIndex = i;
                    if (m_func)
                        m_func(m_currentIndex);
                } else {
                    m_tabButtons[i]->setChecked(false);
                }
            }
        }

    private:
        void onSelectedChanged(_Button* tabButton) {
            m_currentIndex = -1;
            int selected = -1;
            for (int i = 0, count = m_tabButtons.size(); i < count; ++i)
            {
                if (m_tabButtons[i] != tabButton) {
                    m_tabButtons[i]->setChecked(false);
                } else {
                    m_currentIndex = i;
                    selected       = i;
                }
            }
            if (m_currentIndex >= 0)
            {
                m_tabButtons[m_currentIndex]->setChecked(true);
                if (m_func)
                    m_func(m_currentIndex);
            }

            Refresh();
            Update();
        }

    private:
        SelectedChangedFunc m_func{ NULL };

        std::vector<_Button*> m_tabButtons;
        wxSizer*              m_sizer;

        int m_currentIndex{0};
        wxSize m_tabSize;
    };

    class _ListItem : public wxListItem
    {
    public:
        _ListItem() : wxListItem() {

        }

    public:
        bool isSystem;
        bool isSingle;

    };

    class _ListView : public wxListCtrl
    {
    public:
        _ListView(wxWindow*      parent) : wxListCtrl(parent, wxID_ANY, wxDefaultPosition, listSize, wxLC_REPORT | wxLC_NO_HEADER)
        {
            //SetWindowStyleFlag(GetWindowStyleFlag() & ~(wxLC_HRULES | wxLC_VRULES));
            // 绑定重绘事件处理函数
            Bind(wxEVT_PAINT, &_ListView::OnPaint, this);
            Bind(wxEVT_MOTION, &_ListView::OnMouseMove, this);

            Bind(wxEVT_LEAVE_WINDOW, &_ListView::OnMouseLeave, this);
            
        }

    private:
        int m_hoverRow = -1;
        wxColour m_hoverBgColour    = green; 
        bool is_dark = Slic3r::GUI::wxGetApp().dark_mode();
        wxColour m_normalBGColour = is_dark ? wxColour("#4E4E52") : wxColour("#FFFFFF");//wxColour("#4E4E52");
        wxColour m_singleBGColour   = is_dark ?  wxColour("#474749") : wxColour("#FFFFFF");
        wxColour m_userTextColour = green;
        wxColour m_systemTextColour = is_dark ? white : wxColour("#000000");

        void OnMouseLeave(wxMouseEvent& event) { 
            m_hoverRow = -1;
        }

        void OnPaint(wxPaintEvent& event)
        {
            wxPaintDC dc(this);
            wxRect    clientRect = GetClientRect();
           
             //先绘制整个列表的常规背景（比如默认白色等）
            dc.SetBrush(wxBrush(m_normalBGColour));
            dc.SetPen(wxPen(m_normalBGColour));
            dc.DrawRectangle(clientRect);

            bool firstLine = true;
            // 绘制列表项
            for (int row = 0; row < m_nameItems.size(); row++) {
                _ListItem nameItem = m_nameItems[row];
                _ListItem typeItem = m_typeItems[row];
                //GetItem(item);
                wxRect itemRect;
                GetItemRect(row, itemRect);
                itemRect.SetWidth(GetSize().GetWidth());
                bool isHovered = row == m_hoverRow;
                if (isHovered) {
                    // 如果是悬停行，设置悬停背景颜色绘制
                    dc.SetBrush(wxBrush(m_hoverBgColour));
                    dc.SetPen(wxPen(m_hoverBgColour));
                    dc.DrawRectangle(itemRect);
                } else if (!nameItem.isSingle) {
                    dc.SetBrush(wxBrush(m_singleBGColour));
                    dc.SetPen(wxPen(m_singleBGColour));
                    dc.DrawRectangle(itemRect);
                }

                for (int col = 0; col < 2; col++) {
                    _ListItem item = col ? typeItem : nameItem;
                    if (item.isSystem)
                        dc.SetTextForeground(m_systemTextColour);
                    else
                        dc.SetTextForeground(m_userTextColour);

                    if (isHovered)
                        dc.SetTextForeground(white);

                    if (col == 0)
                        dc.DrawText(GetItemText(row, col), 40, itemRect.GetTop());
                    else
                        dc.DrawText(item.isSystem ? _L("System Preset") : _L("User Preset"), 576, itemRect.GetTop());
                }

                dc.SetPen(wxPen(wxColour("#6E6E72")));
                if (firstLine)
                {
                    firstLine = false;
                    dc.DrawLine(itemRect.GetLeftTop(), itemRect.GetRightTop());
                }
                dc.DrawLine(itemRect.GetLeftBottom(), itemRect.GetRightBottom());
            }

            dc.SetPen(wxPen(wxColour("#6E6E72")));

            dc.DrawLine(clientRect.GetLeftBottom(), clientRect.GetLeftTop());
            dc.DrawLine(clientRect.GetRightBottom(), clientRect.GetRightTop());
        }

        void OnMouseMove(wxMouseEvent& event)
        {
            int x, y;
            event.GetPosition(&x, &y);
            int  flag;
            long itemIndex = HitTest(wxPoint(x, y), flag);
            if (itemIndex != -1) {
                if (GetItemState(itemIndex, wxLIST_STATE_SELECTED) == 0) {
                    _ListItem item = m_nameItems[itemIndex];
                    m_hoverRow      = itemIndex;
                    //GetItem

                }
            } else {
                m_hoverRow = -1;
            }
            event.Skip();
        }

        virtual int GetColumnWidth(int col) const override { return 0; }

    public:
        void insertNameItem(_ListItem item) { 
            InsertItem(item);
            m_nameItems.insert(m_nameItems.begin(), item); 
        }

        void insertTypeItem(_ListItem item) {
            SetItem(item);
            m_typeItems.insert(m_typeItems.begin(), item); 
        }

        void init(const std::string& title1, const std::string& title2)
        {
            ClearAll();
            InsertColumn(0, title1);
            InsertColumn(1, title2);
            SetColumnWidth(0, 570);
            SetColumnWidth(1, GetSize().GetWidth() - 580);

            m_nameItems.clear();
            m_typeItems.clear();
        }

    private:
        std::vector<_ListItem>  m_nameItems;
        std::vector<_ListItem> m_typeItems;
    };

    class DrawBorderBox : public StaticBox
    {
    public:
        DrawBorderBox(wxWindow* parent,
                                    wxWindowID id    = wxID_ANY,
                                  const wxPoint&              pos   = wxDefaultPosition,
                                  const wxSize&               size  = wxDefaultSize,
                                  long                        style = 0)
            : StaticBox(parent, id, pos, size, style)
        { 
            SetBorderWidth(0);
            Bind(wxEVT_PAINT, &DrawBorderBox::OnPaint, this);
        }

    public:
        void OnPaint(wxPaintEvent& event)
        {
            wxPaintDC dc(this);
            StaticBox::doRender(dc);

            dc.SetPen(wxPen(wxColour("#6E6E72"), 1));

            wxRect rect = GetRect();
            dc.DrawLine(rect.GetLeftBottom(), rect.GetLeftTop());
            dc.DrawLine(rect.GetLeftTop(), rect.GetRightTop());
            dc.DrawLine(rect.GetRightTop(), rect.GetRightBottom());
            //if (topEnable)
            dc.DrawLine(rect.GetRightBottom(), rect.GetLeftBottom());
        }

        bool topEnable{true};

    };

    class _ListWidget : public DrawBorderBox
    {
    public:
        _ListWidget(wxWindow* parent) : DrawBorderBox(parent) 
        { 
            //SetBorderColor(wxColour("#68686B"));
            //SetBorderColor(wxColour("#FF0000"));
            //SetBorderWidth(3);
            bool is_dark = Slic3r::GUI::wxGetApp().dark_mode();
            SetBackgroundColor(is_dark ? wxColour("#474749") : wxColour("#ffffff"));
            //SetBackgroundColor(wxColour("#FF0000"));

            m_listView = new _ListView(this);
            m_listView->SetPosition(wxPoint(-2, 36));

            StaticBox* header = new StaticBox(this, wxID_ANY, wxPoint(1, 1), wxSize(798, 37));
            header->SetBackgroundColour(is_dark ? wxColour("#474749") : wxColour("#ffffff"));
            //header->SetBackgroundColor(wxColour("#6E6E72"));
            header->SetBorderWidth(0);
            header->SetCornerRadius(0);
            m_header1         = new wxStaticText(header, wxID_ANY, "", wxPoint(85, 12));
            m_header1->SetFont(::Label::Body_14);
            m_header2 = new wxStaticText(header, wxID_ANY, "", wxPoint(572, 12));
            m_header2->SetFont(::Label::Body_14);
            //header->Hide();
      
            
            DrawBorderBox* footer = new DrawBorderBox(this, wxID_ANY,
                                                  wxPoint(0, m_listView->GetPosition().y + m_listView->GetSize().GetHeight() - 2),
                                                  wxSize(800, 2));
            //footer->topEnable     = false;
            footer->SetBackgroundColor(wxColour("#6E6E72"));
        }

        void insertNameItem(_ListItem item) { 
            m_listView->insertNameItem(item);
        }

        void insertTypeItem(_ListItem item) { 
            m_listView->insertTypeItem(item);
        }

        void init(const std::string& title1, const std::string& title2) { 
            m_listView->init(title1, title2);
            m_header1->SetLabelText(title1);
            m_header2->SetLabelText(title2);
        }


    private:
        wxStaticText* m_header1;
        wxStaticText* m_header2;
        _ListView* m_listView;
    };

    static void setFont(wxWindow* win, int size)
    {
        static wxFont font = win->GetFont();
        font.SetPixelSize(wxSize(size, size));
        win->SetFont(font);
    }

    static void initListItem(_ListItem& item, wxString text, bool isSingle, bool isSystem, int col)
    {
        item.SetBackgroundColour(isSingle ? wxColour("#4E4E52") : wxColour("#474749"));
        item.m_mask = wxLIST_MASK_TEXT;
        item.SetTextColour(isSystem ? white : green);
        item.SetText(text);
        item.SetColumn(col);
        item.SetId(0);
        item.isSingle = isSingle;
        item.isSystem = isSystem;
    }

}

namespace Slic3r { namespace GUI {
    
    using SelectedChangedFunc = std::function<void(int)>; 
    using namespace ConfigRelateGUI;

class RelateBundle
{
public:
    struct Item
    {
        std::string name;
        bool        isSystem;
    };

    struct ModelCategory
    {
        std::string name;
        std::string              fullName;
        std::string        modelName;
        std::string        nozzleName;
        std::vector<Item> materialItems;
        std::vector<Item> techniqueItems;
        std::vector<std::string> childModels;
        std::vector<bool> childModelRoots;
    };

    struct MaterialPreset
    {
        bool                 isEnabled;
        std::string category;   // PLA BAS
        std::string       brand;
        std::string          name;
        bool              isSystem;
        std::vector<Item> relatedModels;
    };

    RelateBundle()
    {
        m_preset_bundle = wxGetApp().preset_bundle;
        m_filaments    = &m_preset_bundle->filaments;
        m_printers = &m_preset_bundle->printers;
        m_technologies  = &m_preset_bundle->prints;
        m_vendors       = &m_preset_bundle->vendors;
       
        reload();
    }

    bool isSameVendorPrinter(const std::string& printer1, const std::string& printer2) {
        if (printer1.empty() || printer2.empty())
            return false;
        Preset* preset1 = m_printers->find_preset(printer1);
        Preset* preset2 = m_printers->find_preset(printer2);
        if (preset1 == nullptr || preset2 == nullptr)
            return false;
        std::string vendor1 = getVendor(*preset1);
        std::string vendor2 = getVendor(*preset2);
        if (vendor1 != vendor2)
            return false;

        return true;
    }

    bool isSameVendorFilament(std::vector<std::string> printers, std::vector<std::string> filaments)
    {
        for (std::string printer : printers) {
            Preset* preset1 = m_printers->find_preset(printer);
            if (!preset1)
                continue;

            const std::string& vendor1 = getVendor(*preset1);
            for (std::string& filament : filaments) {
                Preset* preset2 = m_filaments->find_preset(filament);
                if (!preset2)
                    continue;

                const std::string& vendor2 = getVendor(*preset2);
                if (vendor1 != vendor2)
                    return false;
            }
        }
        return true;
    }

    bool isSameVendorProcess(std::vector<std::string> printers, std::vector<std::string> processes)
    {
        for (std::string printer : printers) {
            Preset* preset1 = m_printers->find_preset(printer);
            if (!preset1)
                continue;

            const std::string& vendor1 = getVendor(*preset1);
            for (std::string& process : processes) {
                Preset* preset2 = m_technologies->find_preset(process);
                if (!preset2)
                    continue;

                const std::string& vendor2 = getVendor(*preset2);
                if (vendor1 != vendor2)
                    return false;
            }
        }
        return true;
    }

    bool checkMaterialRelateValid(std::string targetPrinter)
    {
        auto& mcs = m_modelCategorys;
        for (auto pair : mcs) {
            const std::string&                 name = pair.first;
            const RelateBundle::ModelCategory& mc   = pair.second;

            if (std::find(mc.childModels.begin(), mc.childModels.end(), targetPrinter) != mc.childModels.end())
                continue;

            bool valid = false;
            for (auto& item : mc.materialItems) {
                if (!item.isSystem) {
                    valid = true;
                    break;
                }
            }
            if (!valid)
                continue;

            return true;
        }
        return false;
    }

    bool checkProcessRelateValid(std::string targetPrinter)
    {
        auto& mcs = m_modelCategorys;
        for (auto pair : mcs) {
            const std::string&                 name = pair.first;
            const RelateBundle::ModelCategory& mc   = pair.second;

            if (std::find(mc.childModels.begin(), mc.childModels.end(), targetPrinter) != mc.childModels.end())
                continue;

            bool valid = false;
            for (auto& item : mc.techniqueItems) {
                if (!item.isSystem) {
                    valid = true;
                    break;
                }
            }
            if (!valid)
                continue;

            return true;
        }
        return false;
    }

    const std::deque<Preset>& printerPresets()
    { 
        return m_printers->get_presets(); 
    }

    void mergeVector(std::vector<std::pair<bool, std::string>>& base, std::vector<std::pair<bool, std::string>>& add)
    { 
        for (auto& pair : add)
        {
            bool found = false;
            for (auto& bpair : base)
            {
                if (bpair.first == pair.first && bpair.second == pair.second)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
                base.push_back(pair);
        }
    }

    void getPrinterFamily(const std::string& printer, std::vector<std::pair<bool, std::string>>& family)
    { 
        Preset* ppreset = m_printers->find_preset(printer);
        if (ppreset == NULL)
            return;

        for (auto& pair : m_modelCategorys) {
            std::string name = pair.first;
            ModelCategory& mc   = pair.second;
            if (printer != mc.fullName) 
            {
                bool found = false;
                for (auto m : mc.childModels) {
                    if (m == printer) {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    continue;
            }

            for (int i = 0, count = mc.childModels.size(); i < count; ++i)
            {
                family.push_back(std::pair<bool, std::string>(mc.childModelRoots[i], mc.childModels[i]));
            }
            break;
        }

        std::string inherits = ppreset->inherits();
        if (inherits.empty() && ppreset->name != inherits)
            return;

        std::vector<std::pair<bool, std::string>> parentFamily;
        getPrinterFamily(inherits, parentFamily);
        mergeVector(family, parentFamily);
    }

    void getFilamentsRelatedPrinters(const std::string& filamentName, std::vector<std::pair<bool, std::string>>& printers)
    {
        printers.clear();
        Preset* preset = m_filaments->find_preset(filamentName);
        if (!preset)
            return;

        for (auto& pair : m_materials) {
            for (MaterialPreset& preset : pair.second) {
                std::string mName = preset.name;
                if (mName == filamentName) {
                    for (Item& item : preset.relatedModels) {
                        std::vector<std::pair<bool, std::string>> family;
                        getPrinterFamily(item.name, family);
                        mergeVector(printers, family);
                    }
                    break;
                }
            }
        }

        //std::string  inherit = preset->inherits();
        //if (!inherit.empty()) 
        //{
        //    std::vector<std::pair<bool, std::string>> parentRelatedPrinters;
        //    getFilamentsRelatedPrinters(inherit, parentRelatedPrinters);
        //    for (auto pair : parentRelatedPrinters) {
        //        printers.push_back(pair);
        //    }
        //}
    }

    void getPrinterRelatedFilaments1(const std::string& printerName, std::vector<std::pair<bool, std::string>>& relatedFliaments)
    {
        Preset* ppreset = m_printers->find_preset(printerName);
        if (!ppreset)
            return;

        std::string categroy;
        getCategory(m_filaments, ppreset, categroy);

        const auto& fpreset = m_filaments->get_presets();
        for (const Preset& preset : fpreset) 
        {
            if (preset.is_project_embedded)
                continue;

            auto compatibles = preset.compatibles();
            for (const std::string& name : compatibles) 
            {
                if (name == categroy)
                    relatedFliaments.push_back(std::pair<bool, std::string>(preset.is_system, preset.name));
            }
        }

        std::string inherit = ppreset->inherits();
        if (!inherit.empty() && ppreset->name != inherit) {
            std::vector<std::pair<bool, std::string>> parentRelatedFliaments;
            getPrinterRelatedFilaments1(inherit, parentRelatedFliaments);
            for (auto pair : parentRelatedFliaments) {
                relatedFliaments.push_back(pair);
            }
        }
    }
    

    void getPrinterRelatedFilaments(const std::string& printerName, std::vector<std::pair<bool, std::string>>& relatedFliaments)
    {
        relatedFliaments.clear();
        Preset* preset = m_printers->find_preset(printerName);
        if (!preset)
            return;

        //std::set<std::string> mNameCache;
        for (auto& pair : m_materials) {
            for (MaterialPreset& preset : pair.second) {
                std::string mName = preset.name;

                if (preset.relatedModels.empty()) {
                    bool contains = false;
                    for (auto& pair : relatedFliaments) {
                        if (pair.second == preset.name) {
                            contains = true;
                            break;
                        }
                    }
                    if (!contains)
                        relatedFliaments.push_back(std::pair(preset.isSystem, mName));
                } else {
                    for (Item& item : preset.relatedModels) {
                        if (item.name == printerName) {
                            relatedFliaments.push_back(std::pair(preset.isSystem, mName));
                        }
                    }
                }
            }
        }

        std::string  inherit = preset->inherits();
        if (!inherit.empty() && preset->name != inherit) 
        {
            //std::vector<std::pair<bool, std::string>> parentRelatedFliaments;
            getPrinterRelatedFilaments(inherit, relatedFliaments);
            //for (auto pair : parentRelatedFliaments) {
            //        relatedFliaments.push_back(pair);
            //}
        }
    }

    void getPrinterRelatedTechnologies(const std::string& printerName, std::vector<std::pair<bool, std::string>>& relatedTechnologies)
    {
        relatedTechnologies.clear();
        Preset* preset = m_printers->find_preset(printerName);
        if (!preset)
            return;

        const std::deque<Preset>& tpresets = m_technologies->get_presets();

        for (auto& preset : tpresets) {
            if (preset.is_project_embedded)
                continue;

            auto compatibles = preset.compatibles();
            if (compatibles.empty())
            {
                bool contains = false;
                for (auto& pair : relatedTechnologies)
                {
                    if (pair.second == preset.name)
                    {
                        contains = true;
                        break;
                    }
                }
                if (!contains)
                    relatedTechnologies.push_back(std::pair(preset.is_system, preset.name));
            }
            else 
            {
                for (const std::string& name : preset.compatibles())
                {
                    if (name == printerName)
                    {
                        relatedTechnologies.push_back(std::pair(preset.is_system, preset.name));
                    }
                }
            }
        }
        std::string inherit = preset->inherits();
        if (!inherit.empty() && preset->name != inherit) {
            //std::vector<std::pair<bool, std::string>> parentRelatedTechnologies;
            getPrinterRelatedTechnologies(inherit, relatedTechnologies);
            //for (auto pair : parentRelatedTechnologies)
            //    relatedTechnologies.push_back(pair);
        }
    }

    MaterialPreset findMaterialPreset(const std::string& name)
    {
        for (auto pair : m_materials)
        {
            for (const MaterialPreset& preset : pair.second)
            {
                if (preset.name == name)
                    return preset;
            }

        }
        return MaterialPreset();
    }

    std::string findInherits(const std::string& name, Preset::Type type)
    {
        Preset* p = NULL;
        if (type == Preset::TYPE_PRINT) {
            p = m_technologies->find_preset(name);
        } else if (type == Preset::TYPE_FILAMENT) {
            p = m_filaments->find_preset(name);
        } else if (type == Preset::TYPE_PRINTER) {
            p = m_printers->find_preset(name);
        }

        if (!p)
            return "Custom";

        if (p->vendor)
            return p->vendor->name;

        const ConfigOptionVectorBase* vec = static_cast<const ConfigOptionVectorBase*>(p->config.option("filament_vendor"));
        if (!vec) {
            auto inherits = p->inherits();
            if (!inherits.empty() && p->name != inherits)
                return findInherits(inherits, type);
            else
                return "Custom";
        }

        if (vec->is_vector()) {
            auto vendors = vec->vserialize();
            if (vendors.empty())
                return "Custom";
            else
                return vendors[0];
        } else
            return vec->serialize();
    }


    std::string getVendor(const Preset& preset)
    {        
        //auto              type = preset.type;
        //PresetCollection* col = NULL;
        //if (type == Preset::TYPE_PRINT) {
        //    col = m_technologies;
        //} else if (type == Preset::TYPE_FILAMENT) {
        //    col = m_filaments;
        //} else if (type == Preset::TYPE_PRINTER) {
        //    col = m_printers;
        //}
        //if (!col)
        //    return "Custom";

        //const PresetWithVendorProfile vendorProfile = col->get_preset_with_vendor_profile(preset);

        //if (vendorProfile.vendor)
        //    return vendorProfile.vendor->name;
        //else 
        //    return "Custom";

         if (preset.vendor)
             return preset.vendor->name;

         const ConfigOptionVectorBase* vec = static_cast<const ConfigOptionVectorBase*>(preset.config.option("filament_vendor"));
         if (!vec) {
             auto inherits = preset.inherits();
             if (!inherits.empty() && preset.name != inherits)
                 return findInherits(inherits, preset.type);
             else
                 return "Custom";
         }

         if (vec->is_vector()) 
         {
             auto vendors = vec->vserialize();
             if (vendors.empty())
                 return "Custom";
             else
                 return vendors[0];
         } else
             return vec->serialize();
    }

    std::string getPrinterModel(Preset preset)
    {
        //return preset.get_printer_type(m_preset_bundle);
        const ConfigOptionVectorBase* vec     = static_cast<const ConfigOptionVectorBase*>(preset.config.option("printer_model"));
        if (vec == NULL)
            return preset.get_printer_type(m_preset_bundle);

        if (vec->is_vector()) 
        {
            auto models = vec->vserialize();
            if (models.empty())
                return preset.get_printer_type(m_preset_bundle);
            else
                return models[0];
        }
        else 
            return vec->serialize();
        
    }

    std::vector<std::string> getDimensions(const Preset& preset)
    {
        const ConfigOptionVectorBase* vec     = static_cast<const ConfigOptionVectorBase*>(preset.config.option("nozzle_diameter"));
        if (vec == NULL)
            return std::vector<std::string>();

        auto                          vendors = vec->vserialize();
        return vendors;
    }

    void getCategory(PresetCollection* collection, Preset* preset, std::string& category)
    {
        if (!preset)
            return;

        int index = preset->name.find("@");
        if (index != -1) 
        {
            category = preset->name.substr(0, index);
            return;
        }

        std::string inherits = preset->inherits();
        if (inherits.empty() && preset->name != inherits)
        {
            category = preset->name;
            return;
        }

        Preset*     parentPreset = collection->find_preset(inherits);
        if (!parentPreset)
            return;

        getCategory(collection, parentPreset, category);
    }

    void reload()
    {
        m_printerBrands.clear();
        m_filamentBrands.clear();
        m_models.clear();
        m_dims.clear();
        m_categorys.clear();
        m_materials.clear();

        const std::deque<Preset>& ppresets  = m_printers->get_presets();
        const std::deque<Preset>& fpresets = m_filaments->get_presets();
        const auto& fpresets1 = m_filaments->get_filament_presets();
        std::vector<Preset>       fpresets2;
        int                       num = m_filaments->get_user_presets(m_preset_bundle, fpresets2);


        for (size_t i = fpresets.front().is_visible ? 0 : m_filaments->num_default_presets(); i < fpresets.size(); ++i) {
            const Preset& preset = fpresets[i];
            Preset        _preset = preset;

            if (!preset.is_visible || preset.is_project_embedded)
                continue;

            bool        isSystem    = preset.is_system;
            int         type        = preset.type;
            std::string mDetailName = preset.name;
            std::string category;
            _preset.get_filament_type(category);
            std::string alias = preset.alias;
            if (alias.empty())
                alias = category;

            MaterialPreset mpreset;
            mpreset.category = category;
            mpreset.name     = mDetailName;
            mpreset.isSystem = preset.is_system;
            mpreset.brand    = getVendor(preset);
            mpreset.isEnabled = preset.is_visible && !preset.is_project_embedded;
            //mpreset.isEnabled = preset.is_visible;

            m_categorys.insert(category);
            if (mpreset.isEnabled)
                m_validCategorys.insert(category);
            m_filamentBrands.insert(mpreset.brand);

            auto compatibles = preset.compatibles(); 
            for (auto& printer : preset.compatibles()) {
                Item item;
                item.isSystem = preset.is_system;
                item.name     = printer;
                /*              for (auto item : mpreset.relatedModels)
                              {
                                  if (item.name == printer)
                                      break;
                              }*/

                mpreset.relatedModels.push_back(item);
            }

            //for (auto& printer : compatibles) {
            //    for (auto& pair : m_modelCategorys)  {
            //        auto& mc = pair.second;
            //        if (mc.fullName == printer)
            //        {
            //            for (int i = 0, count = mc.childModels.size(); i < count; ++i) 
            //            {
            //                Item item;
            //                item.isSystem = mc.childModelRoots[i];
            //                item.name     = mc.childModels[i];
            //                mpreset.relatedModels.push_back(item);
            //            }
            //        }
            //    }
            //}

            std::string key = alias;
            getCategory(m_filaments, &_preset, key);

            m_materials[key].push_back(mpreset);
        }

        // printer preset
        for (size_t i = 0; i < ppresets.size(); ++i) {
            const Preset& preset  = ppresets[i];
            Preset        _preset = preset;

            if (!preset.is_visible || !preset.m_is_user_presets)
                continue;

            if (preset.is_project_embedded)
                continue;

            std::string              inherits = preset.inherits();
            std::string              brand    = getVendor(preset);
            std::string              model    = getPrinterModel(preset);
            std::vector<std::string> dims     = getDimensions(preset);

            m_printerBrands.insert(brand);
            m_models.insert(model);
            for (auto d : dims)
                m_dims.insert(d);

            for (auto d : dims) {
                std::string name = model + "-" + d;
                if (m_modelCategorys.find(name) == m_modelCategorys.end()) {
                    ModelCategory mc;
                    mc.fullName            = preset.name;
                    mc.name                = name;
                    mc.modelName           = model;
                    mc.nozzleName          = d;
                    m_modelCategorys[name] = mc;
                }

                m_modelCategorys[name].childModels.push_back(preset.name);
                m_modelCategorys[name].childModelRoots.push_back(preset.is_system);

                auto checkRepeat = [=](const std::string& name, const std::vector<Item>& items) {
                    bool repeat = false;
                    for (Item item : items) {
                        if (item.name == name) {
                            repeat = true;
                            break;
                        }
                    }
                    return repeat;
                };

                std::vector<std::pair<bool, std::string>> filaments;
                getPrinterRelatedFilaments(preset.name, filaments);
                // getPrinterRelatedFilaments1(preset.name, filaments);
                for (auto pair : filaments) {
                    if (checkRepeat(pair.second, m_modelCategorys[name].materialItems))
                        continue;

                    Item item;
                    item.isSystem = pair.first;
                    item.name     = pair.second;
                    m_modelCategorys[name].materialItems.push_back(item);
                }

                std::vector<std::pair<bool, std::string>> technologies;
                getPrinterRelatedTechnologies(preset.name, technologies);
                int   aa = 0;
                auto& mc = m_modelCategorys[name];
                for (auto pair : technologies) {
                    if (checkRepeat(pair.second, m_modelCategorys[name].techniqueItems))
                        continue;

                    Item item;
                    item.isSystem = pair.first;
                    item.name     = pair.second;
                    m_modelCategorys[name].techniqueItems.push_back(item);
                }
            }
            // m_models
        }

        //m_models
    }

    void relateModel(const std::string& filamentName, const std::vector<std::string>& newModelNames)
    {
        auto presets = m_filaments->get_presets();
        int  index   = -1;
        for (int i = 0, count = presets.size(); i < count; ++i) {
            auto preset = presets[i];
            if (preset.name == filamentName) {
                index = i;
                break;
            }
        }
        if (index == -1) {
            // not found
            return;
        }

        auto&                         pp     = m_filaments->preset(index);
        const ConfigOptionVectorBase* vec    = static_cast<const ConfigOptionVectorBase*>(pp.config.option("compatible_printers"));
        auto                          result = vec->vserialize();

        std::string value_str;
        for (const std::string& str : result) {
            value_str += "\"";
            value_str += str;
            value_str += "\";";
        }
        for (const std::string& str : newModelNames) {
            std::string parentName = _getParentPrinterName(str);
            value_str += "\"";
            value_str += str;
            value_str += "\";";
        }
        if (!value_str.empty())
            value_str = value_str.substr(0, value_str.size() - 1);

        ConfigSubstitutionContext context(ForwardCompatibilitySubstitutionRule::EnableSystemSilent);
        pp.config.set_deserialize("compatible_printers", value_str, context);
        pp.set_dirty();

        std::string newName = pp.name;
        m_filaments->save_current_preset(newName, false, false, &pp);
        auto new_preset       = m_filaments->find_preset(newName, false, true);
        new_preset->sync_info = "update";
        if (wxGetApp().is_user_login())
            new_preset->user_id = wxGetApp().getAgent()->get_user_id();
        new_preset->save_info();

        m_preset_bundle->update_compatible(PresetSelectCompatibleType::Never);

        reload();
    }

    void copyFilamanetAndRelateModels(const std::string& filamentName, const std::vector<std::string>& newModelNames) 
    {
        auto&            presets = m_filaments->get_presets();
        std::vector<int> nameSet;
        int              nameSize    = filamentName.size();
        int              targetIndex = -1;
        int              index       = 0;
        for (const Preset& p : presets) {
            std::string name = p.name;
            if (name == filamentName) {
                if (nameSet.size() < 1)
                    nameSet.resize(1);

                nameSet[0]  = 1;
                targetIndex = index;

            } else if (name.find(filamentName) == 0 && name[nameSize] == '(') {
                int end = name.find(')', nameSize);
                if (end != std::string::npos) {
                    std::string numberStr = name.substr(nameSize + 1, name.size() - nameSize - 2);
                    try {
                        int number = std::stoi(numberStr);
                        if (nameSet.size() < number + 1)
                            nameSet.resize(number + 1);
                        nameSet[number] = 1;
                    } catch (...) {}
                }
            }
            index++;
        }

        if (targetIndex == -1)
            return;

        int id = -1;
        for (int i = 0; i < nameSet.size(); ++i) {
            if (nameSet[i] == 0) {
                id = i;
                break;
            }
        }
        if (id == -1)
            id = nameSet.size();

        Preset*                       pp     = m_filaments->find_preset(filamentName);
        if (!pp)
            return;

        std::string newName = pp->name + "(" + std::to_string(id) + ")";
        m_filaments->save_current_preset(newName, false, false, pp);
        auto new_preset = m_filaments->find_preset(newName, false, true);

        const ConfigOptionVectorBase* vec    = static_cast<const ConfigOptionVectorBase*>(pp->config.option("compatible_printers"));
        auto                          result = vec->vserialize();

        std::string value_str;
        for (const std::string& str : newModelNames) {
            std::string parentName = _getParentPrinterName(str);
            value_str += "\"";
            value_str += str;
            value_str += "\";";
        }
        if (!value_str.empty())
            value_str = value_str.substr(0, value_str.size() - 1);

        ConfigSubstitutionContext context(ForwardCompatibilitySubstitutionRule::EnableSystemSilent);
        new_preset->config.set_deserialize("compatible_printers", value_str, context);
        new_preset->set_dirty();
        m_filaments->save_current_preset(newName, false, false, new_preset);
        new_preset->sync_info = "update";
        if (wxGetApp().is_user_login())
            new_preset->user_id = wxGetApp().getAgent()->get_user_id();
        new_preset->save_info();

        m_preset_bundle->update_compatible(PresetSelectCompatibleType::Never);

        reload();
    }

    std::string _getParentPrinterName(const std::string& printerName) { 
        std::string name = printerName;

        while (1) {
            Preset* preset = m_printers->find_preset(name);
            if (!preset)
                return name;

            std::string inherit = preset->inherits();
            if (inherit.empty() && preset->name != inherit)
                return name;

            name = inherit;
        }
    }

    void _copyFilamentPreset(PresetCollection* collection, const std::string& filamentName, std::string& newName, const std::string& newPrinter) {
            auto&            presets = collection->get_presets();
            std::vector<int> nameSet;
            int              nameSize    = filamentName.size();
            int              targetIndex = -1;
            int              index       = 0;
            for (const Preset& p : presets) {
                std::string name = p.name;
                if (name == filamentName) {
                    if (nameSet.size() < 1)
                        nameSet.resize(1);

                    nameSet[0]  = 1;
                    targetIndex = index;

                } else if (name.find(filamentName) == 0 && name[nameSize] == '(') {
                    int end = name.find(')', nameSize);
                    if (end != std::string::npos) {
                        std::string numberStr = name.substr(nameSize + 1, name.size() - nameSize - 2);
                        try {
                            int number = std::stoi(numberStr);
                            if (nameSet.size() < number + 1)
                                nameSet.resize(number + 1);
                            nameSet[number] = 1;
                        } catch (...) {}
                    }
                }
                index++;
            }

            if (targetIndex == -1)
                return;

            int id = -1;
            for (int i = 0; i < nameSet.size(); ++i) {
                if (nameSet[i] == 0) {
                    id = i;
                    break;
                }
            }
            if (id == -1)
                id = nameSet.size();

            newName       = filamentName + "(" + std::to_string(id) + ")";
            Preset preset = presets[targetIndex];
            collection->save_current_preset(newName, false, false, &preset);
            // m_filaments->save_current_preset(newName, false, false);
            auto new_preset       = collection->find_preset(newName, false, true);
            std::string                   value_str;
            value_str += "\"";
            value_str += newPrinter;
            value_str += "\"";
            ConfigSubstitutionContext context(ForwardCompatibilitySubstitutionRule::EnableSystemSilent);
            new_preset->config.set_deserialize("compatible_printers", value_str, context);
            new_preset->set_dirty();
            collection->save_current_preset(newName, false, false, new_preset);
            new_preset->sync_info = "update";
            if (wxGetApp().is_user_login())
                new_preset->user_id = wxGetApp().getAgent()->get_user_id();
            new_preset->save_info();
        };


    void relateFilament(const std::string& modelName, const std::vector<std::string>& newFilamentNames)
    {
        std::string parentName = _getParentPrinterName(modelName);
        for (const std::string& str : newFilamentNames) {

            std::string newName;
            _copyFilamentPreset(m_filaments, str, newName, parentName);
            m_preset_bundle->update_compatible(PresetSelectCompatibleType::Always);
        }

        reload();
        wxGetApp().get_tab(Preset::TYPE_FILAMENT)->load_current_preset();
    }

    void relateTechnology(const std::string& modelName, const std::vector<std::string>& newTechnologyNames)
    {
        std::string parentName = _getParentPrinterName(modelName);
        for (const std::string& str : newTechnologyNames) {
            std::string newName;
            _copyFilamentPreset(m_technologies, str, newName, parentName);
            //m_preset_bundle->update_compatible(PresetSelectCompatibleType::Never);
            m_preset_bundle->update_compatible(PresetSelectCompatibleType::Always);
        }

        reload();
        wxGetApp().get_tab(Preset::TYPE_PRINT)->load_current_preset();
    }

    bool isSameCategory(const Preset& preset, const ModelCategory& mc) 
    { 
        std::string              model = getPrinterModel(preset);
        if (model != mc.modelName)
            return false;
        
        std::vector<std::string> dims  = getDimensions(preset);

        for (auto d : dims)
        {
            if (d == mc.nozzleName)
                return true;
        }

        return false;

    }

    PresetBundle* m_preset_bundle{NULL};
    PresetCollection* m_filaments{NULL};
    PresetCollection* m_technologies{NULL};
    PresetCollection* m_printers{NULL};
    VendorMap*        m_vendors;

    //std::vector<Brand>                                 m_brands;
    std::set<std::string>                              m_printerBrands;
    std::set<std::string>                              m_filamentBrands;
    std::set<std::string>                              m_models;
    std::set<std::string>                              m_dims;
    std::set<std::string>                              m_categorys;
    std::set<std::string>                              m_validCategorys;
    //std::map<std::string, ModelPreset>    m_models;     // key: model name
    std::map<std::string, std::vector<MaterialPreset>> m_materials; // key : category
    std::map<std::string, ModelCategory>                       m_modelCategorys;

};

class CheckDialog : public DPIDialog
{
public:
    CheckDialog(wxWindow* parent, std::string title)
        : DPIDialog(parent,
                    wxID_ANY,
                    title.c_str(),
                    wxDefaultPosition,
                    wxSize(600, 275),
                    /*wxCAPTION*/ wxDEFAULT_DIALOG_STYLE)
    {
    }

    bool isOk() { return m_ok; }

protected:
    virtual void onOk()
    { 
        m_ok = true;
        this->Close();
    }
    virtual void  onCancel()
    {
        m_ok = false;
        this->Close();
    }

    virtual void on_dpi_changed(const wxRect& suggested_rect) override {}

protected:
    bool m_ok{false};

};

class CopyRelateDialog : public CheckDialog
{
public:
    CopyRelateDialog(wxWindow* parent, std::string title, std::string targetPrinter, Slic3r::GUI::RelateBundle* bundle)
        : CheckDialog(parent, title)
    {
        m_bundle        = bundle;
        m_targetPrinter = targetPrinter;
        initUI();
    }

    const std::vector<std::string> selected() 
    { 
        std::vector<std::string> selections;
        for (int i = 0, count = m_checkList.size(); i < count; ++i) 
        {
            if (m_checkList[i]->IsChecked())
                selections.push_back(m_checkNames[i]);
        }
        return selections;
    }

private:
    void initUI()
    {
        wxFont font = wxGetApp().normal_font();
        font.SetPixelSize(wxSize(12, 12));
        SetFont(font);
        SetBackgroundColour(*wxWHITE);

        wxBoxSizer* winSizer = new wxBoxSizer(wxVERTICAL);

        {
            wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);

            // 打印机选择
            wxStaticText* printerLabel = new wxStaticText(this, wxID_ANY, L"打印机选择", wxDefaultPosition, wxSize(80, 28), wxALIGN_CENTER);
            mainSizer->Add(printerLabel, 1, wxALIGN_CENTER_HORIZONTAL | wxTOP, 5);

            {
                wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);

                {
                    m_printerBox = new ComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(390, 28), 0, nullptr, wxCB_READONLY);
                    setFont(m_printerBox, 12);
                    rightSizer->Add(m_printerBox, 0, wxLEFT | wxTOP, 0);

                    m_printerBox->Bind(wxEVT_COMBOBOX, [=](auto& e) { loadSelections(); });
                }

                {
                    m_checkWindow = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(430, 100));
                    m_checkWindow->SetScrollbars(20, 20, 50, 50);
                    m_checkWindow->SetVirtualSize(390, 118);

                    rightSizer->Add(m_checkWindow, 0, wxTOP, 10);
                }
                mainSizer->Add(rightSizer, 2, wxRIGHT | wxTOP, 0);
            }
            winSizer->Add(mainSizer, 0, wxRIGHT | wxTOP, 30);
        }

        {
            // 按钮
            wxBoxSizer* btnSizer     = new wxBoxSizer(wxHORIZONTAL);
            _Button*    okButton     = new _Button(this, _L("Confirm"));
            _Button*    cancelButton = new _Button(this, _L("Cancel"));

            okButton->Bind(wxEVT_LEFT_DOWN, [=](wxMouseEvent& e) { onOk(); });
            cancelButton->Bind(wxEVT_LEFT_DOWN, [=](wxMouseEvent& e) { onCancel(); });

            btnSizer->Add(okButton, 1, wxALL, 10);
            btnSizer->Add(cancelButton, 1, wxALL, 10);
            winSizer->Add(btnSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 0);
        }
        SetSizer(winSizer);

        wxGetApp().UpdateDlgDarkUI(this);
    }

protected:
    void loadModels()
    {
        auto& mcs = m_bundle->m_modelCategorys;
        for (auto pair : mcs)
        {
            const std::string& name = pair.first;
            const RelateBundle::ModelCategory& mc   = pair.second;

            if (std::find(mc.childModels.begin(), mc.childModels.end(), m_targetPrinter) != mc.childModels.end())
                continue;

            if (!checkValid(mc))
                continue;

            wxString wxName(name.c_str());
            m_printerBox->Append(wxName);

            if (m_printerBox->GetTextLabel().empty())
                m_printerBox->SetSelection(0);
        }
    }

    bool checkValid(const RelateBundle::ModelCategory& mc) 
    {
        std::vector<RelateBundle::Item> related = getRelated(mc);

        bool valid = false;
        for (auto& item : related) {
            if (!item.isSystem) {
                valid = true;
                break;
            }
        }
        return valid;
    }

    virtual std::vector<RelateBundle::Item> getRelated(const RelateBundle::ModelCategory& mc) = 0;

    virtual void loadSelections()
    {
    }

protected:
    void on_dpi_changed(const wxRect& suggested_rect) override {}

protected:
    std::string                m_targetPrinter;
    Slic3r::GUI::RelateBundle* m_bundle;

    ComboBox*                m_printerBox;
    wxScrolledWindow*        m_checkWindow;
    std::vector<_CheckBox*>  m_checkList;
    std::vector<std::string> m_checkNames;

};

class CopyRelateFilamentDialog : public CopyRelateDialog
{
public:
    CopyRelateFilamentDialog(wxWindow* parent, std::string title, std::string targetPrinter, Slic3r::GUI::RelateBundle* bundle)
        : CopyRelateDialog(parent, title, targetPrinter, bundle)
    {
        loadModels();
        loadSelections();
    }

    virtual std::vector<RelateBundle::Item> getRelated(const RelateBundle::ModelCategory& mc)
    {
        return mc.materialItems;
    }

    virtual void loadSelections()
    {
        for (auto checkbox : m_checkList)
            delete checkbox;
        m_checkList.clear();
        m_checkNames.clear();

        std::string modelName = m_printerBox->GetTextLabel().ToStdString();
        if (modelName.empty())
            return;

        auto& mcs = m_bundle->m_modelCategorys;
        RelateBundle::ModelCategory targetMc;
        bool                        found = false;
        for (auto& pair : mcs) {
            //const std::string&                 name = pair.first;
            const RelateBundle::ModelCategory& mc = pair.second;
            for (auto& name : mc.childModels) {
                if (m_targetPrinter == name) {
                    targetMc = mc;
                    found    = true;
                    break;
                }
            }
            if (found)
                break;
        }
        std::set<std::string> exitsMaterials;
        for (const RelateBundle::Item& item : targetMc.materialItems)
        {
            exitsMaterials.insert(item.name);
        }

        for (auto& pair : mcs) {
            const std::string& name = pair.first;
            const RelateBundle::ModelCategory& mc   = pair.second;
            //if (!preset.is_visible)
            //    continue;
            if (name == modelName) {
                int index = 0;
                for (const RelateBundle::Item& item : mc.materialItems)
                {
                        if (item.isSystem)
                            continue;

                        if (exitsMaterials.find(item.name) != exitsMaterials.end())
                            continue;

                        _CheckBox* checkbox = new _CheckBox(m_checkWindow, from_u8(item.name).ToStdString());
                        checkbox->SetPosition(wxPoint(0, index * 37 + 10));
                        index++;
                        m_checkList.push_back(checkbox);
                        m_checkNames.push_back(item.name);
                }
                m_checkWindow->SetVirtualSize(390, index * 37 + 10);
                break;
            }
        }
    }

    std::string getSelectedPrinter() { return m_printerBox->GetStringSelection().ToStdString();
    }
};

class CopyRelateTechnologyDialog : public CopyRelateDialog
{
public:
    CopyRelateTechnologyDialog(wxWindow* parent, std::string title, std::string targetPrinter, Slic3r::GUI::RelateBundle* bundle)
        : CopyRelateDialog(parent, title, targetPrinter, bundle)
    {
        loadModels();
        loadSelections();
    }

    virtual std::vector<RelateBundle::Item> getRelated(const RelateBundle::ModelCategory& mc)
    {
        return mc.techniqueItems;
    }
    virtual void loadSelections()
    {
        for (auto checkbox : m_checkList)
            delete checkbox;
        m_checkList.clear();
        m_checkNames.clear();

        std::string modelName = m_printerBox->GetTextLabel().ToStdString();
        if (modelName.empty())
            return;

        auto& mcs = m_bundle->m_modelCategorys;
        for (auto& pair : mcs) {
            const std::string&                 name = pair.first;
            const RelateBundle::ModelCategory& mc   = pair.second;
            // if (!preset.is_visible)
            //     continue;
            if (mc.name == modelName) {
                int index = 0;
                for (const RelateBundle::Item& item : mc.techniqueItems) {
                    if (item.isSystem)
                        continue;
                    _CheckBox* checkbox = new _CheckBox(m_checkWindow, from_u8(item.name).ToStdString());
                    checkbox->SetSize(wxSize(400, 20));
                    // materialCheckBox1->SetValue(true);
                    checkbox->SetPosition(wxPoint(0, index * 40));
                    index++;
                    m_checkList.push_back(checkbox);
                    m_checkNames.push_back(item.name);
                }
                m_checkWindow->SetVirtualSize(390, index * 37 + 10);
                break;
            }
        }
    }
};

class RelatePrinterDialog : public CheckDialog
{
public:
    RelatePrinterDialog(wxWindow* parent, std::string title, std::string target, Slic3r::GUI::RelateBundle* bundle)
        : CheckDialog(parent, title)
    {
        wxFont font = wxGetApp().normal_font();
        font.SetPixelSize(wxSize(12, 12));
        SetFont(font);
        //SetFont(wxGetApp().normal_font());
        SetBackgroundColour(*wxWHITE);

        m_bundle        = bundle;
        m_target = target;
       
        initUI();
    }

    const std::vector<std::string> selected()
    {
        auto& mcs = m_bundle->m_modelCategorys;
        std::vector<std::string> selections;
        for (int i = 0, count = m_checkList.size(); i < count; ++i) {
            if (!m_checkList[i]->IsChecked())
                continue;

            for (auto& pair : mcs)
            {
                if (pair.first != m_checkNames[i])
                    continue;
                        
                auto& mc = pair.second;
                //for (const std::string& model : mc.childModels) 
                //{
                //    selections.push_back(model);
                //}
                selections.push_back(mc.fullName);
                break;
            }
        }
        return selections;
    }

private:
    void initUI()
    {
        wxBoxSizer* winSizer = new wxBoxSizer(wxVERTICAL);
        {
            wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);

            // 打印机选择
            wxStaticText* printerLabel = new wxStaticText(this, wxID_ANY, _L("Select Printer"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
            setFont(printerLabel, 12);
            printerLabel->SetSize(wxSize(62, 13));

            mainSizer->Add(printerLabel, 1, wxALIGN_CENTER_HORIZONTAL | wxTOP, 5);

            {
                wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);

                {
                    m_checkWindow = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(430, 138));
                    m_checkWindow->SetScrollbars(20, 20, 50, 50);
                    m_checkWindow->SetVirtualSize(390, 138);

                    for (auto checkbox : m_checkList)
                        delete checkbox;
                    m_checkList.clear();

                    RelateBundle::MaterialPreset preset = m_bundle->findMaterialPreset(m_target);
                    std::vector<RelateBundle::Item>& relatedModels = preset.relatedModels;

                    int   index    = 0;
                    auto& mcs   = m_bundle->m_modelCategorys;
                    for (auto& pair : mcs) {
                        const std::string&                 cName = pair.first;
                        const RelateBundle::ModelCategory& mc = pair.second;

                        bool isAllRelated = true;
                        for (const std::string& model : mc.childModels) 
                        {
                            for (auto& item : relatedModels) {
                                if (item.name == model) {
                                    isAllRelated = false;
                                    break;
                                }
                            }
                        }
                        if (!isAllRelated)
                            continue;

                        _CheckBox* checkbox = new _CheckBox(m_checkWindow, from_u8(cName).ToStdString());
                        checkbox->SetSize(wxSize(400, 20));
                        setFont(checkbox, 12);
                        // materialCheckBox1->SetValue(true);

                        int column = index % 2;
                        int row    = index / 2;
                        checkbox->SetPosition(wxPoint(0 * 200, index * 37));
                        index++;
                        m_checkList.push_back(checkbox);
                        m_checkNames.push_back(cName);
                    }
                    m_checkWindow->SetVirtualSize(390, index * 37);
                    // scrolledWin->SetSize(460, 108);
                    rightSizer->Add(m_checkWindow, 0, wxLEFT, 0);
                }
                mainSizer->Add(rightSizer, 2, wxRIGHT | wxTOP, 0);
            }
            winSizer->Add(mainSizer, 0, wxRIGHT | wxTOP, 30);
        }

        {
            // 按钮
            wxBoxSizer* btnSizer     = new wxBoxSizer(wxHORIZONTAL);
            _Button*    okButton     = new _Button(this, _L("Confirm"));
            _Button*    cancelButton = new _Button(this, _L("Cancel"));

            okButton->Bind(wxEVT_LEFT_DOWN, [=](wxMouseEvent& e) { onOk(); });
            cancelButton->Bind(wxEVT_LEFT_DOWN, [=](wxMouseEvent& e) { onCancel(); });

            btnSizer->Add(okButton, 1, wxALL, 10);
            btnSizer->Add(cancelButton, 1, wxALL, 10);
            winSizer->Add(btnSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 0);
        }
        SetSizer(winSizer);

        wxGetApp().UpdateDlgDarkUI(this);
    }

protected:
    void on_dpi_changed(const wxRect& suggested_rect) override {}

private:
    std::string                m_target;
    Slic3r::GUI::RelateBundle* m_bundle;

    ComboBox*                m_printerBox;
    wxScrolledWindow*        m_checkWindow;
    std::vector<_CheckBox*>  m_checkList;
    std::vector<std::string> m_checkNames;

};

class _WarningTipsDialog : public CheckDialog
{
public:
    _WarningTipsDialog(wxWindow* parent, wxString text): 
        CheckDialog(parent, _L("Warning").ToStdString()),
        m_text(text)
    {
// _L("The printers are from different vendors. Please check the parameters to avoid printing failure.")
        this->SetBackgroundColour(*wxWHITE);
        std::string icon_path = (boost::format("%1%/images/Creative3DTitle.ico") % resources_dir()).str();
        SetIcon(wxIcon(encode_path(icon_path.c_str()), wxBITMAP_TYPE_ICO));

        wxBoxSizer *main_sizer = new wxBoxSizer(wxVERTICAL);
        // top line
        auto m_line_top = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1), wxTAB_TRAVERSAL);
        m_line_top->SetBackgroundColour(wxColour(0xA6, 0xa9, 0xAA));
        main_sizer->Add(m_line_top, 0, wxEXPAND, 0);
        main_sizer->Add(0, 0, 0, wxTOP, FromDIP(5));

        wxBoxSizer *    content_sizer = new wxBoxSizer(wxHORIZONTAL);
        wxStaticBitmap *info_bitmap   = new wxStaticBitmap(this, wxID_ANY, create_scaled_bitmap("info", nullptr, 60), wxDefaultPosition, wxSize(FromDIP(70), FromDIP(70)), 0);
        wxBoxSizer *    msg_sizer     = get_msg_sizer();
        content_sizer->Add(info_bitmap, 0, wxEXPAND | wxALL, FromDIP(5));
        content_sizer->Add(msg_sizer, 0, wxEXPAND | wxALL, FromDIP(5));
        main_sizer->Add(content_sizer, 0, wxEXPAND | wxALL, FromDIP(5));
        {
            // 按钮
            wxBoxSizer* btnSizer     = new wxBoxSizer(wxHORIZONTAL);
            _Button*    okButton     = new _Button(this, _L("Resume"));
            _Button*    cancelButton = new _Button(this, _L("Cancel"));

            okButton->Bind(wxEVT_LEFT_DOWN, [=](wxMouseEvent& e) { onOk(); });
            cancelButton->Bind(wxEVT_LEFT_DOWN, [=](wxMouseEvent& e) { onCancel(); });

            btnSizer->Add(okButton, 1, wxALL, 10);
            btnSizer->Add(cancelButton, 1, wxALL, 10);
            main_sizer->Add(btnSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxRIGHT, 0);
        }

        this->SetSizer(main_sizer);
        Layout();
        Fit();
        wxGetApp().UpdateDlgDarkUI(this);

    }

private:
    wxBoxSizer *get_msg_sizer()
    {
        wxBoxSizer *vertical_sizer     = new wxBoxSizer(wxVERTICAL);
        wxStaticText *text1 = new wxStaticText(this, wxID_ANY, m_text);

        vertical_sizer->Add(text1, 0, wxEXPAND | wxTOP, FromDIP(5));

        return vertical_sizer;
    }

    wxString m_text;
};


class PrinterRelatePanel : public wxPanel
{
public:
    PrinterRelatePanel(wxWindow* parent) : wxPanel(parent)
    {
        SetBackgroundColour(*wxWHITE);
        // SetBackgroundColour(*wxRED);

        wxBoxSizer* sizer       = new wxBoxSizer(wxVERTICAL);

        wxPanel* filterPanel = new wxPanel(this);
        initFilterPanel(filterPanel);
        sizer->Add(filterPanel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

        wxPanel* middle_line = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(windowSize.x, 1),
                                         wxTAB_TRAVERSAL);
        middle_line->SetBackgroundColour(DESIGN_GRAY400_COLOR);
        sizer->Add(middle_line, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

        wxPanel* relatePanel = new wxPanel(this);
        initRelatePanel(relatePanel, parent);
        sizer->Add(relatePanel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

        SetSizer(sizer);
    }

    void loadBundle(RelateBundle* bundle)
    { 
        m_bundle = bundle;
        _update();
    }

    void selectTab(int index) { m_tab->select(index); }

    private:
        wxDataViewItem selectedModel(std::string& name)
        {
            wxDataViewItem id = m_treeCtrl->GetSelection();
            name = into_u8(m_treeCtrl->GetItemText(id).ToStdString());
            return id;
        }

        void select(const std::string& modelName)
        {
            for (auto id : m_userIds) 
            {
                if (into_u8(m_treeCtrl->GetItemText(id).ToStdString()) == modelName)
                {
                    m_treeCtrl->Select(id);
                    return;
                }
            }
            for (auto id : m_systemIds) {
                if (into_u8(m_treeCtrl->GetItemText(id).ToStdString()) == modelName) {
                    m_treeCtrl->Select(id);
                    return;
                }
            }
        }

        void _update()
        {
            
            m_brandChoice->Clear();
            m_brandChoice->Append(_L("all"));
            m_brandChoice->SetSelection(0);
            m_modelChoice->Clear();
            m_modelChoice->Append(_L("all"));
            m_modelChoice->SetSelection(0);
            m_nozzleChoice->Clear();
            m_nozzleChoice->Append(_L("all"));
            m_nozzleChoice->SetSelection(0);

            if (!m_bundle)
                return;

            for (const std::string& b : m_bundle->m_printerBrands)
                m_brandChoice->Append(b);
            for (const std::string& m : m_bundle->m_models)
                m_modelChoice->Append(m);
            for (const std::string& d : m_bundle->m_dims)
                m_nozzleChoice->Append(d + "mm");
            
            _updateTree();
        }

        void _syncOtherChoicesWithBrand()
        {
            std::string specialBrand = m_brandChoice->GetSelection() == 0 ? std::string("") : m_brandChoice->GetTextLabel().ToStdString();
            std::string specialModel = m_modelChoice->GetSelection() == 0 ? std::string("") : m_modelChoice->GetTextLabel().ToStdString();
            m_modelChoice->Clear();
            m_modelChoice->Append(_L("all"));
            m_modelChoice->SetSelection(0);

            if (specialBrand.empty()) {
                for (const std::string& m : m_bundle->m_models) {
                    m_modelChoice->Append(m);
                    if (specialModel == m)
                        m_modelChoice->SetSelection(m_modelChoice->GetCount() - 1);
                }
                return;
            }

            auto& models = m_bundle->m_models;
            for (const auto& pair : *(m_bundle->m_vendors))
            {
                if (pair.first != specialBrand)
                    continue;

                auto v = pair.second;
                for (auto& m : v.models)
                {
                    for (auto& n : models) {
                        if (n == m.name) {
                            m_modelChoice->Append(m.name);
                            if (specialModel == m.name)
                                m_modelChoice->SetSelection(m_modelChoice->GetCount() - 1);
                            break;
                        }
                    }
                }
            }
        }

        void _syncOtherChoicesWithModel()
        {
            std::string specialModel = m_modelChoice->GetSelection() == 0 ? std::string("") : m_modelChoice->GetTextLabel().ToStdString();
            std::string specialBrand = m_brandChoice->GetSelection() == 0 ? std::string("") : m_brandChoice->GetTextLabel().ToStdString();
            m_brandChoice->Clear();
            m_brandChoice->Append(_L("all"));
            m_brandChoice->SetSelection(0);
            if (specialModel.empty()) 
            {
                for (const std::string& b : m_bundle->m_printerBrands) {
                    m_brandChoice->Append(b);
                    if (specialBrand == b)
                        m_brandChoice->SetSelection(m_brandChoice->GetCount() - 1);
                }
                return;
            }

            auto& brands = m_bundle->m_printerBrands;
            for (const auto& pair : *(m_bundle->m_vendors)) {
                bool isContain = false;
                for (auto& b : brands)
                {
                    if (b == pair.first)
                    {
                        isContain = true;
                        break;
                    }
                }
                if (!isContain)
                    continue;

                isContain = false;
                auto v = pair.second;
                for (auto& m : v.models) {
                    if (m.name == specialModel)
                    {
                        isContain = true;
                        break;
                    }
                }
                if (isContain) {
                    m_brandChoice->Append(pair.first);
                    if (specialBrand == pair.first)
                        m_brandChoice->SetSelection(m_brandChoice->GetCount() - 1);
                }
            }
        }

        void _updateTree()
        {
            m_binding = false;
            m_treeCtrl->DeleteAllItems();
            m_userIds.clear();
            m_systemIds.clear();

            if (!m_bundle)
                return;

            std::string specialBrand = m_brandChoice->GetSelection() == 0 ? std::string("") : m_brandChoice->GetTextLabel().ToStdString();
            std::string specialModel = m_modelChoice->GetSelection() == 0 ? std::string("") : m_modelChoice->GetTextLabel().ToStdString();
            std::string specialDim   = m_nozzleChoice->GetSelection() == 0 ? std::string("") : m_nozzleChoice->GetTextLabel().ToStdString();
            if (!specialDim.empty())
                specialDim = specialDim.substr(0, specialDim.size() - 2);
            std::string filterName = into_u8(m_printerSearch->GetTextCtrl()->GetValue());
            std::string pattern;
            for (int i = 0; i < filterName.size(); ++i)
            {
                pattern = pattern.append(std::string(".*") + filterName[i]);
            }
            if (!pattern.empty())
                pattern = pattern.append(std::string (".*"));

            bool hasUserRoot   = false;
            bool hasSystemRoot = false;
            m_userPreset       = m_treeCtrl->AppendContainer(wxDataViewItem(nullptr), _L("User Preset"));
            m_systemPreset     = m_treeCtrl->AppendContainer(wxDataViewItem(nullptr), _L("System Preset"));
            auto& ppresets = m_bundle->printerPresets();
            for (const Preset& preset : ppresets) {
                if (!preset.is_visible || preset.is_project_embedded)
                    continue;
                //if (!preset.is_visible)
                //    continue;

                if (!specialBrand.empty())
                {
                    auto brand = m_bundle->getVendor(preset);
                    if (specialBrand != brand) 
                        continue; 
                }
           
                if (!specialModel.empty()) {
                    std::string model = m_bundle->getPrinterModel(preset);
                    if (model != specialModel)
                        continue;
                }

                if (!specialDim.empty()) 
                {
                    float                         dim = std::stof(specialDim);
                    std::vector<std::string>      dims  = m_bundle->getDimensions(preset);
                    bool                          found = false;
                    for (std::string& d : dims) 
                    {
                        if (abs(std::stof(d) - dim) < 0.001)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        continue;
                }

                if (!filterName.empty())
                {
                    try {
                        std::regex re(pattern);
                        if (!std::regex_match(preset.name, re))
                            continue;

                    } catch (const std::regex_error& e) {}
                }

                std::string str = from_u8(preset.name).ToStdString();
                if (preset.is_system) {
                    if (!hasSystemRoot) {
                        hasSystemRoot  = true;
                    }
                    auto id = m_treeCtrl->AppendItem(m_systemPreset, str);
                    m_systemIds.push_back(id);
                } else {
                    if (!hasUserRoot) {
                        hasUserRoot = true;
                    }
                    auto id = m_treeCtrl->AppendItem(m_userPreset, str);
                    m_userIds.push_back(id);
                }

                //m_treeCtrl->SetItemTextColour(,);
            }
            if (hasSystemRoot) 
                m_treeCtrl->Expand(m_systemPreset);
            else
                m_treeCtrl->DeleteItem(m_systemPreset);
            if (hasUserRoot)
                m_treeCtrl->Expand(m_userPreset);
            else
                m_treeCtrl->DeleteItem(m_userPreset);

            m_binding = true;
            if (!m_userIds.empty())
                m_treeCtrl->Select(m_userIds[0]);
            else if (!m_systemIds.empty())
                m_treeCtrl->Select(m_systemIds[0]);

            _sync();
        }

        void _sync()
        { 
            if (!m_binding)
                return;

            m_materialListCtrl->init(_L("Filament presets name").ToStdString(), _L("Preset Type").ToStdString());
            m_technologiesListCtrl->init(_L("Process presets name").ToStdString(), _L("Preset Type").ToStdString());

            if (!m_bundle)
                return;
            auto id = m_treeCtrl->GetSelection();
            std::string modelName(into_u8(m_treeCtrl->GetItemText(id).ToStdString()));

            m_addMaterialButton->Enable(m_bundle->checkMaterialRelateValid(modelName));
            m_addProcessButton->Enable(m_bundle->checkProcessRelateValid(modelName));
            
            std::vector<std::pair<bool, std::string>> relatedFilaments;
            m_bundle->getPrinterRelatedFilaments(modelName, relatedFilaments);
            bool isSingle = true;
            for (auto& pair : relatedFilaments) {
                if (!pair.first)
                    continue;
                _ListItem nameItem;
                initListItem(nameItem, from_u8(pair.second).ToStdString(), isSingle, true, 0);
                m_materialListCtrl->insertNameItem(nameItem);

                _ListItem typeItem;
                initListItem(typeItem, "", isSingle, true, 1);
                m_materialListCtrl->insertTypeItem(typeItem);

                isSingle = !isSingle;
            }
            for (auto& pair : relatedFilaments) {
                if (pair.first)
                    continue;

                _ListItem nameItem;
                initListItem(nameItem, from_u8(pair.second).ToStdString(), isSingle, false, 0);
                m_materialListCtrl->insertNameItem(nameItem);

                _ListItem typeItem;
                initListItem(typeItem, "", isSingle, false, 1);
                m_materialListCtrl->insertTypeItem(typeItem);

                isSingle = !isSingle;
            }

            isSingle = true;
            std::vector<std::pair<bool, std::string>> relatedTechnologies;
            m_bundle->getPrinterRelatedTechnologies(modelName, relatedTechnologies);
            for (auto& pair : relatedTechnologies) {
                if (!pair.first)
                    continue;
                _ListItem nameItem;
                initListItem(nameItem, from_u8(pair.second).ToStdString(), isSingle, true, 0);
                m_technologiesListCtrl->insertNameItem(nameItem);

                _ListItem typeItem;
                initListItem(typeItem, "", isSingle, true, 1);
                m_technologiesListCtrl->insertTypeItem(typeItem);

                isSingle = !isSingle;
            }
            for (auto& pair : relatedTechnologies) {
                if (pair.first)
                    continue;
                _ListItem nameItem;
                initListItem(nameItem, from_u8(pair.second).ToStdString(), isSingle, false, 0);
                m_technologiesListCtrl->insertNameItem(nameItem);

                _ListItem typeItem;
                initListItem(typeItem, "", isSingle, false, 1);
                m_technologiesListCtrl->insertTypeItem(typeItem);

                isSingle = !isSingle;
            }
          
            m_materialListCtrl->Refresh();
            m_technologiesListCtrl->Refresh();
            Update();
        }

        void initFilterPanel(wxPanel* parent)
        {
            wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
            {
                wxStaticText* brandLabel = new wxStaticText(parent, wxID_ANY, _L("Vendor"));
                //m_brandChoice = new wxChoice(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize);
                m_brandChoice = new ComboBox(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(130, 28), 0, nullptr,
                                                        wxCB_READONLY);
                m_brandChoice->Bind(wxEVT_COMBOBOX, [=](auto& e) 
                    { 
                        _updateTree(); 
                        _syncOtherChoicesWithBrand();
                    });

                sizer->Add(brandLabel, 0, wxALL | wxALIGN_CENTER, 5);
                sizer->Add(m_brandChoice, 0, wxALL | wxALIGN_CENTER, 5);
            }

            {
                wxStaticText* modelLabel = new wxStaticText(parent, wxID_ANY, _L("Printer"));
                m_modelChoice = new ComboBox(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(130, 28), 0, nullptr, wxCB_READONLY);
                m_modelChoice->Bind(wxEVT_COMBOBOX, [=](auto& e) { 
                    _updateTree();
                    _syncOtherChoicesWithModel();
                    });

                sizer->Add(modelLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
                sizer->Add(m_modelChoice, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
            }

            {
                wxStaticText* nozzleLabel = new wxStaticText(parent, wxID_ANY, _L("Nozzle"));
                m_nozzleChoice            = new ComboBox(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(130, 28), 0, nullptr,
                                                         wxCB_READONLY);
                m_nozzleChoice->Bind(wxEVT_COMBOBOX, [=](auto& e) { _updateTree(); });
                sizer->Add(nozzleLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
                sizer->Add(m_nozzleChoice, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
            }

            {
                wxStaticText* printerLabel = new wxStaticText(parent, wxID_ANY, _L("Preset name"));

                m_printerSearch = new ::TextInput(parent, wxEmptyString, wxEmptyString, wxEmptyString, wxDefaultPosition, wxSize(124, 24),
                                             wxTE_PROCESS_ENTER);
                StateColor input_bg(std::pair<wxColour, int>(wxColour("#F0F0F1"), StateColor::Disabled),
                                    std::pair<wxColour, int>(*wxWHITE, StateColor::Enabled));
                m_printerSearch->SetBackgroundColor(input_bg);


                ////m_printerSearch            = new TextInputCtrl(parent, wxID_ANY, wxEmptyString);
                //m_printerSearch->SetBorderColor(StateColor(std::make_pair(0x00FF00, (int) StateColor::Focused),
                //                                               std::make_pair(0x30BF40, (int) StateColor::Hovered),
                //                                               std::make_pair(0xEEEEEE, (int) StateColor::Normal)));
                //m_printerSearch->SetFont(::Label::Body_14);

                //m_printerSearch->SetLabelColor(AMS_MATERIALS_SETTING_GREY800);
                m_printerSearch->GetTextCtrl()->Bind(wxEVT_TEXT, [=](wxCommandEvent& e) {
                    _updateTree();
                    });
                //m_printerSearch->GetTextCtrl()->Hide();

                m_printerSearch->Bind(wxEVT_COMBOBOX, [=](auto& e) { _updateTree(); });
                sizer->Add(printerLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
                sizer->Add(m_printerSearch, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
            }

            parent->SetSizer(sizer);
        }
        void splitPrinterName(const std::string& printerName, std::string& printerNameAfter, std::string& nozzle)
        {
            int pos  = printerName.rfind("-");
            if (pos != -1) {
                printerNameAfter = printerName.substr(0, pos);
                nozzle           = printerName.substr(pos + 1, printerName.length() - pos - 1);
            } else {
                printerNameAfter = printerName;
            }
        }
        void initRelatePanel(wxPanel* parent, wxWindow* parentWindow)
        {
            m_treeCtrl = new _TreeView(parent);
            m_treeCtrl->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, [this](wxCommandEvent& e) { _sync(); });

            wxPanel*       tabView  = new wxPanel(parent, wxID_ANY, wxDefaultPosition, rightTabSize);
            wxToolbook*    notebook = new wxToolbook(tabView, wxID_ANY);
            m_toolbook              = notebook;
            wxToolBarBase* toolBar  = notebook->GetToolBar();
            toolBar->Hide();
            _Tab* tab = new _Tab(tabView);
            m_tab     = tab;
            tab->appendTab(_L("Related Filament"));
            tab->appendTab(_L("Related Process"));
            SelectedChangedFunc func = [notebook](int index) {
                notebook->SetSelection(index);
            };
            tab->bind(func);
            {
                wxPanel* relateMaterialPage = new wxPanel(notebook, wxID_ANY, wxDefaultPosition, rightTabContentSize);
                notebook->AddPage(relateMaterialPage, _L("Related Filament"));

                //m_materialListCtrl = new _ListView(relateMaterialPage, wxID_ANY, wxDefaultPosition, wxSize(460, 300), wxLC_REPORT);
                m_materialListCtrl = new _ListWidget(relateMaterialPage);
                //m_materialListCtrl->SetBackgroundColour(wxColour(*wxWHITE));

                m_addMaterialButton = new _Button(relateMaterialPage, _L("Add Related Filament"), wxSize(FromDIP(160),FromDIP(24)));
                m_addMaterialButton->Bind(wxEVT_LEFT_DOWN, [=](wxMouseEvent& e) {
                    std::string       targetPrinter;
                    wxDataViewItem      id    = selectedModel(targetPrinter);
                    std::string       title = _L("Copy filament from other printers and relate to ").ToStdString() +
                                        from_u8(targetPrinter).ToStdString();
                    CopyRelateFilamentDialog dlg(parentWindow, title, targetPrinter, m_bundle);
                    dlg.ShowModal();

                    if (dlg.isOk()) {
                        auto& filaments = dlg.selected();
                        std::vector<std::string> printers;
                        printers.push_back(targetPrinter);
                        std::string selectedPrinter = dlg.getSelectedPrinter();
                        std::string          printerName2;
                        std::string          nozzle;
                        splitPrinterName(selectedPrinter, printerName2, nozzle);
                        if (selectedPrinter != printerName2){
                            selectedPrinter = printerName2 + " " + nozzle + " nozzle";
                        }
                        bool done = m_bundle->isSameVendorPrinter(targetPrinter, selectedPrinter);
                        //bool done = m_bundle->isSameVendorFilament(printers, filaments);  
                        if (!done) {
                            _WarningTipsDialog wdlg(parentWindow,
                                     _L("The printers are from different vendors. Please check the parameters to avoid printing failure."));
                            wdlg.ShowModal();
                            done = wdlg.isOk();
                        }

                        if (done) {
                            m_bundle->relateFilament(targetPrinter, filaments);
                            _update();
                            select(targetPrinter);
                            _sync();
                        } 
                    }
                });

                wxBoxSizer* amSizer = new wxBoxSizer(wxHORIZONTAL);
                //amSizer->AddSpacer(FromDIP(140));
                amSizer->Add(m_addMaterialButton, 0, wxALIGN_CENTER);
                //amSizer->AddSpacer(FromDIP(140));

                wxBoxSizer* rmSizer = new wxBoxSizer(wxVERTICAL);  
                rmSizer->Add(m_materialListCtrl, 0, wxEXPAND);
                rmSizer->AddSpacer(FromDIP(5));
                rmSizer->Add(amSizer, 0, wxALIGN_CENTER | wxTOP, 21);
                relateMaterialPage->SetSizer(rmSizer);

            }
            {
                wxPanel* relateTechniquePage = new wxPanel(notebook);
                notebook->AddPage(relateTechniquePage, _L("Related Print")); 
                m_technologiesListCtrl = new _ListWidget(relateTechniquePage);
                //m_technologiesListCtrl->SetBackgroundColour(wxColour(*wxWHITE));

                m_addProcessButton = new _Button(relateTechniquePage, _L("Add Related Process"), wxSize(FromDIP(160), FromDIP(24)));
                m_addProcessButton->Bind(wxEVT_LEFT_DOWN, [=](wxMouseEvent& e) {
                    std::string              targetPrinter;
                    wxDataViewItem             id    = selectedModel(targetPrinter);
                    std::string                title = "Copy technologies from other printers to " + from_u8(targetPrinter).ToStdString();
                    CopyRelateTechnologyDialog dlg(parentWindow, title, targetPrinter, m_bundle);
                    dlg.ShowModal();

                    if (dlg.isOk()) {
                        auto&                    processes = dlg.selected();
                        std::vector<std::string> printers;
                        printers.push_back(targetPrinter);
                        bool done = m_bundle->isSameVendorProcess(printers, processes);
                        if (!done) {
                            _WarningTipsDialog
                                wdlg(parentWindow,
                                     _L("The printers are from different vendors. Please check the parameters to avoid printing failure."));
                            wdlg.ShowModal();
                            done = wdlg.isOk();
                        }

                        if (done) {
                            m_bundle->relateTechnology(targetPrinter, processes);
                            _update();
                            select(targetPrinter);
                            _sync();
                        }
                    }
                });

                wxBoxSizer* amSizer = new wxBoxSizer(wxHORIZONTAL);
                amSizer->Add(m_addProcessButton, 0, wxALIGN_CENTER);

                wxBoxSizer* rmSizer = new wxBoxSizer(wxVERTICAL);
                rmSizer->Add(m_technologiesListCtrl, 0, wxEXPAND);
                rmSizer->AddSpacer(FromDIP(5));
                rmSizer->Add(amSizer, 0, wxALIGN_CENTER | wxTOP, 21);
                relateTechniquePage->SetSizer(rmSizer);

            }

            wxBoxSizer* pSizer = new wxBoxSizer(wxHORIZONTAL);
            pSizer->Add(m_treeCtrl, 3, wxEXPAND | wxALL, 0);

            //tabView
            tab->SetPosition(wxPoint(0, 0));
            wxBoxSizer* tabSizer = new wxBoxSizer(wxVERTICAL);
            //tabSizer->Add(tab, 0, wxEXPAND);
            tabSizer->Add(notebook, 0, wxTOP, 22);
            tabView->SetSizer(tabSizer);

            pSizer->Add(tabView, 5, wxEXPAND | wxLEFT, 20);
            parent->SetSizer(pSizer);
        }

    private:
        bool          m_binding{false};
        RelateBundle* m_bundle{NULL};

        /* 过滤区域 */
        ComboBox* m_brandChoice;
        ComboBox* m_modelChoice;
        ComboBox* m_nozzleChoice;
        ::TextInput* m_printerSearch;

        /* 参数显示区域 */
        _TreeView*         m_treeCtrl;
        wxDataViewItem              m_userPreset;
        std::vector<wxDataViewItem> m_userIds;
        wxDataViewItem              m_systemPreset;
        wxDataViewColumn*           m_testColumn;
        std::vector<wxDataViewItem> m_systemIds;
        _Tab*                       m_tab;

        _ListWidget* m_materialListCtrl;
        _ListWidget* m_technologiesListCtrl;
        wxToolbook*  m_toolbook;

        _Button* m_addMaterialButton;
        _Button* m_addProcessButton;

};

class MaterialRelatePanel : public wxPanel
{
public:
    MaterialRelatePanel(wxWindow* parent) : wxPanel(parent)
    {
        SetBackgroundColour(*wxWHITE);
        // SetBackgroundColour(*wxBLUE);

        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

        wxPanel* filterPanel = new wxPanel(this);
        initFilterPanel(filterPanel);
        sizer->Add(filterPanel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

        wxPanel* middle_line = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(windowSize.x, 1), wxTAB_TRAVERSAL);
        middle_line->SetBackgroundColour(DESIGN_GRAY400_COLOR);
        sizer->Add(middle_line, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

        wxPanel* relatePanel = new wxPanel(this);
        initRelatePanel(relatePanel, parent);
        sizer->Add(relatePanel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

        SetSizer(sizer);
    }

    void loadBundle(RelateBundle* bundle)
    {
        m_bundle = bundle;
        _update();
    }

    void selectTab(int index) { m_tab->select(index); }

  private:
    std::string selectedFilament()
    {
        auto        id = m_treeCtrl->GetSelection();
        std::string filamentName = into_u8(m_treeCtrl->GetItemText(id).ToStdString());
        return filamentName;
    }

    void select(const std::string& filamentName)
    {
        for (auto id : m_treeIds) {
            if (into_u8(m_treeCtrl->GetItemText(id).ToStdString()) == filamentName) {
                m_treeCtrl->Select(id);
                return;
            }
        }
    }

    void _update()
    {
        //m_treeCtrl->clear
        //m_treeCtrl->DeleteChildren(m_userPreset);

        m_brandChoice->Clear();
        m_brandChoice->Append(_L("all"));
        m_brandChoice->SetSelection(0);
        m_categoryChoice->Clear();
        m_categoryChoice->Append(_L("all"));
        m_categoryChoice->SetSelection(0);


        for (const std::string& b : m_bundle->m_filamentBrands)
            m_brandChoice->Append(b);
        for (const std::string& c : m_bundle->m_validCategorys)
            m_categoryChoice->Append(c);

        _updateTree();
    }

    void _syncOtherChoicesWithBrand()
    {
        std::string specialBrand = m_brandChoice->GetSelection() == 0 ? std::string("") : m_brandChoice->GetTextLabel().ToStdString();
        std::string specialCategory = m_categoryChoice->GetSelection() == 0 ? std::string("") : m_categoryChoice->GetTextLabel().ToStdString();
        m_categoryChoice->Clear();
        m_categoryChoice->Append(_L("all"));
        m_categoryChoice->SetSelection(0);

        if (specialBrand.empty()) {
            for (const std::string& m : m_bundle->m_validCategorys) {
                m_categoryChoice->Append(m);
                if (specialCategory == m)
                    m_categoryChoice->SetSelection(m_categoryChoice->GetCount() - 1);
            }
            return;
        }

        auto& filaments = m_bundle->m_validCategorys;
        for (const auto& pair : *(m_bundle->m_vendors)) {
            if (pair.first != specialBrand)
                continue;

            auto v = pair.second;
            for (auto& m : v.default_filaments) {
                for (auto& n : filaments) {
                    if (n == m) {
                        m_categoryChoice->Append(n);
                        if (specialCategory == n)
                            m_categoryChoice->SetSelection(m_categoryChoice->GetCount() - 1);
                        break;
                    }
                }
            }
        }
    }

    void _syncOtherChoicesWithFilament()
    {
        std::string specialCategory = m_categoryChoice->GetSelection() == 0 ? std::string("") :
                                                                              m_categoryChoice->GetTextLabel().ToStdString();
        std::string specialBrand = m_brandChoice->GetSelection() == 0 ? std::string("") : m_brandChoice->GetTextLabel().ToStdString();
        m_brandChoice->Clear();
        m_brandChoice->Append(_L("all"));
        m_brandChoice->SetSelection(0);
        if (specialCategory.empty()) {
            for (const std::string& b : m_bundle->m_printerBrands) {
                m_brandChoice->Append(b);
                if (specialBrand == b)
                    m_brandChoice->SetSelection(m_brandChoice->GetCount() - 1);
            }
            return;
        }

        auto& brands = m_bundle->m_printerBrands;
        for (const auto& pair : *(m_bundle->m_vendors)) {
            bool isContain = false;
            for (auto& b : brands) {
                if (b == pair.first) {
                    isContain = true;
                    break;
                }
            }
            if (!isContain)
                continue;

            isContain = false;
            auto v    = pair.second;
            for (auto& m : v.default_filaments) {
                if (m == specialCategory) {
                    isContain = true;
                    break;
                }
            }
            if (isContain) {
                m_brandChoice->Append(pair.first);
                if (specialBrand == pair.first)
                    m_brandChoice->SetSelection(m_brandChoice->GetCount() - 1);
            }
        }
    }

    void _updateTree() 
    {
        m_treeCtrl->DeleteAllItems();
        m_treeIds.clear();

        if (!m_bundle)
            return;

        m_binding                   = false;

        std::string specialBrand    = m_brandChoice->GetSelection() == 0 ? std::string("") : m_brandChoice->GetTextLabel().ToStdString();
        std::string specialCategory = m_categoryChoice->GetSelection() == 0 ? std::string("") : m_categoryChoice->GetTextLabel().ToStdString();
        std::string filterName      = into_u8(m_printerSearch->GetTextCtrl()->GetValue());
        std::string pattern;
        for (int i = 0; i < filterName.size(); ++i) {
            pattern = pattern.append(std::string(".*") + filterName[i]);
        }
        if (!pattern.empty())
            pattern = pattern.append(std::string(".*"));

        auto& materials = m_bundle->m_materials;
        for (auto& pair : materials) {
            const std::string&                         name    = pair.first;
            std::vector<RelateBundle::MaterialPreset>& presets = pair.second;

            bool           hasRoot       = false;
            bool           hasUserRoot = false;
            bool           hasSystemRoot = false;
            wxDataViewItem materialRoot;
            wxDataViewItem userPreset;
            wxDataViewItem systemPreset;
            for (RelateBundle::MaterialPreset& preset : presets) {
                if (!preset.isEnabled)
                    continue;

                if (!specialBrand.empty()) {
                    auto brand = preset.brand;
                    if (specialBrand != brand)
                        continue;
                }

                if (!specialCategory.empty()) {
                    std::string category = preset.category;
                    if (category != specialCategory)
                        continue;
                }

                if (!filterName.empty()) {
                    try {
                        std::regex re(pattern);
                        if (!std::regex_match(preset.name, re))
                            continue;

                    } catch (const std::regex_error& e) {}
                }

                if (!hasRoot) {
                    hasRoot                      = true;
                    materialRoot = m_treeCtrl->AppendContainer(wxDataViewItem(nullptr), name);
                    userPreset                   = m_treeCtrl->AppendContainer(materialRoot, _L("User Preset"));
                    systemPreset                 = m_treeCtrl->AppendContainer(materialRoot, _L("System Preset"));
                }

                wxDataViewItem id;
                std::string    str = from_u8(preset.name).ToStdString();
                if (preset.isSystem) {
                    if (!hasSystemRoot) {
                        hasSystemRoot = true;
                    }
                    id = m_treeCtrl->AppendItem(systemPreset, str);
                } else {
                   if (!hasUserRoot) {
                        hasUserRoot = true;
                   }
                   id = m_treeCtrl->AppendItem(userPreset, str);
                }

                m_treeIds.push_back(id);
            }
            if (hasRoot) 
                m_treeCtrl->Expand(materialRoot);
            if (hasUserRoot)
                m_treeCtrl->Expand(userPreset);
            else
                m_treeCtrl->DeleteItem(userPreset);

            if (hasSystemRoot)
                m_treeCtrl->Expand(systemPreset);
            else
                m_treeCtrl->DeleteItem(systemPreset);
        }

        m_binding = true;
        if (!m_treeIds.empty())
            m_treeCtrl->Select(m_treeIds[0]);

        _sync();
    }

    void _sync()
    { 
         if (!m_binding)
            return;

         m_printerListCtrl->init(_L("Printer Preset Name").ToStdString(), _L("Preset Type").ToStdString());

         if (!m_bundle)
             return;
         auto                         id = m_treeCtrl->GetSelection();
         std::string                  materialName = into_u8(m_treeCtrl->GetItemText(id).ToStdString());
         RelateBundle::MaterialPreset selectedPreset;
         bool                         isFound = false;
         for (auto& pair : m_bundle->m_materials) {
             std::vector<RelateBundle::MaterialPreset>& presets = pair.second;

            for (RelateBundle::MaterialPreset& preset : presets) {
                 std::string u8Name = preset.name;
                if (u8Name == materialName) {
                    selectedPreset = preset;
                    isFound        = true;
                    break;
                }
            }
            if (isFound)
                break;
        }
         if (!isFound)
         {
            m_copyMaterialButton->Enable(false);
            m_addPrinterButton->Enable(false);
            return;
         }
         m_copyMaterialButton->Enable(true);
         m_addPrinterButton->Enable(!selectedPreset.isSystem);

         std::vector<std::pair<bool, std::string>> relatedPrinters;
         //for (auto& item : selectedPreset.relatedModels) {
         //    relatedPrinters.push_back(std::pair<bool, std::string>(item.isSystem, item.name));
         // }

         m_bundle->getFilamentsRelatedPrinters(selectedPreset.name, relatedPrinters);
        // int index = 0;
         bool isSingle = true;
         for (auto& pair : relatedPrinters) {
             if (!pair.first)
                 continue;

             _ListItem nameItem;
             initListItem(nameItem, from_u8(pair.second).ToStdString(), isSingle, true, 0);
             m_printerListCtrl->insertNameItem(nameItem);

             _ListItem typeItem;
             initListItem(typeItem, "", isSingle, true, 1);
             m_printerListCtrl->insertTypeItem(typeItem);

             isSingle = !isSingle;
         }
         for (auto& pair : relatedPrinters) {
             if (pair.first)
                continue;

             _ListItem nameItem;
             initListItem(nameItem, from_u8(pair.second).ToStdString(), isSingle, false, 0);
             m_printerListCtrl->insertNameItem(nameItem);

             _ListItem typeItem;
             initListItem(typeItem, "", isSingle, false, 1);
             m_printerListCtrl->insertTypeItem(typeItem);

             isSingle = !isSingle;
         }
    }

    void initFilterPanel(wxPanel* parent)
    {
        wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
        {
            wxStaticText* brandLabel = new wxStaticText(parent, wxID_ANY, _L("Vendor"));
            m_brandChoice = new ComboBox(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(130, 28), 0, nullptr, wxCB_READONLY);
            m_brandChoice->Bind(wxEVT_COMBOBOX, [=](auto& e) { 
                _updateTree();
                // _syncOtherChoicesWithBrand();
            });

            sizer->Add(brandLabel, 0, wxALL | wxALIGN_CENTER, 5);
            sizer->Add(m_brandChoice, 0, wxALL | wxALIGN_CENTER, 5);
        }

        {
            wxStaticText* modelLabel = new wxStaticText(parent, wxID_ANY, _L("Type"));
            m_categoryChoice = new ComboBox(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(130, 28), 0, nullptr, wxCB_READONLY);
            m_categoryChoice->Bind(wxEVT_COMBOBOX, [=](auto& e) {
                _updateTree();
                // _syncOtherChoicesWithFilament();
            });
            sizer->Add(modelLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
            sizer->Add(m_categoryChoice, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
        }


            {
            wxStaticText* printerLabel = new wxStaticText(parent, wxID_ANY, _L("Preset name"));

            m_printerSearch = new ::TextInput(parent, wxEmptyString, wxEmptyString, wxEmptyString, wxDefaultPosition, wxSize(124, 24),
                                              wxTE_PROCESS_ENTER);
            StateColor input_bg(std::pair<wxColour, int>(wxColour("#F0F0F1"), StateColor::Disabled),
                                std::pair<wxColour, int>(*wxWHITE, StateColor::Enabled));
            m_printerSearch->SetBackgroundColor(input_bg);

            m_printerSearch->GetTextCtrl()->Bind(wxEVT_TEXT, [=](wxCommandEvent& e) { _updateTree(); });
            // m_printerSearch->GetTextCtrl()->Hide();

            m_printerSearch->Bind(wxEVT_COMBOBOX, [=](auto& e) { _updateTree(); });
            sizer->Add(printerLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
            sizer->Add(m_printerSearch, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
        }

        parent->SetSizer(sizer);
    }
    void initRelatePanel(wxPanel* parent, wxWindow* parentWindow)
    {
        m_treeCtrl = new _TreeView(parent);
        m_treeCtrl->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, [this](wxCommandEvent& e) 
            { 
                _sync(); 
            });

        wxPanel* tabView = new wxPanel(parent, wxID_ANY, wxDefaultPosition, rightTabSize);
        tabView->SetBackgroundColour(wxColour(*wxWHITE));
        wxToolbook*    notebook = new wxToolbook(tabView, wxID_ANY);
        m_toolbook             = notebook;
        wxToolBarBase* toolBar  = notebook->GetToolBar();
        toolBar->Hide();
        _Tab* tab = new _Tab(tabView);
        m_tab     = tab;
        tab->appendTab(_L("Related Printer"));
        SelectedChangedFunc func = [notebook](int index) {
            notebook->SetSelection(index);
        };
        tab->bind(func);
        {
            wxPanel* relatePrinterPage = new wxPanel(notebook);
            {
                m_printerListCtrl = new _ListWidget(relatePrinterPage);
                m_printerListCtrl->SetBackgroundColour(wxColour(*wxWHITE));

                wxBoxSizer* btSizer = new wxBoxSizer(wxHORIZONTAL);
                {
                    m_addPrinterButton = new _Button(relatePrinterPage, _L("Add Related Printer"), wxSize(FromDIP(160),FromDIP(28)));
                    m_addPrinterButton->Bind(wxEVT_LEFT_DOWN, [=](wxMouseEvent& e) {
                        std::string         targetFilament = selectedFilament();
                        std::string         title          = _L("Add Related Printer").ToStdString();
                        RelatePrinterDialog dlg(parentWindow, title, targetFilament, m_bundle);
                        dlg.ShowModal();

                        if (dlg.isOk()) {
                            auto&                    printers = dlg.selected();
                            std::vector<std::string> filaments;
                            filaments.push_back(targetFilament);
                            bool done = m_bundle->isSameVendorFilament(printers, filaments);
                            if (!done) {
                                _WarningTipsDialog wdlg(parentWindow, _L("The printers are from different vendors. Please check the parameters to avoid printing failure."));
                                wdlg.ShowModal();
                                done = wdlg.isOk();
                            }

                            if (done) {
                                m_bundle->relateModel(targetFilament, printers);
                                _update();
                                select(targetFilament);
                                _sync();
                            } 
                        }
                    });

                    m_copyMaterialButton = new _Button(relatePrinterPage, _L("Copy the filament to other printers"), wxSize(FromDIP(160), FromDIP(28)));
                    m_copyMaterialButton->Bind(wxEVT_LEFT_DOWN, [=](wxMouseEvent& e) {
                        std::string         targetFilament = selectedFilament();
                        std::string         title          = _L("Copy the filament to other printers").ToStdString();
                        RelatePrinterDialog dlg(parentWindow, title, targetFilament, m_bundle);
                        dlg.ShowModal();

                        if (dlg.isOk()) {
                            auto models = dlg.selected();
                            m_bundle->copyFilamanetAndRelateModels(targetFilament, models);
                            _update();
                            select(targetFilament);
                            _sync();
                        }
                    });
                    btSizer->Add(m_copyMaterialButton, 0, wxLEFT, 0);
                    btSizer->Add(m_addPrinterButton, 0, wxLEFT, 20);
                }
                wxBoxSizer* rmSizer = new wxBoxSizer(wxVERTICAL);
                rmSizer->Add(m_printerListCtrl, 0, wxEXPAND | wxALL, 0);
                rmSizer->Add(btSizer, 0, wxALIGN_CENTER | wxTOP, 21);
                relatePrinterPage->SetSizer(rmSizer);
            }
            notebook->AddPage(relatePrinterPage, _L("Related Printer"));
        }

        tab->SetPosition(wxPoint(0, 0));
        wxBoxSizer* tabSizer = new wxBoxSizer(wxVERTICAL);
        tabSizer->Add(notebook, 0, wxTOP, 22);
        tabView->SetSizer(tabSizer);

        wxBoxSizer* pSizer = new wxBoxSizer(wxHORIZONTAL);
        pSizer->Add(m_treeCtrl, 3, wxEXPAND | wxALL, 0);
        pSizer->Add(tabView, 5, wxEXPAND | wxLEFT, 20);
        parent->SetSizer(pSizer);
    }

private:
    bool          m_binding{false};
    RelateBundle* m_bundle{NULL};

    /* 过滤区域 */
    ComboBox* m_brandChoice;
    ComboBox* m_categoryChoice;
    ::TextInput* m_printerSearch;

    /* 参数显示区域 */
    _TreeView*       m_treeCtrl;
    std::vector<wxDataViewItem> m_treeIds;
    _Button*                    m_addPrinterButton; 
    _Button*                    m_copyMaterialButton;
    _Tab*                       m_tab;

    _ListWidget* m_printerListCtrl;
    wxToolbook* m_toolbook;
};

class _MyscrolledWindow : public wxScrolledWindow {
public:
    _MyscrolledWindow(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxVSCROLL) : wxScrolledWindow(parent, id, pos, size, style) {}

    bool ShouldScrollToChildOnFocus(wxWindow* child) override { return false; }
};

void ConfigRelateDialog::set_dark_mode()
{
#ifdef __WINDOWS__
#ifdef _MSW_DARK_MODE
    NppDarkMode::SetDarkExplorerTheme(this->GetHWND());
    NppDarkMode::SetDarkTitleBar(this->GetHWND());
    wxGetApp().UpdateDlgDarkUI(this);
    SetActiveWindow(wxGetApp().mainframe->GetHWND());
    SetActiveWindow(GetHWND());
#endif
#endif
}

ConfigRelateDialog::ConfigRelateDialog(wxWindow *parent, wxWindowID id, const wxString &title, const wxPoint &pos, const wxSize &size, long style)
    : DPIDialog(parent, id, _L("Configuration Relationship Management"), pos, windowSize, style)
{
    SetBackgroundColour(*wxWHITE);
    create();
    wxGetApp().UpdateDlgDarkUI(this);
    Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
        try {
            NetworkAgent* agent = GUI::wxGetApp().getAgent();
            if (agent) {
                json j;
                std::string value;
                value = wxGetApp().app_config->get("auto_calculate");
                j["auto_flushing"] = value;
                value = wxGetApp().app_config->get("auto_calculate_when_filament_change");
                j["auto_calculate_when_filament_change"] = value;
                agent->track_event("preferences_changed", j.dump());
            }
        } catch(...) {}
        event.Skip();
        });
}

void ConfigRelateDialog::create()
{
    app_config             = get_app_config();
    m_backup_interval_time = app_config->get("backup_interval");

    m_relateBundle = new RelateBundle();

    // set icon for dialog
    std::string icon_path = (boost::format("%1%/images/Creative3DTitle.ico") % resources_dir()).str();
    SetIcon(wxIcon(encode_path(icon_path.c_str()), wxBITMAP_TYPE_ICO));
    SetSizeHints(wxDefaultSize, wxDefaultSize);

    auto main_sizer = new wxBoxSizer(wxVERTICAL);

    m_scrolledWindow = new _MyscrolledWindow(this, wxID_ANY, wxDefaultPosition, windowSize, wxVSCROLL);
    m_scrolledWindow->SetScrollRate(5, 5);

    m_sizer_body = new wxBoxSizer(wxVERTICAL);

    auto m_top_line = new wxPanel(m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxSize(windowSize.x, 1), wxTAB_TRAVERSAL);
     m_top_line->SetBackgroundColour(DESIGN_GRAY400_COLOR);
    //m_top_line->SetBackgroundColour(wxColour(255, 0, 0));

    m_sizer_body->Add(m_top_line, 0, wxEXPAND, 0);

    wxPanel*       tabView  = new wxPanel(m_scrolledWindow, wxID_ANY, wxDefaultPosition, contentSize);
    wxToolbook*    notebook = new wxToolbook(tabView, wxID_ANY);
    wxToolBarBase* toolBar = notebook->GetToolBar();
    toolBar->Hide();
    _Tab* tab = new _Tab(tabView, wxSize(FromDIP(100), FromDIP(30)));
    //tab->SetMinSize(wxSize(200, 30));
    tab->appendTab(_L("Printer"));
    tab->appendTab(_L("Filament"));
    //{
        PrinterRelatePanel* printerRelatePanel = new PrinterRelatePanel(notebook);
        MaterialRelatePanel* materialRelatePanel = new MaterialRelatePanel(notebook);
        notebook->AddPage(printerRelatePanel, _L("Printer"));
        notebook->AddPage(materialRelatePanel, _L("Filament"));

        printerRelatePanel->loadBundle(m_relateBundle);
        materialRelatePanel->loadBundle(m_relateBundle);
    //}
    tab->SetPosition(wxPoint(456, 0));
    wxBoxSizer* tabSizer = new wxBoxSizer(wxVERTICAL);
    tabSizer->Add(notebook, 0, wxTOP | wxEXPAND, FromDIP(26));
    tabView->SetSizer(tabSizer);

    SelectedChangedFunc func = [=](int index) {
       notebook->SetSelection(index);
    };
    tab->bind(func);

#if !BBL_RELEASE_TO_PUBLIC
    auto debug_page   = create_debug_page();
#endif

    m_sizer_body->Add(0, 0, 0, wxTOP, FromDIP(10));
    m_sizer_body->Add(tabView, 0, wxALL, FromDIP(10));



#if !BBL_RELEASE_TO_PUBLIC
    m_sizer_body->Add(debug_page, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(38));
#endif
    m_sizer_body->Add(0, 0, 0, wxBOTTOM, FromDIP(10));
    m_scrolledWindow->SetSizerAndFit(m_sizer_body);

    main_sizer->Add(m_scrolledWindow, 1, wxEXPAND);

    SetSizer(main_sizer);
    Layout();
    Fit();

    int screen_height = wxGetDisplaySize().GetY();
    if (this->GetSize().GetY() > screen_height)
        this->SetSize(this->GetSize().GetX() + FromDIP(40), screen_height * 4 / 5);

    CenterOnParent();
    wxPoint start_pos = this->GetPosition();
    if (start_pos.y < 0) { this->SetPosition(wxPoint(start_pos.x, 0)); }

    //select first
    auto event = wxCommandEvent(EVT_PREFERENCES_SELECT_TAB);
    event.SetInt(0);
    event.SetEventObject(this);
    wxPostEvent(this, event);

    tab->select(1);
    tab->select(0);
    printerRelatePanel->selectTab(1);
    printerRelatePanel->selectTab(0);
    materialRelatePanel->selectTab(1);
    materialRelatePanel->selectTab(0);
    Refresh();
    Update();
}

void ConfigRelateDialog::resetLayout()
{

}

ConfigRelateDialog::~ConfigRelateDialog()
{
    m_radio_group.DeleteContents(true);
    m_hash_selector.clear();
}

void ConfigRelateDialog::on_dpi_changed(const wxRect &suggested_rect) { this->Refresh(); }

void ConfigRelateDialog::Split(const std::string &src, const std::string &separator, std::vector<wxString> &dest)
{
    std::string            str = src;
    std::string            substring;
    std::string::size_type start = 0, index;
    dest.clear();
    index = str.find_first_of(separator, start);
    do {
        if (index != std::string::npos) {
            substring = str.substr(start, index - start);
            dest.push_back(substring);
            start = index + separator.size();
            index = str.find(separator, start);
            if (start == std::string::npos) break;
        }
    } while (index != std::string::npos);

    substring = str.substr(start);
    dest.push_back(substring);
}



void ConfigRelateDialog::notify_preferences_changed()
{
    if(!app_config)
        return;

    std::vector<wxString> prefs;

    wxString strlang = wxGetApp().current_language_code_safe();
    std::string country_code = app_config->get("region");
    std::string use_inches = app_config->get("use_inches");

    prefs.push_back(strlang);
    prefs.push_back(wxString(country_code));
    prefs.push_back(wxString(use_inches));

    if(Slic3r::GUI::wxGetApp().mainframe->m_webview)
    {
        Slic3r::GUI::wxGetApp().mainframe->m_webview->sync_preferences(prefs);
    }
    

}

}} // namespace Slic3r::GUI
