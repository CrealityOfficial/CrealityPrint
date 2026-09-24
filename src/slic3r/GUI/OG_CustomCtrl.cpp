#include "OG_CustomCtrl.hpp"
#include "ParameterSwitchTrace.hpp"
#include "OptionsGroup.hpp"
#include "MarkdownTip.hpp"
#include "Plater.hpp"
#include "GUI_App.hpp"
#include "MsgDialog.hpp"
#include "libslic3r/AppConfig.hpp"

#include <algorithm>
#include <boost/log/trivial.hpp>
#include <iostream>
#include <ostream>
#include <wx/colour.h>
#include <wx/gdicmn.h>
#include <wx/scrolwin.h>
#include <wx/utils.h>
#include <boost/algorithm/string/split.hpp>
#include "libslic3r/Utils.hpp"
#include "I18N.hpp"
#include "format.hpp"
#include "slic3r/GUI/ParamsPanel.hpp"
//#include "wx/msw/window.h"
#include <slic3r/GUI/Widgets/Label.hpp>

namespace Slic3r { namespace GUI {

 // BBS: modify param ui style
    constexpr int titleWidth = 20;
    constexpr int ctrlWidth = 50;
#ifdef __WXOSX__
    constexpr int ctrlWidthExtra = 0;
#else
    constexpr int ctrlWidthExtra = 6;
#endif

#define DISABLE_BLINKING
#define DISABLE_UNDO_SYS

static bool is_point_in_rect(const wxPoint& pt, const wxRect& rect)
{
    return  rect.GetLeft() <= pt.x && pt.x <= rect.GetRight() &&
            rect.GetTop() <= pt.y && pt.y <= rect.GetBottom();
}

static wxSize get_bitmap_size(const wxBitmap& bmp)
{
#ifdef __APPLE__
    return bmp.GetScaledSize();
#else
    return bmp.GetSize();
#endif
}

OG_CustomCtrl::OG_CustomCtrl(   wxWindow*            parent,
                                OptionsGroup*        og,
                                const wxPoint&       pos /* = wxDefaultPosition*/,
                                const wxSize&        size/* = wxDefaultSize*/,
                                const wxValidator&   val /* = wxDefaultValidator*/,
                                const wxString&      name/* = wxEmptyString*/) :
    wxPanel(parent, wxID_ANY, pos, size, /*wxWANTS_CHARS |*/ wxBORDER_NONE | wxTAB_TRAVERSAL),
    opt_group(og)
{
    ParameterSwitchTrace trace("Ctrl.construct", this);
    trace.note("GROUP", " group=", og, " parent_window=", parent);
    if (!wxOSX)
        SetDoubleBuffered(true);// SetDoubleBuffered exists on Win and Linux/GTK, but is missing on OSX
    SetBackgroundColour(parent->GetBackgroundColour());

    // BBS: new font
    m_font = Label::Body_13;
    SetFont(m_font);
    m_em_unit   = em_unit(m_parent);
    m_v_gap   = lround(1.2 * m_em_unit);
    m_v_gap2  = lround(0.8 * m_em_unit);
    m_h_gap   = lround(0.2 * m_em_unit);

    //m_bmp_mode_sz       = get_bitmap_size(create_scaled_bitmap("mode_simple", this, wxOSX ? 10 : 12));
    m_bmp_blinking_sz   = get_bitmap_size(create_scaled_bitmap("blank_16", this));

    init_ctrl_lines();// from og.lines()

    this->Bind(wxEVT_PAINT,     &OG_CustomCtrl::OnPaint, this);
    this->Bind(wxEVT_MOTION,    &OG_CustomCtrl::OnMotion, this);
    this->Bind(wxEVT_LEFT_DOWN, &OG_CustomCtrl::OnLeftDown, this);
     this->Bind(wxEVT_LEAVE_WINDOW, &OG_CustomCtrl::OnLeaveWin, this);
}

wxCoord OG_CustomCtrl::calculate_line_height(const Line& line)
{
    wxClientDC dc(this);
    dc.SetFont(Label::Body_14);

    const std::vector<Option>& option_set = line.get_options();

    if (opt_group->split_multi_line && option_set.size() > 1) {
        const wxSize label_size = dc.GetTextExtent(line.label.AfterLast('\n'));
        return (label_size.y + m_v_gap2) * line.visible_options_count() + m_v_gap - m_v_gap2 + FromDIP(5);
    }

    wxString multiline_text;
    const int label_width = uses_leading_action_layout(line) ? get_field_layout(line).label_width :
        int(opt_group->label_width * m_em_unit);
    const wxSize label_size = Label::split_lines(dc, label_width, line.label, multiline_text);
    return label_size.y + m_v_gap + FromDIP(5);
}

OG_CustomCtrl::~OG_CustomCtrl()
{
    ParameterSwitchTrace trace("Ctrl.destruct", this);
    trace.note("STATE", " valid=", m_is_valid, " group=", opt_group, " cached_lines=", ctrl_lines.size());
}

void OG_CustomCtrl::trace_state(const char* event) const noexcept
{
    if (Slic3r::get_logging_level() < 5) return;
    ParameterSwitchTrace trace(event, this, 5);
    trace.note("STATE", " valid=", m_is_valid, " group=", opt_group, " cached_lines=", ctrl_lines.size());
    if (!m_is_valid || opt_group == nullptr) return;
    try {
        const auto& live = opt_group->get_lines();
        trace.note("GROUP", " active_ctrl=", opt_group->custom_ctrl, " live_lines=", live.size());
        std::ostringstream rows;
        for (const CtrlLine& cached : ctrl_lines) {
            const Line* address = &cached.og_line;
            const auto found = std::find_if(live.begin(), live.end(),
                [address](const Line& line) { return &line == address; });
            rows << " [line=" << address << " live=" << (found != live.end()) << " visible=" << cached.is_visible;
            if (found != live.end()) {
                for (const auto& option : found->get_options())
                    rows << " key=" << option.opt_id << " field=" << opt_group->get_field(option.opt_id);
            }
            rows << ']';
        }
        trace.note("ROWS", rows.str());
    } catch (...) {
        trace.note("SNAPSHOT_FAILED");
    }
}

void OG_CustomCtrl::init_ctrl_lines()
{
    // BBS: Add null pointer check to prevent crash when opt_group is destroyed
    if (!opt_group) {
        BOOST_LOG_TRIVIAL(error) << "OG_CustomCtrl::init_ctrl_lines: opt_group is null, cannot initialize control lines";
        return;
    }
    
    const std::vector<Line>& og_lines = opt_group->get_lines();
    for (const Line& line : og_lines)
    {
        if (line.is_separator()) {
            ctrl_lines.emplace_back(CtrlLine(0, this, line));
            continue;
        }

        if (line.full_width && (
            // description line
            line.widget != nullptr ||
            // description line with widget (button)
            !line.get_extra_widgets().empty())
            )
            continue;

        const std::vector<Option>& option_set = line.get_options();
        wxCoord height;

        // if we have a single option with no label, no sidetext just add it directly to sizer
        if (option_set.size() == 1 && opt_group->label_width == 0 && option_set.front().opt.full_width &&
            option_set.front().opt.sidetext.size() == 0 && option_set.front().side_widget == nullptr &&
            line.get_extra_widgets().size() == 0)
        {
            height = m_bmp_blinking_sz.GetHeight() + m_v_gap;
            ctrl_lines.emplace_back(CtrlLine(height, this, line, true));
        }
        else if (opt_group->label_width != 0 && (!line.label.IsEmpty() || option_set.front().opt.gui_type == ConfigOptionDef::GUIType::legend) )
        {
            height = calculate_line_height(line);
            ctrl_lines.emplace_back(CtrlLine(height, this, line, false, opt_group->staticbox));
        }
        else
            assert(false);
    }
}

int OG_CustomCtrl::get_height(const Line& line)
{
    for (auto ctrl_line : ctrl_lines)
        if (&ctrl_line.og_line == &line)
            return ctrl_line.height;
        
    return 0;
}

wxPoint OG_CustomCtrl::get_client_rect_point(const wxPoint& pos)
{
    return ClientToScreen(pos);
}

void OG_CustomCtrl::set_ctrl_widget_tooltip_binding(CtrlLine& line)
{
    if (!is_active())
        return;

    if (!line.is_visible || line.og_line.is_separator() || line.draw_just_act_buttons || line.has_right_widget_binding) 
        return;

    const std::vector<Option>& option_set = line.og_line.get_options();

    for (auto opt : option_set)
    {
        Field* field = line.ctrl->opt_group->get_field(opt.opt_id);
        if (!field)
            continue;

        if (field->getSizer())
            continue;

        wxWindow* field_win = field->getWindow();
        if (field_win)
        {
            field_win->Bind(wxEVT_ENTER_WINDOW, &OG_CustomCtrl::CtrlLine::on_ctrl_widget_enter, &line);
            field_win->Bind(wxEVT_LEAVE_WINDOW, &OG_CustomCtrl::CtrlLine::on_ctrl_widget_leave, &line);

            line.has_right_widget_binding = true;

        }

    }

}

void OG_CustomCtrl::on_destroyed()
{
    ParameterSwitchTrace trace("Ctrl.retire", this);
    trace_state("Ctrl.retire.snapshot");
    if (!m_is_valid)
        return;

    // Stop callbacks before any fields or lines are released. Repeated cleanup is harmless.
    m_is_valid = false;
    for (CtrlLine& line : ctrl_lines)
    {
        const std::vector<Option>& option_set = line.og_line.get_options();

        for (auto opt : option_set)
        {
            Field* field = line.ctrl->opt_group->get_field(opt.opt_id);
            if (!field)
                continue;

            wxWindow* field_win = field->getWindow();
            if (field_win)
            {
                field_win->Unbind(wxEVT_ENTER_WINDOW, &OG_CustomCtrl::CtrlLine::on_ctrl_widget_enter, &line);
                field_win->Unbind(wxEVT_LEAVE_WINDOW, &OG_CustomCtrl::CtrlLine::on_ctrl_widget_leave, &line);
            }
        }
    }
    // Keep CtrlLine callback receivers alive until the window is destroyed.
    // Their Line references must no longer be used after detaching the group.
    opt_group = nullptr;
}

void OG_CustomCtrl::on_ctrl_widget_enter(wxMouseEvent& event)
{
    if (!is_active())
        return;

    const wxPoint pos = event.GetLogicalPosition(wxClientDC(this));
    for (CtrlLine& line : ctrl_lines)
    {
        if (!line.is_visible) continue;
        line.is_focused = line.rect_label.GetTop() >= pos.y && pos.y >= line.rect_label.GetBottom();
        if (line.is_focused)
        {
			if (line.og_line.get_options().size() > 0)
			{
				std::string markdowntip;
				markdowntip = line.og_line.label.empty()
					? line.og_line.get_options().front().opt_id : into_u8(line.og_line.label);
				markdowntip.erase(0, markdowntip.find_last_of('#') + 1);
				if (!markdowntip.empty())
				{
					wxString tooltip;
					if (!line.og_line.label_hyperlink.empty())
						tooltip = line.og_line.label_hyperlink + "\n\n";
					tooltip += line.og_line.label_tooltip;

					wxPoint pos2 = { line.rect_label.x, line.rect_label.y + 25 };
					pos2 = ClientToScreen(pos2);
					MarkdownTip::ShowTip(markdowntip, into_u8(tooltip), pos2);
				}

				wxString img = line.og_line.label_tooltip_img;
				std::string tooltip_img;
				tooltip_img = "process/";
				tooltip_img += wxGetApp().dark_mode() ? "dark/" : "light/";
				tooltip_img += img.ToStdString();
				tooltip_img += ".svg";

				std::string url = Slic3r::var(tooltip_img);
				fs::path    ph(url);
				if (!fs::exists(ph))
					tooltip_img = "";

				if (!tooltip_img.empty())
				{
				}
			}

            break;
        }
    }

    std::cout << "on enter";
}

void OG_CustomCtrl::on_ctrl_widget_leave(wxMouseEvent& event)
{
    if (!is_active())
        return;

    std::cout << "on leave";
    MarkdownTip::ShowTip("", "", {});
}

bool OG_CustomCtrl::uses_leading_action_layout(const Line& line)
{
    const auto* group = dynamic_cast<ConfigOptionsGroup*>(opt_group);
    const auto& options = line.get_options();
    if (group == nullptr || group->config_type() != Preset::TYPE_FILAMENT ||
        options.size() != 1 || opt_group->label_width == 0 || line.label.IsEmpty() ||
        line.widget || line.near_label_widget_win || line.extra_widget_sizer ||
        options.front().side_widget || !line.get_extra_widgets().empty() || options.front().opt.full_width)
        return false;
    Field* field = opt_group->get_field(options.front().opt_id);
    return field != nullptr && field->getWindow() != nullptr && field->getSizer() == nullptr &&
        !field->has_edit_ui() && (dynamic_cast<MultiVariantField*>(field) != nullptr ||
        options.front().opt.sidetext.empty() || field->combine_side_text());
}

OG_CustomCtrl::FieldLayout OG_CustomCtrl::get_field_layout(const Line& line)
{
    const int label_x = get_title_width() * m_em_unit;
    const int label_width = line.label.IsEmpty() ? 0 :
        int(opt_group->label_width) * m_em_unit;
    const int buttons_x = label_x + (label_width > 0 ? label_width + m_h_gap : 0);
    // Keep a slot even before the child action bitmaps are initialized.
    int buttons_width = m_bmp_blinking_sz.GetWidth() + m_h_gap;
#ifndef DISABLE_UNDO_SYS
    buttons_width += m_bmp_blinking_sz.GetWidth() + m_h_gap;
#endif
#ifndef DISABLE_BLINKING
    buttons_width += m_bmp_blinking_sz.GetWidth() + m_h_gap;
#endif
    Field* field = opt_group->get_field(line.get_options().front().opt_id);
    // Reserve every action slot, including empty icons, using the same bitmaps as render().
    auto include_buttons = [&](Field* child) {
        if (child == nullptr || child->undo_bitmap() == nullptr || child->undo_to_sys_bitmap() == nullptr)
            return;
        int row_width = get_bitmap_size(child->undo_bitmap()->bmp()).GetWidth() + m_h_gap;
#ifndef DISABLE_UNDO_SYS
        row_width += get_bitmap_size(child->undo_to_sys_bitmap()->bmp()).GetWidth() + m_h_gap;
#endif
#ifndef DISABLE_BLINKING
        row_width += get_bitmap_size(create_scaled_bitmap(child->blink() ? "blank_16" : "empty", this)).GetWidth() + m_h_gap;
#endif
        buttons_width = std::max(buttons_width, row_width);
    };
    if (auto* multi = dynamic_cast<MultiVariantField*>(field)) {
        for (const auto& control : multi->controls())
            include_buttons(control.field.get());
    } else {
        include_buttons(field);
    }
    if (uses_leading_action_layout(line)) {
        // Keep the original input column; reserve actions inside the label area.
        return {label_x, std::max(1, label_width - buttons_width),
                buttons_x - buttons_width, buttons_x};
    }
    return {label_x, label_width, buttons_x, buttons_x + buttons_width};
}

wxPoint OG_CustomCtrl::get_pos(const Line& line, Field* field_in/* = nullptr*/)
{
    // BBS: new layout
    wxCoord v_pos = 0;
    wxCoord h_pos = get_title_width() * m_em_unit;

    auto correct_line_height = [](int& line_height, wxWindow* win)
    {
        if (win == nullptr)
            return;
        int win_height = win->GetSize().GetHeight();
        if (line_height < win_height)
            line_height = win_height;
    };

    auto correct_horiz_pos = [this](int& h_pos, Field* field) {
        if (m_max_win_width > 0 && field->getWindow()) {
            int win_width = field->getWindow()->GetSize().GetWidth();
            if (dynamic_cast<CheckBox*>(field))
                win_width *= 0.5;
            h_pos += m_max_win_width - win_width;
        }
    };

    auto add_label_width = [&h_pos, this](CtrlLine &ctrl_line, wxString const & label, int label_width) {
        wxClientDC dc(this);
        dc.SetFont(m_font);
        auto checkDCstate = [this](const wxDC& dc, const wxString& label, const char* caller = nullptr) -> bool {
            bool ok = true; // 整体状态：默认通过

            // ===== 1. DC State Check =====
            if (!dc.IsOk()) {
                ok = false;
                BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - Invalid DC state | Label: \"" << label << "\"";
                boost::log::core::get()->flush();
            }

            // ===== 2. Font State Check =====
            if (!dc.GetFont().IsOk()) {
                ok = false;
                BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - Invalid font | Label: \"" << label << "\"";
                boost::log::core::get()->flush();
            }

            // ===== 3. Windows HDC Check =====
#ifdef __WXMSW__
            if (dc.GetHDC() == nullptr) {
                ok             = false;
                DWORD winError = ::GetLastError(); // 必须先取错误码
                try {
                    // 核心错误码
                    BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - HDC_NULL | WinErr: " << winError << " | Label: \"" << label
                                             << "\"";

                    // Self 窗口状态
                    if (HWND selfHwnd = (HWND) GetHandle(); selfHwnd != nullptr) {
                        BOOST_LOG_TRIVIAL(error)
                            << (caller ? caller : "") << " - Self: hwnd=0x" << std::hex << selfHwnd << std::dec
                            << " | IsWindow=" << (::IsWindow(selfHwnd) ? 1 : 0) << " | IsBeingDeleted=" << this->IsBeingDeleted()
                            << " | IsShown=" << this->IsShown() << " | Label: \"" << label << "\"";
                    } else {
                        BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - Self: NULL_HANDLE | Label: \"" << label << "\"";
                    }

                    // 资源状态
                    try {
                        DWORD gdiCount  = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
                        DWORD userCount = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);
                        BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - Resources: GDI=" << gdiCount << " | USER=" << userCount
                                                 << " | Label: \"" << label << "\"";
                    } catch (...) {
                        BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - ResourceCheckFailed | Label: \"" << label << "\"";
                    }

                    // 线程验证
                    BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - Thread: current=" << GetCurrentThreadId()
                                             << " | main=" << wxThread::GetMainId() << " | Label: \"" << label << "\"";

                    boost::log::core::get()->flush();

                    // 父窗口状态
                    if (wxWindow* parent = GetParent()) {
                        if (HWND parentHwnd = (HWND) parent->GetHandle(); parentHwnd != nullptr) {
                            BOOST_LOG_TRIVIAL(error)
                                << (caller ? caller : "") << " - Parent: hwnd=0x" << std::hex << parentHwnd << std::dec
                                << " | IsWindow=" << (::IsWindow(parentHwnd) ? 1 : 0) << " | IsBeingDeleted=" << parent->IsBeingDeleted()
                                << " | IsShown=" << parent->IsShown() << " | Label: \"" << label << "\"";
                        } else {
                            BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - Parent: NULL_HANDLE | Label: \"" << label << "\"";
                        }
                    } else {
                        BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - Parent: None | Label: \"" << label << "\"";
                    }
                    boost::log::core::get()->flush();

                    // 错误描述
                    if (winError != 0) {
                        char  errBuf[256] = {0};
                        DWORD fmtRet      = ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, winError, 0,
                                                             errBuf, sizeof(errBuf) - 1, NULL);
                        if (fmtRet > 0) {
                            for (char* p = errBuf; *p; ++p)
                                if (*p == '\r' || *p == '\n')
                                    *p = ' ';
                            BOOST_LOG_TRIVIAL(error)
                                << (caller ? caller : "") << " - ErrDesc: " << errBuf << " | Label: \"" << label << "\"";
                        } else {
                            BOOST_LOG_TRIVIAL(error)
                                << (caller ? caller : "") << " - ErrDesc: [FormatMessage failed] | Label: \"" << label << "\"";
                        }
                    }

                } catch (const std::bad_alloc&) {
                    ::OutputDebugStringA("HDC_NULL_LOG: std::bad_alloc\n");
                } catch (const std::exception& e) {
                    BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - LoggingException: " << e.what() << " | Label: \"" << label
                                             << "\"";
                } catch (...) {
                    ::OutputDebugStringA("HDC_NULL_LOG: Unknown exception\n");
                }
            }
#endif

            boost::log::core::get()->flush();
            return ok; // 全部流程完成后统一返回状态
        };

