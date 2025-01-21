#ifndef slic3r_calib_dlg_hpp_
#define slic3r_calib_dlg_hpp_

#include "wxExtensions.hpp"
#include "GUI_Utils.hpp"
#include "Widgets/RadioBox.hpp"
#include "Widgets/Button.hpp"
#include "Widgets/RoundedRectangle.hpp"
#include "Widgets/Label.hpp"
#include "Widgets/CheckBox.hpp"
#include "Widgets/ComboBox.hpp"
#include "Widgets/TextInput.hpp"
#include "GUI_App.hpp"
#include "wx/hyperlink.h"
#include <wx/radiobox.h>
#include "libslic3r/calib.hpp"

namespace Slic3r { namespace GUI {

class PA_Calibration_Dlg : public DPIDialog
{
public:
    PA_Calibration_Dlg(wxWindow* parent, wxWindowID id, Plater* plater);
    ~PA_Calibration_Dlg();
    void on_dpi_changed(const wxRect& suggested_rect) override;
	void on_show(wxShowEvent& event);

protected:
    void reset_params();
	virtual void on_start(wxCommandEvent& event);
	virtual void on_extruder_type_changed(wxCommandEvent& event);
	virtual void on_method_changed(wxCommandEvent& event);

protected:
	bool m_bDDE;
	Calib_Params m_params;


	wxRadioBox* m_rbExtruderType;
	wxRadioBox* m_rbMethod;
	TextInput* m_tiStartPA;
	TextInput* m_tiEndPA;
	TextInput* m_tiPAStep;
    TextInput* m_tiBedTemp;
	CheckBox* m_cbPrintNum;
	Button* m_btnStart;
    int         m_bedTempValue;
    int         iExtruderTypeSeletion;
    int         imethod;

	Plater* m_plater;
};

class Temp_Calibration_Dlg : public DPIDialog
{
public:
    Temp_Calibration_Dlg(wxWindow* parent, wxWindowID id, Plater* plater);
    ~Temp_Calibration_Dlg();
    void on_dpi_changed(const wxRect& suggested_rect) override;

protected:
    virtual void on_start(wxCommandEvent& event);
    virtual void on_filament_type_changed(wxCommandEvent& event);
    Calib_Params m_params;

    wxRadioBox* m_rbFilamentType;
    TextInput* m_tiStart;
    TextInput* m_tiEnd;
    TextInput* m_tiStep;
    TextInput*  m_tiBedTemp;
    Button* m_btnStart;
    Plater* m_plater;
};

class MaxVolumetricSpeed_Test_Dlg : public DPIDialog
{
public:
    MaxVolumetricSpeed_Test_Dlg(wxWindow* parent, wxWindowID id, Plater* plater);
    ~MaxVolumetricSpeed_Test_Dlg();
    void on_dpi_changed(const wxRect& suggested_rect) override;
    void on_show(wxShowEvent& event);
protected:
    void         update_params();
    virtual void on_start(wxCommandEvent& event);
    Calib_Params m_params;

    TextInput* m_tiStart;
    TextInput* m_tiEnd;
    TextInput* m_tiStep;
    TextInput* m_tiBedTemp;
    Button* m_btnStart;
    Plater* m_plater;
    int        m_bedTempValue;
};

class VFA_Test_Dlg : public DPIDialog {
public:
    VFA_Test_Dlg(wxWindow* parent, wxWindowID id, Plater* plater);
    ~VFA_Test_Dlg();
    void on_dpi_changed(const wxRect& suggested_rect) override;
    void on_show(wxShowEvent& event);

protected:
    void         update_params();
    virtual void on_start(wxCommandEvent& event);
    Calib_Params m_params;

