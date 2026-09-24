#include "TextInput.hpp"
#include "Label.hpp"
#include "TextCtrl.h"
#include "slic3r/GUI/Widgets/Label.hpp"

#include <wx/dcclient.h>
#include <wx/dcgraph.h>
#include <wx/image.h>
#include <boost/log/trivial.hpp>

BEGIN_EVENT_TABLE(TextInput, wxPanel)

EVT_PAINT(TextInput::paintEvent)

END_EVENT_TABLE()

/*
 * Called by the system of by wxWidgets when the panel needs
 * to be redrawn. You can also trigger this call by
 * calling Refresh()/Update().
 */

TextInput::TextInput()
    : label_color(std::make_pair(0x909090, (int) StateColor::Disabled),
                 std::make_pair(0x6B6B6B, (int) StateColor::Normal))
    , text_color(std::make_pair(0x909090, (int) StateColor::Disabled),
                 std::make_pair(0x262E30, (int) StateColor::Normal))
{
    radius = 4;
    border_width = 1;
    border_color = StateColor(std::make_pair(0xDBDBDB, (int) StateColor::Disabled), std::make_pair(0x15BF59, (int) StateColor::Hovered),
                              std::make_pair(0xDBDBDB, (int) StateColor::Normal));
    background_color = StateColor(std::make_pair(0xF0F0F1, (int) StateColor::Disabled), std::make_pair(*wxWHITE, (int) StateColor::Normal));
    SetFont(Label::Body_12);
}

TextInput::TextInput(wxWindow *     parent,
                     wxString       text,
                     wxString       label,
                     wxString       icon,
                     const wxPoint &pos,
                     const wxSize & size,
                     long           style)
    : TextInput()
{
    Create(parent, text, label, icon, pos, size, style);
}

void TextInput::Create(wxWindow *     parent,
                       wxString       text,
                       wxString       label,
                       wxString       icon,
                       const wxPoint &pos,
                       const wxSize & size,
                       long           style)
{
        text_ctrl = nullptr;
    StaticBox::Create(parent, wxID_ANY, pos, size, style);
    wxWindow::SetLabel(label);
    style &= ~wxRIGHT;
    state_handler.attach({&label_color, & text_color});
    state_handler.update_binds();
    text_ctrl = new TextCtrl(this, wxID_ANY, text, {4, 4}, wxDefaultSize, style | wxBORDER_NONE | wxTE_PROCESS_ENTER);
    text_ctrl->SetFont(Label::Body_13);
    text_ctrl->SetInitialSize(text_ctrl->GetBestSize());
    text_ctrl->SetBackgroundColour(background_color.colorForStates(state_handler.states()));
    text_ctrl->SetForegroundColour(text_color.colorForStates(state_handler.states()));
    state_handler.attach_child(text_ctrl);
    text_ctrl->Bind(wxEVT_KILL_FOCUS, [this](auto &e) {
        // 防护：窗口正在销毁则不再转发事件
        if (this->IsBeingDeleted()) {
            wxString value_del = text_ctrl ? text_ctrl->GetValue() : wxString();
            BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " TextInput(KILL_FOCUS): window being deleted, skip (id=" << GetId() << ") value='" << value_del.ToStdString() << "'";
            e.Skip();
            return;
        }

        // OnEdit 保护性执行
        try {
            OnEdit();
        } catch (const std::exception &ex) {
            wxString value = text_ctrl ? text_ctrl->GetValue() : wxString();
            BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " TextInput(KILL_FOCUS): OnEdit threw: " << ex.what() << " value='" << value.ToStdString() << "'";
        } catch (...) {
            wxString value = text_ctrl ? text_ctrl->GetValue() : wxString();
            BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " TextInput(KILL_FOCUS): OnEdit threw unknown error value='" << value.ToStdString() << "'";
        }

        // 二次防护：若 OnEdit 导致窗口进入销毁，则不再进行本地分发
        if (this->IsBeingDeleted()) {
            wxString value = text_ctrl ? text_ctrl->GetValue() : wxString();
            BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " TextInput(KILL_FOCUS): window deleted during OnEdit, skip (id=" << GetId() << ") value='" << value.ToStdString() << "'";
            e.Skip();
            return;
        }

        // 同步本地分发，保持原有业务流程
        e.SetId(GetId());
        ProcessEventLocally(e);
        e.Skip();
    });
    text_ctrl->Bind(wxEVT_TEXT_ENTER, [this](auto &e) {
        OnEdit();
        e.SetId(GetId());
        ProcessEventLocally(e);
    });
    text_ctrl->Bind(wxEVT_RIGHT_DOWN, [this](auto &e) {}); // disable context menu
    if (!icon.IsEmpty()) {
        this->icon = ScalableBitmap(this, icon.ToStdString(), 16);
    }
    messureSize();
}