        checkDCstate(dc, label, __FUNCTION__);

        wxString multiline_text;
        auto size = Label::split_lines(dc, label_width, label, multiline_text);
        if (label_width > 0) size.x = label_width;
        h_pos += size.x + m_h_gap;
        if (ctrl_line.height < size.y)
            ctrl_line.height = size.y;
    };

    auto add_buttons_width = [&h_pos, this] (int blinking_button_width) {
#ifndef DISABLE_BLINKING
#  ifndef DISABLE_UNDO_SYS
        h_pos += 3 * blinking_button_width;
#  else
        h_pos += 2 * blinking_button_width;
#  endif
#else
#  ifndef DISABLE_UNDO_SYS
        h_pos += 2 * blinking_button_width;
#  else
        h_pos += 1 * blinking_button_width;
#  endif
#endif
    };

    for (CtrlLine& ctrl_line : ctrl_lines) {
        if (&ctrl_line.og_line == &line)
        {
            // BBS: new layout
            // h_pos = m_bmp_mode_sz.GetWidth() + m_h_gap;
            if (line.near_label_widget_win) {
                wxSize near_label_widget_sz = line.near_label_widget_win->GetSize();
                if (field_in)
                    h_pos += near_label_widget_sz.GetWidth() + m_h_gap;
                else
                    break;
            }

            const std::vector<Option>& option_set = line.get_options();
            if (option_set.size() == 1) {
                Field* field = opt_group->get_field(option_set.front().opt_id);
                if (dynamic_cast<MultiVariantField*>(field) != nullptr || uses_leading_action_layout(line)) {
                    // Unit text belongs to the child control and must not select another layout.
                    h_pos = get_field_layout(line).panel_x;
                    ctrl_line.height = std::max(ctrl_line.height, calculate_line_height(line));
                    correct_line_height(ctrl_line.height, field->getWindow());
                    break;
                }
            }

            wxString label = line.label;
            if (opt_group->label_width != 0)
                add_label_width(ctrl_line, label, opt_group->label_width * m_em_unit);

            int blinking_button_width = m_bmp_blinking_sz.GetWidth() + m_h_gap;

            if (line.widget) {
#ifndef DISABLE_BLINKING
                h_pos += (line.has_undo_ui() ? 3 : 1) * blinking_button_width;
#endif

                for (auto child : line.widget_sizer->GetChildren())
                    if (child->IsWindow())
                        correct_line_height(ctrl_line.height, child->GetWindow());
                break;
            }

            if (opt_group->option_label_at_right) // BBS: position buttons at right
                add_buttons_width(blinking_button_width);

            // If we have a single option with no sidetext
            if (option_set.size() == 1 && option_set.front().opt.sidetext.size() == 0 &&
                option_set.front().side_widget == nullptr && line.get_extra_widgets().size() == 0)
            {
                // BBS: new layout
                // h_pos += 3 * blinking_button_width;
                Field* field = opt_group->get_field(option_set.front().opt_id);
                if (field == nullptr)
                    break;
                correct_line_height(ctrl_line.height, field->getWindow());
                correct_horiz_pos(h_pos, field);
                break;
            }

            bool is_multioption_line = option_set.size() > 1;
            for (auto opt : option_set) {
                if (!opt.toggle_visible)
                    continue;
                Field* field = opt_group->get_field(opt.opt_id);
                correct_line_height(ctrl_line.height, field->getWindow());

                if (!opt_group->option_label_at_right) { // BBS: position option label at right
                    ConfigOptionDef option = opt.opt;
                    // add label if any
                    if (is_multioption_line && !option.label.empty()) {
                        //!            To correct translation by context have to use wxGETTEXT_IN_CONTEXT macro from wxWidget 3.1.1
                        auto label = (option.label == L_CONTEXT("Top", "Layers") || option.label == L_CONTEXT("Bottom", "Layers")) ? _CTX(option.label, "Layers") :
                                                                                                                                     _(option.label);
                        // BBS
                        // label += ":";
                        add_label_width(ctrl_line, label, opt_group->sublabel_width * m_em_unit);
                        h_pos += 8;
                    }

                }

                if (field == field_in) {
                    correct_horiz_pos(h_pos, field);
                    break;
                }
                if (opt_group->split_multi_line) {// BBS
                    v_pos += (ctrl_line.height - m_v_gap + m_v_gap2) / std::max(size_t(1), line.visible_options_count());
                } else {
                    // BBS: new layout
                    h_pos += field->getWindow()->GetSize().x;
                    add_buttons_width(blinking_button_width);
                    if (option_set.size() == 1 && option_set.front().opt.full_width)
                        break;

                    // add sidetext if any
                    if (!field->combine_side_text() && (!opt.opt.sidetext.empty() || opt_group->sidetext_width > 0))
                        h_pos += opt_group->sidetext_width * m_em_unit + m_h_gap;

                    if (opt.opt_id != option_set.back().opt_id) //! istead of (opt != option_set.back())
                        h_pos += lround(0.6 * m_em_unit);
                }
            }
            break;
        }
        if (ctrl_line.is_visible)
            v_pos += ctrl_line.height;
    }