    TextInput* m_tiStart;
    TextInput* m_tiEnd;
    TextInput* m_tiStep;
    TextInput* m_tiBedTemp;
    Button* m_btnStart;
    Plater* m_plater;
    int        m_bedTempValue;
};


class Retraction_Test_Dlg : public DPIDialog
{
public:
    Retraction_Test_Dlg (wxWindow* parent, wxWindowID id, Plater* plater);
    ~Retraction_Test_Dlg ();
    void on_dpi_changed(const wxRect& suggested_rect) override;
    void on_show(wxShowEvent& event);

protected:
    void         update_params();
    virtual void on_start(wxCommandEvent& event);
    Calib_Params m_params;

    TextInput* m_tiStart;
    TextInput* m_tiEnd;
    TextInput* m_tiStep;
    TextInput* m_tiBedTemp;
    Button* m_btnStart;
    Plater* m_plater;
    int        m_bedTempValue;
};

class Retraction_Speed_Dlg : public DPIDialog
{
public:
    Retraction_Speed_Dlg(wxWindow* parent, wxWindowID id, Plater* plater);
    ~Retraction_Speed_Dlg();
    void on_dpi_changed(const wxRect& suggested_rect) override;
    void on_show(wxShowEvent& event);

protected:
    void         update_params();
    virtual void on_start(wxCommandEvent& event);
    Calib_Params m_params;

    TextInput* m_tiStart;
    TextInput* m_tiEnd;
    TextInput* m_tiStep;
    TextInput* m_tiBedTemp;
    Button*    m_btnStart;
    Plater*    m_plater;
    int        m_bedTempValue;
};


class High_Flowrate_Dlg : public DPIDialog
{
public:
    High_Flowrate_Dlg(wxWindow* parent, wxWindowID id, Plater* plater);
    ~High_Flowrate_Dlg();
    void on_dpi_changed(const wxRect& suggested_rect) override;

protected:
    //virtual void on_start(wxCommandEvent& event);
    void         on_start0(wxCommandEvent& event);
    void         on_start1(wxCommandEvent& event);
    void         on_start2(wxCommandEvent& event);
    void         on_start3(wxCommandEvent& event);
    void         on_start4(wxCommandEvent& event);
    void         on_start5(wxCommandEvent& event);
    void         on_start6(wxCommandEvent& event);
    void         on_start7(wxCommandEvent& event);
    void         on_start8(wxCommandEvent& event);
    Calib_Params m_params;
    wxButton*      m_btn0;
    wxButton*    m_btn1;
    wxButton*      m_btn2;
    wxButton*      m_btn3;
    wxButton*      m_btn4;
    wxButton*      m_btn5;
    wxButton*      m_btn6;
    wxButton*      m_btn7;
    wxButton*      m_btn8;
    //Button*      m_btn8;
    Plater*      m_plater;
};

class Limit_Speed_Dlg : public DPIDialog
{
public:
    Limit_Speed_Dlg(wxWindow* parent, wxWindowID id, Plater* plater);
    ~Limit_Speed_Dlg();
    void on_dpi_changed(const wxRect& suggested_rect) override;
    void on_show(wxShowEvent& event);

protected:
    void         update_params();
    virtual void on_start(wxCommandEvent& event);
    Calib_Params m_params;

    TextInput* m_tiStart;
    TextInput* m_tiEnd;
    TextInput* m_tiStep;
    TextInput* m_tiBedTemp;
    TextInput* m_tiAccSpeed;
    Button*    m_btnStart;
    Plater*    m_plater;
    int        m_bedTempValue;
};

class Speed_Tower_Dlg : public DPIDialog
{
public:
    Speed_Tower_Dlg(wxWindow* parent, wxWindowID id, Plater* plater);
    ~Speed_Tower_Dlg();
    void on_dpi_changed(const wxRect& suggested_rect) override;
    void on_show(wxShowEvent& event);

protected:
    virtual void on_start(wxCommandEvent& event);
    void         update_params();
    Calib_Params m_params;

