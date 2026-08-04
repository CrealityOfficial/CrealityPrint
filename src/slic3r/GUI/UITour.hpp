#pragma once
#include "ParamsDialog.hpp"
#include <wx/graphics.h>
class Button;
namespace Slic3r { namespace GUI {
class GuidePanel : public wxPanel
{
public:
    GuidePanel(wxWindow* parent);
    ~GuidePanel();
    void UpdateUI(wxRect          pos,
                  int             curStep,
                  int             totalSteps,
                  const wxString& tipsFront,
                  const wxString& tipsBack,
                  const wxString& tipsImg,
                  const wxString& littleImg,
                  wxDirection     arrowDir,
                  wxWindow*       item = nullptr);

protected:
    void OnPaint(wxPaintEvent& evt);
    void DrawArrow(wxGraphicsContext* gc, const wxPoint& start, const wxPoint& end);

private:
    wxBitmap        createScaledBitmap(const std::string& bmp_name_in, wxWindow* win, const int px_cnt, const wxSize imgSize);
    wxPoint         m_arrowTarget = wxPoint(rand() % 400, rand() % 400);
    int             m_TotalStep   = 0;
    int             m_CurStep     = 0;
    wxDirection     m_ArrowDir    = wxRIGHT;
    wxWindow*       m_TargetItem  = nullptr;
    wxString        m_TipsFront;
    wxString        m_TipsBack;
    wxString        m_Img;
    wxRect          m_Pos;
    wxStaticBitmap* m_StepBitmap                = nullptr;
    wxStaticBitmap* m_TipBitMap                 = nullptr;
    wxStaticText*   m_TipContent                = nullptr;
    wxStaticText*   m_TipContent_back           = nullptr;
    wxBoxSizer*     m_TipContentExtensionSizer   = nullptr;
    wxStaticText*   m_CurStepStatic             = nullptr;
    wxPanel*        m_MainPanel                 = nullptr;
    Button*         m_SkipBtn                   = nullptr;
    Button*         m_PreBtn                    = nullptr;
    Button*         m_NextBtn                   = nullptr;
};

class UITour : public wxWindow
{
public:
    void AddStep(wxWindow* target, const wxString& text, wxDirection arrowDir = wxRIGHT);
    void AddStep(int             index,
                 wxRect          target,
                 const wxString& frontText,
                 const wxString& backText,
                 const wxString& stepImg,
                 const wxString& littleImg,
                 wxDirection     arrowDir = wxRIGHT,
                 wxWindow*       item     = nullptr);
    void Start();
    void Previous();
    void Next();
    void End();

    UITour(wxWindow* parent = nullptr);
    ~UITour();
    
    void           RefreshRes();
    void           deleteStep(int index);
    // UITour(const UITour&)               = delete;
    // UITour& operator=(const UITour&) = delete;
    wxRect GetCurrentRect() const;
    virtual void on_dpi_changed(const wxRect& suggested_rect) {}
    void         getScreenShotCut();

private:
    static UITour& Instance();
    void OnPaint(wxPaintEvent& evt);
    void OnClick(wxMouseEvent& evt);

    struct TourStep
    {
        wxRect      rect;
        wxString    frontText;
        wxString    backText;
        wxString    guideImg;
        wxString    littleImg;
        wxDirection arrowDir;
        wxWindow*   target;
    };
    wxPoint                     m_platerPosition;
    wxBitmap                   m_platerMap;
    wxBitmap                   m_backgroundMap;
    std::map<size_t, TourStep> steps;
    size_t                     m_CurrentStep = 0;
    GuidePanel*                m_GuidePanel  = nullptr;
};
}} // namespace Slic3r::GUI