    return wxPoint(h_pos, v_pos);
}

// BBS: draw multi-line title
static void draw_title(wxDC& dc, wxPoint pos, const wxString& text, const wxColour* color, int width)
{
    wxString multiline_text;
    if (width > 0 && dc.GetTextExtent(text).x > width) {
        multiline_text = text;

        size_t idx = size_t(-1);
        size_t start = 0;
        for (size_t i = 0; i < multiline_text.Len(); i++)
        {
            if (multiline_text[i] == ' ')
            {
                if (dc.GetTextExtent(multiline_text.SubString(start, i)).x < width)
                    idx = i;
                else {
                    if (idx == size_t(-1))
                        idx = i;
                    multiline_text[idx] = '\n';
                    start = idx + 1;
                    idx = size_t(-1);
                }
            }
        }
        if (idx != size_t(-1))
            multiline_text[idx] = '\n';
    }

    if (!text.IsEmpty()) {
        const wxString& out_text = multiline_text.IsEmpty() ? text : multiline_text;
        wxCoord text_width, text_height;
        dc.GetMultiLineTextExtent(out_text, &text_width, &text_height);

        wxColour old_clr = dc.GetTextForeground();
        wxFont old_font = dc.GetFont();
        dc.SetTextForeground(color ? *color :
#ifdef _WIN32
            wxGetApp().get_label_clr_default());
#else
            wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
#endif /* _WIN32 */
        dc.DrawText(out_text, pos);
        dc.SetTextForeground(old_clr);
        dc.SetFont(old_font);

        if (width < 1)
            width = text_width;
    }
}

void OG_CustomCtrl::OnPaint(wxPaintEvent&)
{
    ParameterSwitchTrace trace("Ctrl.paint", this, 5);
    trace_state("Ctrl.paint.snapshot");
    // Validate the paint region even when this retired window has queued paint events.
    wxPaintDC dc(this);
    if (!is_active())
        return;

    auto checkDCstate = [this](const wxDC& dc, const char* caller = nullptr) -> bool {
        bool ok = true; // 整体状态：默认通过

        // ===== 1. DC State Check =====
        if (!dc.IsOk()) {
            ok = false;
            BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - Invalid DC state";
            boost::log::core::get()->flush();
        }

        // ===== 2. Font State Check =====
        if (!dc.GetFont().IsOk()) {
            ok = false;
            BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - Invalid font";
            boost::log::core::get()->flush();
        }

        // ===== 3. Windows HDC Check =====
#ifdef __WXMSW__
        if (dc.GetHDC() == nullptr) {
            ok             = false;
            DWORD winError = ::GetLastError(); // 必须先取错误码
            try {
                // 核心错误码
                BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - HDC_NULL | WinErr: " << winError;

                // Self 窗口状态
                if (HWND selfHwnd = (HWND) GetHandle(); selfHwnd != nullptr) {
                    BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - Self: hwnd=0x" << std::hex << selfHwnd << std::dec
                                             << " | IsWindow=" << (::IsWindow(selfHwnd) ? 1 : 0)
                                             << " | IsBeingDeleted=" << this->IsBeingDeleted() << " | IsShown=" << this->IsShown();
                } else {
                    BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - Self: NULL_HANDLE";
                }

                // 资源状态
                try {
                    DWORD gdiCount  = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
                    DWORD userCount = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);
                    BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - Resources: GDI=" << gdiCount << " | USER=" << userCount;
                } catch (...) {
                    BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - ResourceCheckFailed";
                }

                // 线程验证
                BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - Thread: current=" << GetCurrentThreadId()
                                         << " | main=" << wxThread::GetMainId();

                boost::log::core::get()->flush();

                                // 父窗口状态
                if (wxWindow* parent = GetParent()) {
                    if (HWND parentHwnd = (HWND) parent->GetHandle(); parentHwnd != nullptr) {
                        BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - Parent: hwnd=0x" << std::hex << parentHwnd << std::dec
                                                 << " | IsWindow=" << (::IsWindow(parentHwnd) ? 1 : 0)
                                                 << " | IsBeingDeleted=" << parent->IsBeingDeleted() << " | IsShown=" << parent->IsShown();
                    } else {
                        BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - Parent: NULL_HANDLE";
                    }
                } else {
                    BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - Parent: None";
                }
                boost::log::core::get()->flush();


                // 错误描述
                if (winError != 0) {
                    char  errBuf[256] = {0};
                    DWORD fmtRet = ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, winError, 0, errBuf,
                                                    sizeof(errBuf) - 1, NULL);
                    if (fmtRet > 0) {
                        for (char* p = errBuf; *p; ++p)
                            if (*p == '\r' || *p == '\n')
                                *p = ' ';
                        BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - ErrDesc: " << errBuf;
                    } else {
                        BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - ErrDesc: [FormatMessage failed]";
                    }
                }

            } catch (const std::bad_alloc&) {
                ::OutputDebugStringA("HDC_NULL_LOG: std::bad_alloc\n");
            } catch (const std::exception& e) {
                BOOST_LOG_TRIVIAL(error) << (caller ? caller : "") << " - LoggingException: " << e.what();
            } catch (...) {
                ::OutputDebugStringA("HDC_NULL_LOG: Unknown exception\n");
            }
        }
