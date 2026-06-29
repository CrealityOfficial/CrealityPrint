#pragma once
#include <wx/wx.h>
#include "Widgets/ProgressBar.hpp"

namespace Slic3r { namespace GUI {

class Plater; // 前置声明

class UploadDialog : public wxDialog
{
public:
    explicit UploadDialog(Plater* plater);
    explicit UploadDialog(wxWindow* parent);

    void UpdateProgress(float percent, double speed);
    void ShowCancelButton(bool show);

private:
    Plater*       m_plater{nullptr};
    wxStaticText* m_labelUploading{nullptr};
    ProgressBar*  m_progress{nullptr};
    wxStaticText* m_labelSpeed{nullptr};
    wxButton*     m_btnCancel{nullptr};
    wxTimer       m_timer;
    void          OnTimeout(wxTimerEvent& event);

    void init_ui(wxWindow* parent);
};

}} // namespace Slic3r::GUI
