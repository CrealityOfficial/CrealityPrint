#ifndef slic3r_Check3mfVendor_hpp_
#define slic3r_Check3mfVendor_hpp_

#include <string>
#include "GUI_Utils.hpp"
#include "Widgets/ComboBox.hpp"
#include "miniz/miniz.h"
#include "libslic3r/miniz_extension.hpp"
#include "GUI_Preview.hpp"
#include "Plater.hpp"

namespace Slic3r {
namespace GUI {

class ChoosePresetDlg;

class Check3mfVendor
{
public:
    static Check3mfVendor* getInstance();
    void updateCurPrinterType();
    // Return true for Creality 3MF file, false for other vendors
    bool check(const std::string& fileName, const std::string& printerSettingId, BusyCursor* busy);
    bool get3mfConfig(const DynamicPrintConfig& config_loaded, DynamicPrintConfig& new_config_loaded);
    void doSelectPrinterPreset();
    bool isCreality3mf();
    void setCreality3mf(bool isCreality3mf);
    void updatePlateObject(const PlateDataPtrs& plate_data, const Slic3r::Model& model,
                           const DynamicPrintConfig& source_config);
    void centerModelToPlate(View3D* view3D, Sidebar* sidebar);

private:
    Check3mfVendor();
    bool isCreality3mf(const std::string& fileName);
    bool isCrealityIn3dModel(mz_zip_archive* pArchive);

private:
    bool m_isCurPrinterProject = false;
    bool m_isCreality3mf = false;
    bool m_bNeedSelectPrinterPreset = false;
    std::string m_printerPresetName = "";
    int m_printerPresetIdx = -1;
    DynamicPrintConfig m_new_config_loaded;
    DynamicPrintConfig m_process_config;
    std::string m_defaultProcess;
    std::vector<vector<int>> m_vtPlateObject;
};

class ChoosePresetDlg : public DPIDialog
{
public:
    ChoosePresetDlg(wxWindow* parent, const std::string& printerSettingId, bool isCurPrinterProject);
    std::string m_printerPresetName = "";
    int m_printerPresetIdx = -1;

protected:
    void on_dpi_changed(const wxRect& suggested_rect)override;
    void OnComboboxSelected(wxCommandEvent& evt);
    void OnOk(wxMouseEvent& event);

private:
    ::ComboBox* m_combo = nullptr;
    std::vector<std::string> m_vtComboText;
    int m_comboLastSelected = -1;
    int m_projectPresetCount = 0;
};

}
}

#endif