#endif

        boost::log::core::get()->flush();
        return ok; // 全部流程完成后统一返回状态
    };
    bool res = checkDCstate(dc, __FUNCTION__);
    if (!res) {
        if (!ctrl_lines.empty()) {
            // 打印第一个ctrl_line的og_line信息
            const Slic3r::GUI::Line& first_og_line = ctrl_lines.front().og_line;
            BOOST_LOG_TRIVIAL(error) << "First line label: " << first_og_line.get_options().front().opt_id;
            boost::log::core::get()->flush();
        }
        static thread_local std::mt19937_64                    eng{std::random_device{}()};
        static thread_local std::uniform_int_distribution<int> dist(0, 3); // [0,3]
        if (dist(eng) != 0) {
            return;
        }
    }

    wxCoord h_pos = get_title_width() * m_em_unit;
    wxCoord v_pos = 0;
    // BBS: new layout
    if (!GetLabel().IsEmpty()) {
        BOOST_LOG_TRIVIAL(warning) << "!GetLabel().IsEmpty().";
        dc.SetFont(Label::Head_16);
        wxColour color = StateColor::darkModeColorFor("#283436");
        draw_title(dc, {0, v_pos}, GetLabel(), &color, h_pos);
        dc.SetFont(m_font);
    }
    for (CtrlLine& line : ctrl_lines) {
        if (!line.is_visible)
            continue;
        line.render(dc, h_pos, v_pos);
        v_pos += line.height;
    }
}

void OG_CustomCtrl::OnMotion(wxMouseEvent& event)
{
    ParameterSwitchTrace trace("Ctrl.motion", this, 5);
    trace_state("Ctrl.motion.snapshot");
    if (!is_active())
        return;
    
    const wxPoint pos = event.GetLogicalPosition(wxClientDC(this));
    wxString tooltip;
    std::string tooltip_img;
    std::string markdowntip;
    wxString	tooltip_title;
    wxString      tooltip_url;
    wxString      tooltip_content;
    wxString      tooltip_key;

    // BBS: markdown tip
    CtrlLine* focusedLine = nullptr;
    wxRect focused_rect;
    // Clear the previous target even when the hit-test loop stops at an earlier line.
    for (CtrlLine& line : ctrl_lines) {
        line.is_focused = false;
        line.focused_option = -1;
    }

    for (CtrlLine& line : ctrl_lines) {
        if (!line.is_visible) continue;
        line.is_focused = !line.rect_label.IsEmpty() && is_point_in_rect(pos, line.rect_label);
        for (size_t i = 0; i < line.rects_option_label.size(); ++i) {
            if (i >= line.og_line.get_options().size() || !line.og_line.get_options()[i].toggle_visible)
                continue;
            if (!line.rects_option_label[i].IsEmpty() && is_point_in_rect(pos, line.rects_option_label[i])) {
                line.focused_option = int(i);
                break;
            }
        }
        trace.note("HIT_TEST", " line=", &line.og_line, " focused=", line.is_focused);
        if (line.is_focused || line.focused_option >= 0) {
            const auto& option_set = line.og_line.get_options();
            if (option_set.empty())
                continue;
            const bool is_option = line.focused_option >= 0;
            const size_t option_idx = is_option ? size_t(line.focused_option) : 0;
            if (option_idx >= option_set.size())
                continue;
            const Line tip_line = is_option ? opt_group->create_single_option_line(option_set[option_idx]) : line.og_line;
            focused_rect = is_option ? line.rects_option_label[option_idx] : line.rect_label;
            if (!tip_line.label_hyperlink.empty()) {
                tooltip = tip_line.label_hyperlink + "\n\n";
                tooltip_url = tip_line.label_hyperlink;
            }
            // Some grouped options (such as overhang levels) share the original line help.
            tooltip_content = tip_line.label_tooltip.IsEmpty() ? line.og_line.label_tooltip : tip_line.label_tooltip;
            tooltip += tooltip_content;
            const std::string& opt_key = option_set[option_idx].opt_id;
            trace.note("FOCUSED_KEY", " key=", opt_key, " options=", option_set.size());
            // A group summary does not describe a single parameter.
            tooltip_key = is_option || option_set.size() == 1 ? opt_key : std::string();
            tooltip_title = line.og_line.label;
            if (is_option)
                tooltip_title += " / " + tip_line.label;
            //tooltip_title = line.og_line.get_options()[0].opt_id;

            if(line.og_line.get_options().size() > 0)
            {
                wxString img = tip_line.label_tooltip_img;
          /*      tooltip_img  = "process/";
                tooltip_img += wxGetApp().dark_mode() ? "dark/" : "light/";*/
                tooltip_img += img.ToStdString();
                //tooltip_img += ".svg";
                tooltip_img += wxGetApp().dark_mode() ? "" : "_light";
                std::string url_svg = Slic3r::var(tooltip_img + ".svg");
                std::string url_png = Slic3r::var(tooltip_img + ".png");
                fs::path    phSvg(url_svg);
                fs::path    phPng(url_png);
                if(tip_line.label_tooltip_img=="detect_narrow_internal_solid_infill")
                {
                    Field* field = opt_group->get_field(tip_line.label_tooltip_img);
                    if(field)
                    {
                        try{
                            auto value =  boost::any_cast<bool>(field->get_value());
                            if(!value)
                            {
                                tooltip_img = tooltip_img + "_unchecked";
                                phPng = Slic3r::var(tooltip_img + ".png");
                                phSvg = Slic3r::var(tooltip_img + ".svg");
                            }
                        } catch(const boost::bad_any_cast &e) {
      
                        }

                    }
                }
                if (!fs::exists(phSvg) && !fs::exists(phPng))
                    tooltip_img  = "";
            }
                
            // BBS: markdown tip
            focusedLine = &line;
            markdowntip = line.og_line.label.empty() 
                ? line.og_line.get_options().front().opt_id : into_u8(line.og_line.label);
            markdowntip.erase(0, markdowntip.find_last_of('#') + 1);
            // BBS
            break;
        }else{
            // BBS: Add null pointer check to prevent crash when opt_group is destroyed
            if (!opt_group) {
                BOOST_LOG_TRIVIAL(warning) << "OG_CustomCtrl::OnMotion: opt_group is null, skipping tooltip handling";
                break;
            }
            
            try {
                ConfigOptionsGroup* config_group = dynamic_cast<ConfigOptionsGroup*>(opt_group);
                if (!config_group) {
                    BOOST_LOG_TRIVIAL(warning) << "OG_CustomCtrl::OnMotion: Failed to cast opt_group to ConfigOptionsGroup";
                    break;
                }
                
                int type = config_group->config_type();
                if(type == Preset::TYPE_PRINT || type == Preset::TYPE_PLATE || type == Preset::TYPE_MODEL)
                {
                    if(line.og_line.get_options().size() > 0)
                    {
                        Field* field = opt_group->get_field(line.og_line.get_options().front().opt_id);
                        if(field)
                        {
                            wxWindow *w = field->getWindow();
                            if(w)
                            {
                                w->UnsetToolTip();
                                for (wxWindow* c : w->GetChildren())
                                    c->UnsetToolTip();
                            }
                        }
                    }
                }
            } catch (const std::exception& e) {
                BOOST_LOG_TRIVIAL(error) << "OG_CustomCtrl::OnMotion: Exception in tooltip handling: " << e.what();
                break;
            } catch (...) {
                BOOST_LOG_TRIVIAL(error) << "OG_CustomCtrl::OnMotion: Unknown exception in tooltip handling";
                break;
            }
            
        }

        size_t undo_icons_cnt = line.rects_undo_icon.size();
        assert(line.rects_undo_icon.size() == line.rects_undo_to_sys_icon.size());
        const std::vector<Option>& option_set = line.og_line.get_options();
        
        // BBS: Add null pointer check to prevent crash when opt_group is destroyed
        if (!opt_group) {
            BOOST_LOG_TRIVIAL(warning) << "OG_CustomCtrl::OnMotion: opt_group is null, skipping undo icon handling";
            break;
        }
        
        Field* first_field = option_set.empty() ? nullptr : opt_group->get_field(option_set.front().opt_id);
        auto* multi_variant = dynamic_cast<MultiVariantField*>(first_field);
        auto field_for_icon = [&option_set, multi_variant, this](size_t opt_idx) -> Field* {
            if (multi_variant != nullptr) {
                const auto& controls = multi_variant->controls();
                return opt_idx < controls.size() ? controls[opt_idx].field.get() : nullptr;
            }
            return opt_idx < option_set.size() ? opt_group->get_field(option_set[opt_idx].opt_id) : nullptr;
        };

        for (size_t opt_idx = 0; opt_idx < undo_icons_cnt; opt_idx++) {
            if (multi_variant == nullptr &&
                (opt_idx >= option_set.size() || !option_set[opt_idx].toggle_visible))
                continue;
            Field* icon_field = field_for_icon(opt_idx);
            if (is_point_in_rect(pos, line.rects_undo_icon[opt_idx])) {
                if (Slic3r::get_logging_level() >= 5)
                    trace.note("UNDO_HIT", " index=", opt_idx, " field=", icon_field,
                               " field_tip=", icon_field ? static_cast<const void*>(icon_field->undo_tooltip()) : nullptr,
                               " line_tip=", static_cast<const void*>(line.og_line.undo_tooltip()));
                if (multi_variant == nullptr && line.og_line.has_undo_ui())
                    tooltip = *line.og_line.undo_tooltip();
                else if (icon_field != nullptr)
                    tooltip = *icon_field->undo_tooltip();
                break;
            }
            if (is_point_in_rect(pos, line.rects_undo_to_sys_icon[opt_idx])) {
                if (Slic3r::get_logging_level() >= 5)
                    trace.note("SYSTEM_UNDO_HIT", " index=", opt_idx, " field=", icon_field,
                               " field_tip=", icon_field ? static_cast<const void*>(icon_field->undo_to_sys_tooltip()) : nullptr,
                               " line_tip=", static_cast<const void*>(line.og_line.undo_to_sys_tooltip()));
                if (multi_variant == nullptr && line.og_line.has_undo_ui())
                    tooltip = *line.og_line.undo_to_sys_tooltip();
                else if (icon_field != nullptr)
                    tooltip = *icon_field->undo_to_sys_tooltip();
                break;
            }
            if (opt_idx < line.rects_edit_icon.size() && is_point_in_rect(pos, line.rects_edit_icon[opt_idx])) {
                if (icon_field != nullptr && icon_field->has_edit_ui())
                    tooltip = *icon_field->edit_tooltip();
                break;
            }
        }
        if (!tooltip.IsEmpty())
            break;
    }
    
    trace.note("TOOLTIP_LOOKUP", " focused_line=", focusedLine, " mouse_x=", pos.x, " mouse_y=", pos.y);
    ProcessTip* mdt = ProcessTip::processTip();
    trace.note("TOOLTIP_READY", " tip=", mdt);
    if(!focusedLine)
    {
        mdt->closeTip();
        Refresh();
        return;
    }
    int posW = mdt->GetSize().GetWidth();
    wxPoint      pos2 = {mdt->GetSize().GetWidth() * (-1), focused_rect.y};
    pos2 = ClientToScreen(pos2);
    wxPoint screenPos = ClientToScreen(focused_rect.GetPosition());
    mdt->setLineRect(wxRect(screenPos, focused_rect.GetSize()));
    // Set tooltips with information for each icon
    // BBS: markdown tip
    if (!markdowntip.empty()) {
        assert(focusedLine);
        

        int type = dynamic_cast<ConfigOptionsGroup*>(opt_group)->config_type();
        bool isShow = false;
        if(type == Preset::TYPE_PRINT || type == Preset::TYPE_PLATE || type == Preset::TYPE_MODEL)
        {
            isShow = ProcessTip::ShowTip(markdowntip, (tooltip_title), (tooltip_content), tooltip_key, (tooltip_img), (tooltip_url), pos2);
        }else{
            isShow = MarkdownTip::ShowTip(markdowntip, into_u8(tooltip), pos2);
        }

        if (isShow) {
            try {
                    ParamsPanel* params_panel = dynamic_cast<ParamsPanel*>(focusedLine->ctrl->GetGrandParent()->GetParent());
                    if (params_panel && params_panel->get_image_tooltip_panel()) {
                    wxGetApp().params_panel()->show_image_tooltip(tooltip_title,wxString(tooltip_img));
                    }
            } catch (...) {
                BOOST_LOG_TRIVIAL(warning) << "show image tool tip exception";
            }

            tooltip.clear();
            
        }
    }
    else {
        int type = dynamic_cast<ConfigOptionsGroup*>(opt_group)->config_type();
        bool isShow = false;
        if(type == Preset::TYPE_PRINT || type == Preset::TYPE_PLATE || type == Preset::TYPE_MODEL)
        {
            isShow = ProcessTip::ShowTip(markdowntip, (tooltip_title), (tooltip_content), tooltip_key, (tooltip_img), (tooltip_url), {tooltip.empty() ? 0 : 1, 0});
        }else{
            isShow = MarkdownTip::ShowTip(markdowntip, "", {tooltip.empty() ? 0 : 1, 0});
        }
    }

    trace.note("TOOLTIP_DONE");
    if (GetToolTipText() != tooltip)
        this->SetToolTip(tooltip);
    // BBS

    trace.note("REFRESH_BEGIN");
    Refresh();
    Update();
    trace.note("REFRESH_END");
    event.Skip();
}