void TextInput::SetCornerRadius(double radius)
{
    this->radius = radius;
    Refresh();
}

void TextInput::SetLabel(const wxString& label)
{
    wxWindow::SetLabel(label);
    messureSize();
    Refresh();
}

void TextInput::SetMaxLength(int maxLength)
{
    text_ctrl->SetMaxLength(maxLength);
}
void TextInput::SetIconBitmapWithoutRescale(const wxBitmap& bitmap)
{
    this->icon = ScalableBitmap();
    this->icon.bmp() = bitmap;
}

void TextInput::SetIcon(const wxBitmap &icon)
{
    SetIconBitmapWithoutRescale(icon);
    Rescale();
}

void TextInput::SetIcon(const wxString &icon)
{
    if (this->icon.name() == icon.ToStdString())
        return;
    this->icon = ScalableBitmap(this, icon.ToStdString(), 16);
    Rescale();
}

void TextInput::SetLabelColor(StateColor const &color)
{
    label_color = color;
    state_handler.update_binds();
}

void TextInput::SetTextColor(StateColor const& color)
{
    text_color= color;
    state_handler.update_binds();
}

void TextInput::Rescale()
{
    if (!this->icon.name().empty()) {
        this->icon.msw_rescale();

#ifdef __WXMSW__
        // A child window may still report the previous monitor DPI while handling
        // the move event. Use the top-level window's destination scale and enforce
        // the icon's physical 16-DIP size so an old 4K arrow cannot remain at 1080p.
        if (wxWindow* top = wxGetTopLevelParent(this)) {
            const int target_height = std::max(1, int(std::lround(this->icon.px_cnt() * top->GetDPIScaleFactor())));
            wxBitmap& bitmap = this->icon.bmp();
            if (bitmap.IsOk() && bitmap.GetHeight() > 0 && bitmap.GetHeight() != target_height) {
                wxImage image = bitmap.ConvertToImage();
                const int target_width = std::max(1, int(std::lround(double(image.GetWidth()) * target_height / image.GetHeight())));
                image.Rescale(target_width, target_height, wxIMAGE_QUALITY_HIGH);
                bitmap = wxBitmap(image);
            }
            // TextInput paints in physical client coordinates; do not let bitmap
            // metadata apply an additional monitor scale during DrawBitmap().
            if (bitmap.IsOk())
                bitmap.SetScaleFactor(1.0);
        }
#endif
    }

    if (text_ctrl) {
        text_ctrl->SetFont(Label::Body_14);
        text_ctrl->InvalidateBestSize();
        wxSize text_size = text_ctrl->GetSize();
        text_size.SetHeight(text_ctrl->GetBestSize().GetHeight());
        text_ctrl->SetSize(text_size);
    }

    messureSize();
    Refresh();
}

bool TextInput::Enable(bool enable)
{
    bool result = text_ctrl->Enable(enable) && wxWindow::Enable(enable);
    if (result) {
        wxCommandEvent e(EVT_ENABLE_CHANGED);
        e.SetEventObject(this);
        GetEventHandler()->ProcessEvent(e);
        text_ctrl->SetBackgroundColour(background_color.colorForStates(state_handler.states()));
        text_ctrl->SetForegroundColour(text_color.colorForStates(state_handler.states()));
    }
    return result;
}

