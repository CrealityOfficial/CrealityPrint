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
#include <array>
#include "libslic3r/calib.hpp"

class ImgBtn;
namespace Slic3r { namespace GUI {

class PA_Calibration_Dlg : public DPIDialog
{
public:
    PA_Calibration_Dlg(wxWindow* parent, wxWindowID id, Plater* plater);
    ~PA_Calibration_Dlg();
    void on_dpi_changed(const wxRect& suggested_rect) override;
	void on_show(wxShowEvent& event);

protected:
    void         reset_params();
    int          GetSelectedMethod();
    int          GetSelectedExtruderType();
	virtual void on_start(wxCommandEvent& event);
	virtual void on_extruder_type_changed(wxCommandEvent& event);
	virtual void on_method_changed(wxCommandEvent& event);

protected:
	bool m_bDDE;
	Calib_Params m_params;


	//wxRadioBox* m_rbExtruderType;
	//wxRadioBox* m_rbMethod;
    wxStaticBox* extruderBox; 
    wxStaticBox* methodBox;
    wxRadioButton* m_radioNear;
    wxRadioButton* m_radioFar;
    int            m_selectedExtruderType;
    wxRadioButton* m_radioPA;
    wxRadioButton* m_radioLine;
    wxRadioButton* m_radioV;
    int            m_selectedMethod;

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
    void         set_selected_option(int index);
    Calib_Params m_params;
    ImgBtn*      m_btn0 = nullptr;
    ImgBtn*      m_btn1 = nullptr;
    ImgBtn*      m_btn2 = nullptr;
    ImgBtn*      m_btn3 = nullptr;
    ImgBtn*      m_btn4 = nullptr;
    ImgBtn*      m_btn5 = nullptr;
    ImgBtn*      m_btn6 = nullptr;
    ImgBtn*      m_btn7 = nullptr;
    ImgBtn*      m_btn8 = nullptr;
    std::array<ImgBtn*, 9> m_option_buttons{};
    //Button*      m_btn8;
    Plater*      m_plater;

private:
    int   m_selected_index = 0;
    float m_CurrentValue   = 0.8f;
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

class XY_Offset_Calibration_Dlg : public DPIDialog
{
public:
    XY_Offset_Calibration_Dlg(wxWindow* parent, wxWindowID id, Plater* plater);
    ~XY_Offset_Calibration_Dlg();
    void on_dpi_changed(const wxRect& suggested_rect) override;

protected:
    void on_confirm(wxCommandEvent& event);

private:
    ComboBox* m_cb_toolhead_count;
    ComboBox* m_cb_nozzle_diameter;
    ComboBox* m_cb_step;
    Button*   m_btn_confirm;
    Plater*   m_plater;

    int    m_toolhead_count = 2;
    double m_nozzle_diameter = 0.4;
    double m_step = 0.1;
};

}} // namespace Slic3r::GUI

#endif