void OG_CustomCtrl::OnLeftDown(wxMouseEvent& event)
{
    if (!is_active())
        return;

    const wxPoint pos = event.GetLogicalPosition(wxClientDC(this));

    for (const CtrlLine& line : ctrl_lines) {
        if (!line.is_visible) continue;
        if (line.launch_browser())
            return;
        size_t undo_icons_cnt = line.rects_undo_icon.size();
        assert(line.rects_undo_icon.size() == line.rects_undo_to_sys_icon.size());

        const std::vector<Option>& option_set = line.og_line.get_options();
        Field* first_field = option_set.empty() ? nullptr : opt_group->get_field(option_set.front().opt_id);
        auto* multi_variant = dynamic_cast<MultiVariantField*>(first_field);
        auto field_for_icon = [&option_set, multi_variant, this](size_t opt_idx) -> Field* {
            if (multi_variant != nullptr) {
                const auto& controls = multi_variant->controls();
                return opt_idx < controls.size() ? controls[opt_idx].field.get() : nullptr;
            }
            return opt_idx < option_set.size() ? opt_group->get_field(option_set[opt_idx].opt_id) : nullptr;
        };

        for (size_t opt_idx = 0; opt_idx < undo_icons_cnt; opt_idx++) {
            if (multi_variant == nullptr &&
                (opt_idx >= option_set.size() || !option_set[opt_idx].toggle_visible))
                continue;
            Field* icon_field = field_for_icon(opt_idx);
            if (is_point_in_rect(pos, line.rects_undo_icon[opt_idx])) {
                if (multi_variant != nullptr) {
                    if (icon_field != nullptr)
                        icon_field->on_back_to_initial_value();
                } else if (line.og_line.has_undo_ui()) {
                    if (ConfigOptionsGroup* conf_OG = dynamic_cast<ConfigOptionsGroup*>(line.ctrl->opt_group))
                        conf_OG->back_to_initial_value(option_set[opt_idx].opt_id);
                }
                else if (icon_field != nullptr)
                    icon_field->on_back_to_initial_value();
                event.Skip();
                return;
            }
            if (is_point_in_rect(pos, line.rects_undo_to_sys_icon[opt_idx])) {
                if (multi_variant != nullptr) {
                    if (icon_field != nullptr)
                        icon_field->on_back_to_sys_value();
                } else if (line.og_line.has_undo_ui()) {
                    if (ConfigOptionsGroup* conf_OG = dynamic_cast<ConfigOptionsGroup*>(line.ctrl->opt_group))
                        conf_OG->back_to_sys_value(option_set[opt_idx].opt_id);
                }
                else if (icon_field != nullptr)
                    icon_field->on_back_to_sys_value();
                event.Skip();
                return;
            }

            if (opt_idx < line.rects_edit_icon.size() && is_point_in_rect(pos, line.rects_edit_icon[opt_idx])) {
                if (icon_field != nullptr)
                    icon_field->on_edit_value();
                event.Skip();
                return;
            }
        }
    }
#ifndef __linux__
    SetFocusIgnoringChildren();
#endif
}

void OG_CustomCtrl::OnLeaveWin(wxMouseEvent& event)
{
    if (!is_active())
        return;

    for (CtrlLine& line : ctrl_lines) {
        line.is_focused = false;
        line.focused_option = -1;
    }

    // BBS: markdown tip
    MarkdownTip::ShowTip("", "", {});
    wxGetApp().params_panel()->show_image_tooltip("","");

    Refresh();
    Update();
    event.Skip();
}

wxCoord OG_CustomCtrl::include_multi_variant_width(const CtrlLine& line, wxCoord width)
{
    if (!line.is_visible || line.draw_just_act_buttons)
        return width;
    const auto& options = line.og_line.get_options();
    if (options.size() != 1 || !options.front().toggle_visible)
        return width;
    auto* field = dynamic_cast<MultiVariantField*>(opt_group->get_field(options.front().opt_id));
    if (field == nullptr || field->getWindow() == nullptr)
        return width;

    // These panels are positioned manually, so the group's sizer cannot measure
    // their right edge. Their minimum size already includes labels and units.
    wxWindow* panel = field->getWindow();
    const int panel_width = std::max(panel->GetSize().x, panel->GetMinSize().x);
    return std::max(width, get_field_layout(line.og_line).panel_x + panel_width + m_h_gap);
}

bool OG_CustomCtrl::update_visibility(ConfigOptionMode mode)
{
    if (!is_active())
        return false;

    // BBS: new layout
    wxCoord    h_pos = (ctrlWidth + get_title_width() - titleWidth + ctrlWidthExtra) * m_em_unit;
    wxCoord    h_pos2 = get_title_width() * m_em_unit;
    wxCoord    v_pos = 0;

    size_t invisible_lines = 0;
    for (CtrlLine& line : ctrl_lines) {
        line.update_visibility(mode);
        if (line.is_visible)
        {
            v_pos += (wxCoord)line.height;
            h_pos = include_multi_variant_width(line, h_pos);
            set_ctrl_widget_tooltip_binding(line);
        }
        else
            invisible_lines++;
    }
    // BBS: multi-line title
    SetFont(Label::Head_16);
    wxSize label_sz = GetTextExtent(GetLabel());
    SetFont(m_font);
    auto lineHeight = label_sz.y;
    while (label_sz.x > h_pos2) {
        label_sz.x -= h_pos2;
        label_sz.y += lineHeight;
    }
    if (v_pos < label_sz.y) v_pos = label_sz.y;

    //设置最小宽度
    int type = dynamic_cast<ConfigOptionsGroup*>(opt_group)->config_type();
    h_pos    = (type == Preset::TYPE_PRINT || type == Preset::TYPE_PLATE || type == Preset::TYPE_MODEL) ? h_pos : std::max(h_pos, FromDIP(700));
    this->SetMinSize(wxSize(h_pos, v_pos));
    Refresh(false); // Redraw labels/actions after changing per-option visibility.

    return invisible_lines != ctrl_lines.size();
}

void OG_CustomCtrl::update_line_height_for_field(const t_config_option_key& opt_id, bool refresh)
{
    if (!is_active())
        return;

    if (dynamic_cast<MultiVariantField*>(opt_group->get_field(opt_id)) == nullptr)
        return;
    for (CtrlLine& line : ctrl_lines) {
        const std::vector<Option>& options = line.og_line.get_options();
        if (options.size() == 1 && options.front().opt_id == opt_id) {
            line.update_multi_variant_height();
            if (refresh)
                recalculate_and_refresh();
            return;
        }
    }
}

void OG_CustomCtrl::recalculate_and_refresh()
{
    if (!is_active())
        return;

    wxCoord height = 0;
    wxCoord width = (ctrlWidth + get_title_width() - titleWidth + ctrlWidthExtra) * m_em_unit;
    const int type = dynamic_cast<ConfigOptionsGroup*>(opt_group)->config_type();
    if (type != Preset::TYPE_PRINT && type != Preset::TYPE_PLATE && type != Preset::TYPE_MODEL)
        width = std::max(width, FromDIP(700));
    for (CtrlLine& line : ctrl_lines) {
        if (!line.is_visible)
            continue;
        line.correct_items_positions();
        height += line.height;
        width = include_multi_variant_width(line, width);
    }

    const wxSize min_size(width, height);
    const bool size_changed = GetMinSize() != min_size;
    if (size_changed)
        SetMinSize(min_size);
    Refresh();
    if (!size_changed)
        return;
    if (wxWindow* parent = GetParent()) {
        parent->Layout();
        parent->Refresh();
        if (auto* scroll_window = dynamic_cast<wxScrolledWindow*>(parent))
            scroll_window->FitInside();
    }
}

// BBS: call by Tab/Page
void OG_CustomCtrl::fixup_items_positions()
{
    ParameterSwitchTrace trace("Ctrl.fixup", this, 5);
    trace_state("Ctrl.fixup.snapshot");
    if (!is_active())
        return;

    if (GetParent() == nullptr || GetPosition().y + GetSize().y < GetParent()->GetSize().y)
        return;
    for (CtrlLine& line : ctrl_lines) {
        line.correct_items_positions();
    }
}