void TextInput::SetMinSize(const wxSize& size)
{
    wxSize resolved = size;
    if (resolved.y < 0) {
        // A default height means "fit this single-line control", not "allow
        // the sizer to collapse it". Always derive it from current DPI metrics
        // instead of retaining a physical height from the previous monitor.
        resolved.y = text_ctrl ? text_ctrl->GetBestSize().y + FromDIP(8)
                               : FromDIP(24);
    }
    wxWindow::SetMinSize(resolved);
}

void TextInput::DoSetSize(int x, int y, int width, int height, int sizeFlags)
{
    wxWindow::DoSetSize(x, y, width, height, sizeFlags);
    if (sizeFlags & wxSIZE_USE_EXISTING) return;
    wxSize size = GetSize();
    wxPoint textPos = {m_LeftMargin, 0};
    if (this->icon.bmp().IsOk()) {
        wxSize szIcon = this->icon.GetBmpSize();
        textPos.x += szIcon.x;
    }
    bool align_right = GetWindowStyle() & wxRIGHT;
    if (align_right)
        textPos.x += labelSize.x;
    if (text_ctrl) {
        wxSize textSize = text_ctrl->GetSize();
        textSize.x      = size.x - textPos.x - labelSize.x - m_LeftMargin * 2;
        text_ctrl->SetSize(textSize);
        text_ctrl->SetPosition({textPos.x, (size.y - textSize.y) / 2});
    }
}

void TextInput::DoSetToolTipText(wxString const &tip)
{
    wxWindow::DoSetToolTipText(tip);
    text_ctrl->SetToolTip(tip);
}

void TextInput::paintEvent(wxPaintEvent &evt)
{
    // depending on your system you may need to look at double-buffered dcs
    wxPaintDC dc(this);
    auto      checkDCstate = [this](const wxDC& dc, const char* caller = nullptr) -> bool {
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
        static thread_local std::mt19937_64                    eng{std::random_device{}()};
        static thread_local std::uniform_int_distribution<int> dist(0, 3); // [0,3]
        if (dist(eng) != 0) {
            return;
        }
    }
    render(dc);
}

/*
 * Here we do the actual rendering. I put it in a separate
 * method so that it can work no matter what type of DC
 * (e.g. wxPaintDC or wxClientDC) is used.
 */
void TextInput::render(wxDC& dc)
{
    StaticBox::render(dc);
    int states = state_handler.states();
    wxSize size = GetSize();
    bool   align_right = GetWindowStyle() & wxRIGHT;
    // start draw
    wxPoint pt = {5, 0};
    if (icon.bmp().IsOk()) {
        wxSize szIcon = icon.GetBmpSize();
        pt.y = (size.y - szIcon.y) / 2;
        dc.DrawBitmap(icon.bmp(), pt);
        pt.x += szIcon.x + 0;
    }
    auto text = wxWindow::GetLabel();
    if (!text.IsEmpty()) {
        wxSize textSize = text_ctrl->GetSize();
        if (align_right) {
            if (pt.x + labelSize.x > size.x)
                text = wxControl::Ellipsize(text, dc, wxELLIPSIZE_END, size.x - pt.x);
            pt.y = (size.y - labelSize.y) / 2;
        } else {
            pt.x += textSize.x;
            pt.y = (size.y + textSize.y) / 2 - labelSize.y;
        }
        dc.SetTextForeground(label_color.colorForStates(states));
        if(align_right)
            dc.SetFont(GetFont());
        else
            dc.SetFont(Label::Body_12);
        dc.DrawText(text, pt);
    }
}

void TextInput::messureSize()
{
    wxSize size = GetSize();
    wxClientDC dc(this);
    bool   align_right = GetWindowStyle() & wxRIGHT;
    if (align_right)
        dc.SetFont(GetFont());
    else
        dc.SetFont(Label::Body_12);
    labelSize = dc.GetTextExtent(wxWindow::GetLabel());

    // Recompute the single-line height from the current monitor's font on every
    // pass. Using the previous physical height as a lower bound makes fields
    // grow irreversibly after repeated mixed-DPI moves.
    const int height = text_ctrl ? text_ctrl->GetBestSize().y + FromDIP(8) : FromDIP(24);
    size.y = std::max(1, height);

    wxSize minSize(GetMinWidth(), size.y);
    wxWindow::SetMinSize(minSize);
    SetSize(size);
}
