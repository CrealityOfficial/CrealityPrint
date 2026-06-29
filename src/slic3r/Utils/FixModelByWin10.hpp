#ifndef slic3r_GUI_Utils_FixModelByWin10_hpp_
#define slic3r_GUI_Utils_FixModelByWin10_hpp_

#include <string>
#include "../GUI/Widgets/ProgressDialog.hpp"

class ProgressDialog;

namespace Slic3r {

class Model;
class ModelObject;
class Print;

// Returns true when current runtime has a supported mesh-repair backend.
extern bool has_mesh_repair_backend();

extern bool is_windows10();
// returt false, if fixing was canceled
extern bool fix_model_by_win10_sdk_gui(ModelObject &model_object, int volume_idx,GUI::ProgressDialog &progress_dlg, const wxString &msg_header, std::string &fix_result);

// returt false, if fixing was canceled
extern bool fix_model_by_imati_stl_gui(ModelObject &model_object, int volume_idx, GUI::ProgressDialog &progress_dlg,
                                const wxString &msg_header, std::string &fix_result);

// Platform-neutral fix entry.
extern bool fix_model(ModelObject &model_object, int volume_idx, GUI::ProgressDialog &progress_dlg,
               const wxString &msg_header, std::string &fix_result);

} // namespace Slic3r

#endif /* slic3r_GUI_Utils_FixModelByWin10_hpp_ */