void OG_CustomCtrl::correct_window_position(wxWindow* win, const Line& line, Field* field/* = nullptr*/)
{
    wxPoint pos = get_pos(line, field);
    int line_height = get_height(line);
    if (opt_group->split_multi_line) { // BBS
        if (line.get_options().size() > 1)
            line_height = (line_height - m_v_gap + m_v_gap2) / std::max(size_t(1), line.visible_options_count());
    }
    pos.y += std::max(0, int(0.5 * (line_height - win->GetSize().y)));
    win->SetPosition(pos);
};

void OG_CustomCtrl::correct_widgets_position(wxSizer* widget, const Line& line, Field* field/* = nullptr*/) {
    auto children = widget->GetChildren();
    wxPoint line_pos = get_pos(line, field);
    int line_height = get_height(line);
    for (auto child : children)
        if (child->IsWindow()) {
            wxPoint pos = line_pos;
            wxSize  sz = child->GetWindow()->GetSize();
            pos.y += std::max(0, int(0.5 * (line_height - sz.y)));
            if (line.extra_widget_sizer && widget == line.extra_widget_sizer)
                pos.x += m_h_gap;
            child->GetWindow()->SetPosition(pos);
            line_pos.x += sz.x + m_h_gap;
        }
};

void OG_CustomCtrl::init_max_win_width()
{
    if (opt_group->ctrl_horiz_alignment == wxALIGN_RIGHT && m_max_win_width == 0)
        for (CtrlLine& line : ctrl_lines) {
            if (int max_win_width = line.get_max_win_width();
                m_max_win_width < max_win_width)
                m_max_win_width = max_win_width;
        }
}

int OG_CustomCtrl::get_title_width()
{
    if (!GetLabel().IsEmpty())
        return titleWidth;
    else
        return 2;
}

void OG_CustomCtrl::set_max_win_width(int max_win_width)
{
    if (m_max_win_width == max_win_width)
        return;
    m_max_win_width = max_win_width;
    for (CtrlLine& line : ctrl_lines)
        line.correct_items_positions();

    GetParent()->Layout();
}


void OG_CustomCtrl::msw_rescale()
{
    if (!is_active())
        return;

#ifdef __WXOSX__
    return;
#endif
    // BBS: new font
    m_font = Label::Body_13;
    SetFont(m_font);
    m_em_unit   = em_unit(m_parent);
    m_v_gap     = lround(1.2 * m_em_unit);
    m_v_gap2     = lround(0.8 * m_em_unit);
    m_h_gap     = lround(0.2 * m_em_unit);

    //m_bmp_mode_sz = create_scaled_bitmap("mode_simple", this, wxOSX ? 10 : 12).GetSize();
    m_bmp_blinking_sz = create_scaled_bitmap("blank_16", this).GetSize();

    m_max_win_width = 0;

    wxCoord    h_pos  = (ctrlWidth + get_title_width() - titleWidth + ctrlWidthExtra) * m_em_unit;
    wxCoord    h_pos2 = get_title_width() * m_em_unit;
    wxCoord    v_pos = 0;
    for (CtrlLine& line : ctrl_lines) {
        line.msw_rescale();
        if (line.is_visible) {
            v_pos += (wxCoord)line.height;
            h_pos = include_multi_variant_width(line, h_pos);
        }
    }
    // BBS: multi-line title
    SetFont(Label::Head_16);
    wxSize label_sz = GetTextExtent(GetLabel());
    SetFont(m_font);
    auto lineHeight = label_sz.y;
    while (label_sz.x > h_pos2) {
        label_sz.x -= h_pos2;
        label_sz.y += lineHeight;
    }
    if (v_pos < label_sz.y) v_pos = label_sz.y;
    // Keep the same minimum width after DPI changes as after visibility updates.
    const int type = dynamic_cast<ConfigOptionsGroup*>(opt_group)->config_type();
    if (type != Preset::TYPE_PRINT && type != Preset::TYPE_PLATE && type != Preset::TYPE_MODEL)
        h_pos = std::max(h_pos, FromDIP(700));
    this->SetMinSize(wxSize(h_pos, v_pos));

    GetParent()->Layout();
    if (auto* scroll_window = dynamic_cast<wxScrolledWindow*>(GetParent()))
        scroll_window->FitInside();
}

void OG_CustomCtrl::sys_color_changed()
{
}

OG_CustomCtrl::CtrlLine::CtrlLine(  wxCoord         height,
                                    OG_CustomCtrl*  ctrl,
                                    const Line&     og_line,
                                    bool            draw_just_act_buttons /* = false*/,
                                    bool            draw_mode_bitmap/* = true*/):
    height(height),
    ctrl(ctrl),
    og_line(og_line),
    draw_just_act_buttons(draw_just_act_buttons),
    draw_mode_bitmap(draw_mode_bitmap)
{

    for (size_t i = 0; i < og_line.get_options().size(); i++) {
        rects_undo_icon.emplace_back(wxRect());
        rects_undo_to_sys_icon.emplace_back(wxRect());
    }
}

int OG_CustomCtrl::CtrlLine::get_max_win_width()
{
    int max_win_width = 0;
    if (!draw_just_act_buttons) {
        const std::vector<Option>& option_set = og_line.get_options();
        for (auto opt : option_set) {
            Field* field = ctrl->opt_group->get_field(opt.opt_id);
            if (field && field->getWindow())
                max_win_width = field->getWindow()->GetSize().GetWidth();
        }
    }

    return max_win_width;
}

void OG_CustomCtrl::CtrlLine::correct_items_positions()
{
    if (draw_just_act_buttons || !is_visible)
        return;

    if (og_line.near_label_widget_win)
        ctrl->correct_window_position(og_line.near_label_widget_win, og_line);
    if (og_line.widget_sizer)
        ctrl->correct_widgets_position(og_line.widget_sizer, og_line);
    if (og_line.extra_widget_sizer)
        ctrl->correct_widgets_position(og_line.extra_widget_sizer, og_line);

    const std::vector<Option>& option_set = og_line.get_options();
    for (auto opt : option_set) {
        if (!opt.toggle_visible)
            continue;
        Field* field = ctrl->opt_group->get_field(opt.opt_id);
        if (!field)
            continue;
        if (field->getSizer())
            ctrl->correct_widgets_position(field->getSizer(), og_line, field);
        else if (field->getWindow())
            ctrl->correct_window_position(field->getWindow(), og_line, field);
    }
}

void OG_CustomCtrl::CtrlLine::msw_rescale()
{
    // if we have a single option with no label, no sidetext
    if (draw_just_act_buttons)
        height = get_bitmap_size(create_scaled_bitmap("empty")).GetHeight();

    if (ctrl->opt_group->label_width != 0 && !og_line.label.IsEmpty()) {
        const std::vector<Option>& options = og_line.get_options();
        if (options.size() == 1 &&
            dynamic_cast<MultiVariantField*>(ctrl->opt_group->get_field(options.front().opt_id)) != nullptr) {
            update_multi_variant_height();
            correct_items_positions();
            return;
        }
        height = ctrl->calculate_line_height(og_line);
    }

    correct_items_positions();
}

void OG_CustomCtrl::CtrlLine::update_visibility(ConfigOptionMode mode)
{
    if (og_line.is_separator())
        return;
    const std::vector<Option>& option_set = og_line.get_options();

    const ConfigOptionMode& line_mode = option_set.front().opt.mode;
    std::string itemKey = option_set[0].opt_id;
    is_visible = og_line.toggle_visible && line_mode <= mode && og_line.visible_options_count() != 0;
    //Field* field = ctrl->opt_group->get_field(itemKey);
    //if (field)
    //    field->toggle(true);

    std::function updateItemStateByKey = [this, &itemKey, &line_mode, &mode](){
        bool isDevelopItem = Slic3r::GUI::wxGetApp().isDevelopParams(itemKey);

        if (isDevelopItem)
        {
            std::string itemType = Slic3r::GUI::wxGetApp().getDevelopParamsType(itemKey);
            if (itemType == "1")
            {
                is_visible = og_line.toggle_visible && line_mode <= mode && og_line.visible_options_count() != 0;
            }
            else if ((itemType == "2"))
            {
                is_visible = false;
            }
        }
    };

    if (!Slic3r::GUI::wxGetApp().isAlpha())
    {
        std::string factoryrMode = Slic3r::GUI::wxGetApp().app_config->get("is_factory_mode");
        bool isFactoryrMode = factoryrMode == "true";
        auto vtp = wxGetApp().preset_bundle->get_current_vendor_type();
        if (!isFactoryrMode && vtp == VendorType::Creality)
        {
            updateItemStateByKey();
        }
    }
    else
    {

    }

    if (draw_just_act_buttons)
        return;

    if (og_line.near_label_widget_win)
        og_line.near_label_widget_win->Show(is_visible);
    if (og_line.widget_sizer)
        og_line.widget_sizer->ShowItems(is_visible);
    if (og_line.extra_widget_sizer)
        og_line.extra_widget_sizer->ShowItems(is_visible);

    if (ctrl->opt_group->split_multi_line && option_set.size() > 1)
        height = ctrl->calculate_line_height(og_line);

    for (auto opt : option_set) {
        Field* field = ctrl->opt_group->get_field(opt.opt_id);
        if (!field)
            continue;
        const bool show = is_visible && opt.toggle_visible;

        if (field->getSizer()) {
            auto children = field->getSizer()->GetChildren();
            for (auto child : children)
                if (child->IsWindow())
                    child->GetWindow()->Show(show);
        }
        else if (field->getWindow())
            field->getWindow()->Show(show);
    }

    correct_items_positions();
}

void OG_CustomCtrl::CtrlLine::render_separator(wxDC& dc, wxCoord v_pos)
{
    wxPoint begin(ctrl->m_h_gap, v_pos);
    wxPoint end(ctrl->GetSize().GetWidth() - ctrl->m_h_gap, v_pos);

    wxPen old_pen = dc.GetPen();
    // pen.SetColour(*wxLIGHT_GREY);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawLine(begin, end);
    dc.SetPen(old_pen);
}

