#include "UploadDialog.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"

namespace Slic3r { namespace GUI {

UploadDialog::UploadDialog(Plater* plater)
    : wxDialog(static_cast<wxWindow*>(wxGetApp().mainframe),
               wxID_ANY,
               "Uploading",
               wxDefaultPosition,
               wxSize(400, 200),
               wxCAPTION | wxCLOSE_BOX)
    , m_plater(plater)
    , m_timer(this)
{
    init_ui(static_cast<wxWindow*>(wxGetApp().mainframe));

    Bind(wxEVT_TIMER, &UploadDialog::OnTimeout, this);
    m_timer.Start(600000, wxTIMER_ONE_SHOT);
}

void UploadDialog::init_ui(wxWindow* parent)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    m_labelUploading = new wxStaticText(this, wxID_ANY, "Uploading");
    sizer->Add(m_labelUploading, 0, wxALIGN_CENTER | wxALL, 10);

    m_progress = new ProgressBar(this, wxID_ANY, 100, wxDefaultPosition, wxSize(250, 20));
    m_progress->SetValue(0);
    sizer->Add(m_progress, 0, wxALIGN_CENTER | wxALL, 10);

    m_labelSpeed = new wxStaticText(this, wxID_ANY, "Speed: 0 KB/s");
   // m_labelSpeed->Hide();
    sizer->Add(m_labelSpeed, 0, wxALIGN_CENTER | wxALL, 5);

    m_btnCancel = new wxButton(this, wxID_CANCEL, "Cancel Send");
    m_btnCancel->Hide();
    sizer->Add(m_btnCancel, 0, wxALIGN_CENTER | wxALL, 5);

    sizer->AddSpacer(20);

    SetSizer(sizer);
    Centre();
    wxGetApp().UpdateDlgDarkUI(this);
}

void UploadDialog::OnTimeout(wxTimerEvent& event)
{
        wxMessageBox("Upload timeout", "WARNING", wxOK | wxICON_WARNING, this);
        Destroy();
}

void UploadDialog::UpdateProgress(float percent, double speed)
{
    m_progress->SetValue(static_cast<int>(percent));
    if (speed > 0) {
        m_labelSpeed->SetLabel(wxString::Format("Speed: %.1f KB/s", speed));
        m_labelSpeed->Show();
    }

    if (percent >= 100.0f) {
        m_timer.Stop();
    }
}

}} // namespace Slic3r::GUI
