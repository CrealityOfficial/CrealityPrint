#ifndef slic3r_GUI_Utils_FixModelByWin10_hpp2_
#define slic3r_GUI_Utils_FixModelByWin10_hpp2_

#include <string>
#include "crslice2/header.h"

namespace Slic3r {
#ifdef HAS_WIN10SDK
class Model;
class ModelObject;
class Print;
class ccglobal::Tracer;



extern bool is_windows10();
// returt false, if fixing was canceled
extern bool fix_model_by_win10_sdk_gui(Slic3r::ModelObject &model_object, int volume_idx,/*GUI::ProgressDialog &progress_dlg, const wxString &msg_header,*/ std::string &fix_result, ccglobal::Tracer* tracer);

#else /* HAS_WIN10SDK */

inline bool is_windows10() { return false; }
// returt false, if fixing was canceled
inline bool fix_model_by_win10_sdk_gui(ModelObject &, int, std::string& fix_result, ccglobal::Tracer* tracer) { return false; }

#endif /* HAS_WIN10SDK */

} // namespace Slic3r

#endif /* slic3r_GUI_Utils_FixModelByWin10_hpp_ */