void OG_CustomCtrl::CtrlLine::render(wxDC& dc, wxCoord h_pos, wxCoord v_pos)
{
    ParameterSwitchTrace trace("Line.render", this, 5);
    trace.note("REFERENCES", " ctrl=", ctrl, " line=", &og_line);
    if (is_separator()) {
        render_separator(dc, v_pos);
        return;
    }

    if (!draw_just_act_buttons) {
        rect_label = wxRect();
        rects_option_label.assign(og_line.get_options().size(), wxRect());
    }
    Field* field = ctrl->opt_group->get_field(og_line.get_options().front().opt_id);
    trace.note("FIRST_FIELD", " key=", og_line.get_options().front().opt_id, " field=", field);

    if (auto* multi_variant = dynamic_cast<MultiVariantField*>(field)) {
        const auto& controls = multi_variant->controls();
        ensure_rects_size(controls.size());
        std::fill(rects_undo_icon.begin(), rects_undo_icon.end(), wxRect());
        std::fill(rects_undo_to_sys_icon.begin(), rects_undo_to_sys_icon.end(), wxRect());
        rects_edit_icon.clear();

        const auto layout = ctrl->get_field_layout(og_line);
        if (layout.label_width > 0) {
            wxColour blink_color = StateColor::darkModeColorFor("#15BF59");
            const wxColour* text_clr = field->blink() ? &blink_color : field->label_color();
            draw_text(dc, wxPoint(layout.label_x, v_pos), og_line.label, text_clr,
                      layout.label_width, false, true);
        }

        for (size_t variant_idx = 0; variant_idx < controls.size(); ++variant_idx) {
            const MultiVariantField::VariantControl& control = controls[variant_idx];
            Field* variant_field = control.field.get();
            if (variant_field == nullptr || control.row == nullptr ||
                variant_field->undo_to_sys_bitmap() == nullptr || variant_field->undo_bitmap() == nullptr)
                continue;

            const wxPoint row_pos = ctrl->ScreenToClient(control.row->ClientToScreen(wxPoint(0, 0)));
            const int row_height = std::max(control.row->GetSize().y, control.row->GetBestSize().y);
            const int bitmap_height = get_bitmap_size(variant_field->undo_bitmap()->bmp()).GetHeight();
            const wxPoint button_pos(layout.buttons_x, row_pos.y + std::max(0, (row_height - bitmap_height) / 2));
            if (trace_pa_reset(og_line.get_options().front().opt_id)) {
                ParameterSwitchTrace reset_trace("PAReset.render", variant_field);
                reset_trace.note("DRAW", " key=", og_line.get_options().front().opt_id,
                                 " index=", control.opt_index, " display_row=", variant_idx,
                                 " modified=", variant_field->m_is_modified_value,
                                 " nonsys=", variant_field->m_is_nonsys_value,
                                 " undo_to_sys=", og_line.undo_to_sys,
                                 " undo_icon=", variant_field->undo_bitmap()->name(),
                                 " sys_icon=", variant_field->undo_to_sys_bitmap()->name());
            }
            draw_act_bmps(dc, button_pos, variant_field->undo_to_sys_bitmap()->bmp(),
                          variant_field->undo_bitmap()->bmp(), variant_field->blink(), variant_idx, true);
        }
        return;
    }

    if (draw_just_act_buttons) {
        //BBS: GUI refactor
        if (field && field->undo_bitmap()) {
            // if (field)
            //  BBS: new layout
            const wxPoint pos = draw_act_bmps(dc, wxPoint(h_pos, v_pos), field->undo_to_sys_bitmap()->bmp(),
                                              field->undo_bitmap()->bmp(), field->blink());
            if (field->has_edit_ui())
                draw_edit_bmp(dc, pos, *field->edit_bitmap());
        }
        return;
    }

    if (og_line.near_label_widget_win)
        h_pos += og_line.near_label_widget_win->GetSize().x + ctrl->m_h_gap;

    const std::vector<Option>& option_set = og_line.get_options();

    const bool leading_actions = ctrl->uses_leading_action_layout(og_line);
    wxString label = og_line.label;
    wxColour blink_color = StateColor::darkModeColorFor("#15BF59");
    bool is_url_string = false;
    if (ctrl->opt_group->label_width != 0 && !label.IsEmpty()) {
        const wxColour* text_clr = field ? field->label_color() : og_line.label_color();
        for (const Option& opt : option_set) {
            Field* field = ctrl->opt_group->get_field(opt.opt_id);
            if (field && field->blink()) {
                text_clr = &blink_color;
                break;
            }
        }

        is_url_string = !og_line.label_hyperlink.empty();

        // BBS
        h_pos = draw_text(dc, wxPoint(h_pos, v_pos), label /* + ":" */, text_clr,
                          leading_actions ? ctrl->get_field_layout(og_line).label_width :
                          ctrl->opt_group->label_width * ctrl->m_em_unit, is_url_string, true);
    }

    if (leading_actions) {
        const auto layout = ctrl->get_field_layout(og_line);
        ensure_rects_size(1);
        rects_undo_icon[0] = wxRect();
        rects_undo_to_sys_icon[0] = wxRect();
        if (field->undo_to_sys_bitmap() != nullptr && field->undo_bitmap() != nullptr)
            draw_act_bmps(dc, wxPoint(layout.buttons_x, v_pos), field->undo_to_sys_bitmap()->bmp(),
                          field->undo_bitmap()->bmp(), field->blink());
        h_pos = layout.panel_x;
    }

    // If there's a widget, build it and set result to the correct position.
#ifndef DISABLE_BLINKING
    if (og_line.widget != nullptr) {
        draw_blinking_bmp(dc, wxPoint(h_pos, v_pos), og_line.blink);
        return;
    }
#endif

    // If we're here, we have more than one option or a single option with sidetext
    // so we need a horizontal sizer to arrange these things

    auto add_field_width = [&h_pos, &v_pos, this] (Field* field) {
        if (field) {
            if (field->getSizer())
            {
                auto children = field->getSizer()->GetChildren();
                for (auto child : children)
                    if (child->IsWindow())
                        h_pos += child->GetWindow()->GetSize().x + ctrl->m_h_gap;
            }
            else if (field->getWindow()) {
                h_pos += field->getWindow()->GetSize().x + ctrl->m_h_gap;
            }
        }
    };

    auto draw_buttons = [&h_pos, &dc, &v_pos, this, leading_actions](Field* field, size_t bmp_rect_id = 0) {
        if (leading_actions)
            return;
        if (field && field->undo_to_sys_bitmap()) {
            h_pos = draw_act_bmps(dc, wxPoint(h_pos, v_pos), field->undo_to_sys_bitmap()->bmp(), field->undo_bitmap()->bmp(), field->blink(), bmp_rect_id).x;
        }
#ifndef DISABLE_BLINKING
        else if (field && !field->undo_to_sys_bitmap() && field->blink()) 
            draw_blinking_bmp(dc, wxPoint(h_pos, v_pos), field->blink());
#endif
    };

    wxCoord h_pos2 = h_pos;
    if (field)
    {
        std::string str = field->m_opt.label;
        if (str == "Weight limit speed and acceleration" 
        || str == "Height limit speed and acceleration"
        || str == "Flow Temperature Graph")
        {
            if (option_set.front().opt.full_width && field && field->getWindow())
                field->getWindow()->SetSize(ctrl->GetSize().x - h_pos2 + h_pos - h_pos - ctrl->m_em_unit * 3, -1);
        }
    }
    // If we have a single option with no sidetext just add it directly to the grid sizer
    if (option_set.size() == 1 && option_set.front().opt.sidetext.size() == 0 &&
        option_set.front().side_widget == nullptr && og_line.get_extra_widgets().size() == 0)
    {
        // BBS: new layout
        if (ctrl->opt_group->option_label_at_right)
            draw_buttons(field);
        add_field_width(field);
        wxCoord h_pos3 = h_pos;
        if (!ctrl->opt_group->option_label_at_right)
            draw_buttons(field);
        // update width for full_width fields
        if (option_set.front().opt.full_width && field && field->getWindow())
            field->getWindow()->SetSize(ctrl->GetSize().x - h_pos2 + h_pos3 - h_pos - ctrl->m_em_unit * 3, -1);
        return;
    }

    ensure_rects_size(option_set.size());
    std::fill(rects_undo_icon.begin(), rects_undo_icon.end(), wxRect());
    std::fill(rects_undo_to_sys_icon.begin(), rects_undo_to_sys_icon.end(), wxRect());
    bool is_multioption_line = option_set.size() > 1;
    for (size_t option_idx = 0; option_idx < option_set.size(); ++option_idx) {
        const Option& opt = option_set[option_idx];
        if (!opt.toggle_visible)
            continue;
        field = ctrl->opt_group->get_field(opt.opt_id);
        trace.note("FIELD", " key=", opt.opt_id, " field=", field);
        ConfigOptionDef option = opt.opt;
        if (ctrl->opt_group->option_label_at_right)
            draw_buttons(field, option_idx);
        if (ctrl->opt_group->option_label_at_right) // BBS
            add_field_width(field);
        // add label if any
        if (is_multioption_line && !option.label.empty()) {
            //!            To correct translation by context have to use wxGETTEXT_IN_CONTEXT macro from wxWidget 3.1.1
            label = (option.label == L_CONTEXT("Top", "Layers") || option.label == L_CONTEXT("Bottom", "Layers")) ?
                    _CTX(option.label, "Layers") : _(option.label);
            //if (!ctrl->opt_group->option_label_at_right) // BBS
                //label += ":";

            if (is_url_string)
                is_url_string = false;
            else if(opt == option_set.front())
                is_url_string = !og_line.label_path.empty();
            wxColor c = StateColor::darkModeColorFor("#6B6B6B");
            h_pos = draw_text(dc, wxPoint(h_pos, v_pos), label, field ? (field->blink() ? &blink_color : &c) : nullptr, ctrl->opt_group->sublabel_width * ctrl->m_em_unit, is_url_string, false, &rects_option_label[option_idx]);
            h_pos += 8;
        }

        // BBS: new layout
        // add field
        if (option_set.size() == 1 && option_set.front().opt.full_width)
            break;
        if (!ctrl->opt_group->option_label_at_right) // BBS
            add_field_width(field);
        // add sidetext if any
        // BBS: new layout
        wxCoord offset = 0;
        if (!field->combine_side_text() && (!option.sidetext.empty() || ctrl->opt_group->sidetext_width > 0)) {
            wxCoord h_pos2 = h_pos + dc.GetTextExtent(_(option.sidetext)).x;
            h_pos = draw_text(dc, wxPoint(h_pos, v_pos), _(option.sidetext), nullptr, ctrl->opt_group->sidetext_width * ctrl->m_em_unit);
            offset = h_pos - h_pos2;
        }
        // BBS: new layout
        if (!ctrl->opt_group->option_label_at_right) {
            offset -= ctrl->m_h_gap; h_pos -= offset;
            draw_buttons(field, option_idx);
            h_pos += offset;
        }

        if (opt.opt_id != option_set.back().opt_id) //! istead of (opt != option_set.back())
            h_pos += lround(0.6 * ctrl->m_em_unit);

        if (ctrl->opt_group->split_multi_line) { // BBS
            v_pos += (height - ctrl->m_v_gap + ctrl->m_v_gap2) / std::max(size_t(1), og_line.visible_options_count());
            h_pos = h_pos2;
        }
    }
    
}

