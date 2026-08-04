#include "libslic3r/libslic3r.h"
#include "libslic3r/PresetBundle.hpp"
#include "GUI_ObjectList.hpp"
#include "GUI_Factories.hpp"
// #include "GUI_ObjectLayers.hpp"
#include "GUI_App.hpp"
#include "UITour.hpp"
#include "I18N.hpp"
#include "Plater.hpp"
#include "BitmapComboBox.hpp"
#include "MainFrame.hpp"

#include "slic3r/Utils/UndoRedo.hpp"

#include "OptionsGroup.hpp"
#include "Tab.hpp"
#include "wxExtensions.hpp"
#include "libslic3r/Model.hpp"
#include "GLCanvas3D.hpp"
#include "Selection.hpp"
#include "PartPlate.hpp"
#include "format.hpp"
#include "NotificationManager.hpp"
#include "MsgDialog.hpp"
#include "Widgets/ProgressDialog.hpp"
#include "SingleChoiceDialog.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/trivial.hpp>

#include <wx/progdlg.h>
#include <wx/listbook.h>
#include <wx/numformatter.h>
#include <wx/headerctrl.h>
#include <glad/gl.h>

#include "slic3r/Utils/FixModelByWin10.hpp"
#include "libslic3r/Format/bbs_3mf.hpp"
#include "libslic3r/PrintConfig.hpp"

#ifdef __WXMSW__
#include "wx/uiaction.h"
#include <wx/renderer.h>
#endif /* __WXMSW__ */
#include "Gizmos/GLGizmoScale.hpp"

#include "PhysicalPrinterDialog.hpp"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>
#include "slic3r/Config/DispConfig.h"
#include "print_manage/data/DataCenter.hpp"
#include "GLTexture.hpp"
#include "print_manage/Utils.hpp"

#include "libslic3r/ModelVolume.hpp"
#include "libslic3r/ModelInstance.hpp"

namespace Slic3r
{
namespace GUI
{

} //namespace GUI
} //namespace Slic3r