    TextInput* m_tiStart;
    TextInput* m_tiEnd;
    TextInput* m_tiStep;
    TextInput* m_tiBedTemp;
    //TextInput* m_tiAccSpeed;
    Button*    m_btnStart;
    Plater*    m_plater;
    int        m_bedTempValue;
};

class Jitter_Speed_Dlg : public DPIDialog
{
public:
    Jitter_Speed_Dlg(wxWindow* parent, wxWindowID id, Plater* plater);
    ~Jitter_Speed_Dlg();
    void on_dpi_changed(const wxRect& suggested_rect) override;
    void on_show(wxShowEvent& event);

protected:
    void         update_params();
    virtual void on_start(wxCommandEvent& event);
    Calib_Params m_params;

    TextInput* m_tiStart;
    TextInput* m_tiEnd;
    TextInput* m_tiStep;
    TextInput* m_tiBedTemp;
    // TextInput* m_tiAccSpeed;
    Button* m_btnStart;
    Plater* m_plater;
    int     m_bedTempValue;
};

class Fan_Speed_Dlg : public DPIDialog
{
public:
    Fan_Speed_Dlg(wxWindow* parent, wxWindowID id, Plater* plater);
    ~Fan_Speed_Dlg();
    void on_dpi_changed(const wxRect& suggested_rect) override;
    void on_show(wxShowEvent& event);

protected:
    void         update_params();
    virtual void on_start(wxCommandEvent& event);
    Calib_Params m_params;

    TextInput* m_tiStart;
    TextInput* m_tiEnd;
    TextInput* m_tiStep;
    TextInput* m_tiBedTemp;
    // TextInput* m_tiAccSpeed;
    Button* m_btnStart;
    Plater* m_plater;
    int     m_bedTempValue;
};

class Limit_Acceleration_Dlg : public DPIDialog
{
public:
    Limit_Acceleration_Dlg(wxWindow* parent, wxWindowID id, Plater* plater);
    ~Limit_Acceleration_Dlg();
    void on_dpi_changed(const wxRect& suggested_rect) override;
    void on_show(wxShowEvent& event);

protected:
    void         update_params();
    virtual void on_start(wxCommandEvent& event);
    Calib_Params m_params;

    TextInput* m_tiStart;
    TextInput* m_tiEnd;
    TextInput* m_tiStep;
    TextInput* m_tiBedTemp;
    TextInput* m_tiSpeed;
    Button* m_btnStart;
    Plater* m_plater;
    int        m_bedTempValue;
};


class Acceleration_Tower_Dlg : public DPIDialog
{
public:
    Acceleration_Tower_Dlg(wxWindow* parent, wxWindowID id, Plater* plater);
    ~Acceleration_Tower_Dlg();
    void on_dpi_changed(const wxRect& suggested_rect) override;
    void on_show(wxShowEvent& event);

protected:
    void         update_params();
    virtual void on_start(wxCommandEvent& event);
    Calib_Params m_params;

    TextInput* m_tiStart;
    TextInput* m_tiEnd;
    TextInput* m_tiStep;
    TextInput* m_tiBedTemp;
    //TextInput* m_tiSpeed;
    Button*    m_btnStart;
    Plater*    m_plater;
    int        m_bedTempValue;
};



class Dec_Acceleration_Dlg : public DPIDialog
{
public:
    Dec_Acceleration_Dlg(wxWindow* parent, wxWindowID id, Plater* plater);
    ~Dec_Acceleration_Dlg();
    void on_dpi_changed(const wxRect& suggested_rect) override;
    void on_show(wxShowEvent& event);

protected:
    void         update_params();
    virtual void on_start(wxCommandEvent& event);
    Calib_Params m_params;

    TextInput* m_tiStart;
    TextInput* m_tiEnd;
    TextInput* m_tiStep;
    TextInput* m_tiBedTemp;
    TextInput* m_tiHstep;
    Button* m_btnStart;
    Plater* m_plater;
    int        m_bedTempValue;
};

}} // namespace Slic3r::GUI

#endif