wxCoord OG_CustomCtrl::CtrlLine::draw_text(wxDC &dc, wxPoint pos, const wxString &text, const wxColour *color, int width, bool is_url/* = false*/, bool is_main/* = false*/, wxRect* text_rect/* = nullptr*/)
{
    const wxFont old_font = dc.GetFont();
    dc.SetFont(Label::Body_14);

    wxString multiline_text;
    auto size = Label::split_lines(dc, width, text, multiline_text);

    if (!text.IsEmpty()) {
        const wxString& out_text = multiline_text.IsEmpty() ? text : multiline_text;

        if (ctrl->opt_group->split_multi_line && !is_main) { // BBS
            const std::vector<Option> &option_set = og_line.get_options();
            pos.y = pos.y + lround(((height - ctrl->m_v_gap + ctrl->m_v_gap2) / std::max(size_t(1), og_line.visible_options_count()) - size.y) / 2);
        } else {
            pos.y = pos.y + lround((height - size.y) / 2);
        }
        const wxRect bounds(pos, wxSize(size.x, size.y));
        if (is_main)
            rect_label = bounds;
        if (text_rect != nullptr)
            *text_rect = bounds;

        wxColour old_clr = dc.GetTextForeground();
        wxColor clr_url = StateColor::darkModeColorFor("#15BF59");
        if ((is_main && is_focused) || (text_rect != nullptr && focused_option >= 0 &&
            size_t(focused_option) < rects_option_label.size() && text_rect == &rects_option_label[size_t(focused_option)])) {
        // temporary workaround for the OSX because of strange Bold font behavior on BigSerf
#ifdef __APPLE__
            dc.SetFont(old_font.Underlined());
#else
            //dc.SetFont(old_font.Bold().Underlined());
#endif            
            color = &clr_url;
        }
        dc.SetTextForeground(color ? *color :
#ifdef _WIN32
            wxGetApp().get_label_clr_default());
#else
            wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
#endif /* _WIN32 */
        dc.DrawText(out_text, pos);
        dc.SetTextForeground(old_clr);

        if (width < 1)
            width = size.x;
    }

    dc.SetFont(old_font);
    return pos.x + width + ctrl->m_h_gap;
}

wxPoint OG_CustomCtrl::CtrlLine::draw_blinking_bmp(wxDC& dc, wxPoint pos, bool is_blinking)
{
    wxBitmap bmp_blinking = create_scaled_bitmap(is_blinking ? "blank_16" : "empty", ctrl);
    wxCoord h_pos = pos.x;
    wxCoord v_pos = pos.y + lround((height - get_bitmap_size(bmp_blinking).GetHeight()) / 2);

    dc.DrawBitmap(bmp_blinking, h_pos, v_pos);

    int bmp_dim = get_bitmap_size(bmp_blinking).GetWidth();

    h_pos += bmp_dim + ctrl->m_h_gap;
    return wxPoint(h_pos, v_pos);
}

wxPoint OG_CustomCtrl::CtrlLine::draw_act_bmps(wxDC& dc, wxPoint pos, const wxBitmap& bmp_undo_to_sys, const wxBitmap& bmp_undo, bool is_blinking, size_t rect_id, bool skip_vertical_adjust)
{
#ifndef DISABLE_BLINKING
    pos = draw_blinking_bmp(dc, pos, is_blinking);
#else
    if (!skip_vertical_adjust) {
        if (ctrl->opt_group->split_multi_line) { // BBS
            const std::vector<Option> &option_set = og_line.get_options();
            if (option_set.size() > 1)
                pos.y += lround(((height - ctrl->m_v_gap + ctrl->m_v_gap2) / std::max(size_t(1), og_line.visible_options_count()) - get_bitmap_size(bmp_undo).GetHeight()) / 2);
            else
                pos.y += lround((height - get_bitmap_size(bmp_undo).GetHeight()) / 2);
        } else {
            pos.y += lround((height - get_bitmap_size(bmp_undo).GetHeight()) / 2);
        }
    }
#endif
    wxCoord h_pos = pos.x;
    wxCoord v_pos = pos.y;

#ifndef DISABLE_UNDO_SYS
    //BBS: GUI refactor
    dc.DrawBitmap(bmp_undo_to_sys, h_pos, v_pos);

    int bmp_dim = get_bitmap_size(bmp_undo_to_sys).GetWidth();
    rects_undo_to_sys_icon[rect_id] = wxRect(h_pos, v_pos, bmp_dim, bmp_dim);

    h_pos += bmp_dim + ctrl->m_h_gap;
#endif
    dc.DrawBitmap(og_line.undo_to_sys ? bmp_undo_to_sys : bmp_undo, h_pos, v_pos);

    int bmp_dim2 = get_bitmap_size(bmp_undo).GetWidth();
    (og_line.undo_to_sys ? rects_undo_to_sys_icon[rect_id] : rects_undo_icon[rect_id]) = wxRect(h_pos, v_pos, bmp_dim2, bmp_dim2);

    h_pos += bmp_dim2 + ctrl->m_h_gap;

    return wxPoint(h_pos, v_pos);
}

void OG_CustomCtrl::CtrlLine::ensure_rects_size(size_t size)
{
    rects_undo_icon.resize(size, wxRect());
    rects_undo_to_sys_icon.resize(size, wxRect());
}

wxCoord OG_CustomCtrl::CtrlLine::draw_edit_bmp(wxDC &dc, wxPoint pos, const wxBitmap& bmp_edit)
{
    const wxCoord h_pos = pos.x + ctrl->m_h_gap;
    const wxCoord v_pos = pos.y;
    const int bmp_w = bmp_edit.GetWidth();
    rects_edit_icon.emplace_back(wxRect(h_pos, v_pos, bmp_w, bmp_w));

    dc.DrawBitmap(bmp_edit, h_pos, v_pos);

    return h_pos + bmp_w + ctrl->m_h_gap;
}

bool OG_CustomCtrl::CtrlLine::launch_browser() const
{
    return false;
    if (!is_focused || og_line.label_hyperlink.empty())
        return false;

    return OptionsGroup::launch_browser(og_line.label_hyperlink);
}

void OG_CustomCtrl::CtrlLine::on_ctrl_widget_enter(wxMouseEvent& event)
{
    ParameterSwitchTrace trace("Line.mouse_enter", this, 5);
    trace.note("REFERENCES", " ctrl=", ctrl, " line=", &og_line);
    if (ctrl != nullptr) ctrl->trace_state("Line.mouse_enter.snapshot");
    if (ctrl == nullptr || !ctrl->is_active())
        return;

    if (this->og_line.get_options().size() > 0)
    {
        wxString tooltip;
        std::string markdowntip;
        wxString	tooltip_title = this->og_line.label;
        markdowntip = this->og_line.label.empty()
            ? this->og_line.get_options().front().opt_id : into_u8(this->og_line.label);
        markdowntip.erase(0, markdowntip.find_last_of('#') + 1);
        if (!markdowntip.empty()) 
        {
            if (!this->og_line.label_hyperlink.empty())
                tooltip = this->og_line.label_hyperlink + "\n\n";
            tooltip += this->og_line.label_tooltip;

            wxPoint pos2 = { this->rect_label.x, this->rect_label.y + 25 };
            pos2 = this->ctrl->get_client_rect_point(pos2);

            int type = dynamic_cast<ConfigOptionsGroup*>(this->ctrl->opt_group)->config_type();
            if(!(type == Preset::TYPE_PRINT || type == Preset::TYPE_PLATE || type == Preset::TYPE_MODEL))
            {
                MarkdownTip::ShowTip(markdowntip, into_u8(tooltip), pos2);
            }
            
        }

        wxString img = this->og_line.label_tooltip_img;
        std::string tooltip_img;
        tooltip_img = "process/";
        tooltip_img += wxGetApp().dark_mode() ? "dark/" : "light/";
        tooltip_img += img.ToStdString();
        tooltip_img += ".svg";

        std::string url = Slic3r::var(tooltip_img);
        fs::path    ph(url);
        if (!fs::exists(ph))
            tooltip_img = "";

        try {
                ParamsPanel* params_panel = dynamic_cast<ParamsPanel*>(this->ctrl->GetGrandParent()->GetParent());
                if (params_panel && params_panel->get_image_tooltip_panel()) {
                wxGetApp().params_panel()->show_image_tooltip(tooltip_title,wxString(tooltip_img));
            }
        } catch (...) {
            BOOST_LOG_TRIVIAL(warning) << "show image tool tip exception";
        }

    }

    // event.Skip();
    // this->ctrl->Refresh();
    // this->ctrl->Update();
    
}

void OG_CustomCtrl::CtrlLine::on_ctrl_widget_leave(wxMouseEvent& event)
{
    ParameterSwitchTrace trace("Line.mouse_leave", this, 5);
    trace.note("REFERENCES", " ctrl=", ctrl, " line=", &og_line);
    if (ctrl == nullptr || !ctrl->is_active())
        return;

    // BBS: markdown tip
    std::cout << "on leave";
    //MarkdownTip::ShowTip("", "", {});

    // event.Skip();
    // this->ctrl->Refresh();
    // this->ctrl->Update();
    
}

void OG_CustomCtrl::CtrlLine::update_multi_variant_height()
{
    const std::vector<Option>& options = og_line.get_options();
    if (options.size() != 1)
        return;
    auto* field = dynamic_cast<MultiVariantField*>(ctrl->opt_group->get_field(options.front().opt_id));
    if (field == nullptr || field->getWindow() == nullptr)
        return;

    const int label_height = ctrl->calculate_line_height(og_line);
    const int field_height = std::max(field->getWindow()->GetSize().y, field->getWindow()->GetBestSize().y);
    height = std::max(label_height, field_height + ctrl->m_v_gap);
}

} // GUI
} // Slic3r
