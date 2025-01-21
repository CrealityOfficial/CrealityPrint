#include "wx/artprov.h"
#include "wx/cmdline.h"
#include "wx/notifmsg.h"
#include "wx/settings.h"
#include <wx/webview.h>
#include <wx/string.h>

#if wxUSE_WEBVIEW_EDGE
#include "wx/msw/webview_edge.h"
#endif

#include "wx/webviewarchivehandler.h"
#include "wx/webviewfshandler.h"
#include "wx/numdlg.h"
#include "wx/infobar.h"
#include "wx/filesys.h"
#include "wx/fs_arc.h"
#include "wx/fs_mem.h"
#include "wx/stdpaths.h"
#include <wx/panel.h>
#include <wx/tbarbase.h>
#include "wx/textctrl.h"
#include <wx/timer.h>
#include "nlohmann/json_fwd.hpp"
#include "slic3r/GUI/GUI_Utils.hpp"

namespace Slic3r {
namespace GUI {

class Plater;
class PartPlate;

class CxSentToPrinterDialog : public DPIDialog
{
public:
    CxSentToPrinterDialog(Plater* plater = nullptr);
    ~CxSentToPrinterDialog();

    void load_url(const wxString& url, const wxString& apikey = "");
    void UpdateState();
    void OnClose(wxCloseEvent& evt);
    void OnError(wxWebViewEvent& evt);
    void OnLoaded(wxWebViewEvent& evt);
    void reload();

    void OnScriptMessage(wxWebViewEvent& evt);
    void ExecuteScriptCommand(const std::string& commandInfo, bool async = false);
    void RegisterHandler(const std::string& command, std::function<void(const nlohmann::json&)> handler);
    void UnregisterHandler(const std::string& command);

    virtual bool Show(bool show = true) wxOVERRIDE;

    bool get_show_status() { return m_show_status; }
    void on_device_list_updated();

    void on_dpi_changed(const wxRect& suggested_rect) override;
    void update_plate_preview_img_on_send_page();

private:
    void bind_events();
    void SendAPIKey();
    std::string get_plate_data_on_show();
    std::string get_onlygcode_plate_data_on_show();
    std::string get_updated_plate_preview_img(int plateIndex);
    void update_send_page_content();
    std::string get_device_data_on_show();
    void update_device_data_on_send_page();
    void run_script(std::string content);
    void handle_register_complete(const nlohmann::json& json_data);
    void handle_send_gcode(const nlohmann::json& json_data);
    void handle_send_3mf(const nlohmann::json& json_data);
    void handle_cancel_send(const nlohmann::json& json_data);
    std::string build_match_color_cmd_info(int plateIndex, const std::string& ipAddress);
    void handle_request_color_match_info(const nlohmann::json& json_data);
    void handle_receive_color_match_info(const nlohmann::json& json_data);
    void handle_send_start_print_cmd(const nlohmann::json& json_data);
    void handle_request_update_plate_thumbnail(const nlohmann::json& json_data);
    void handle_forward_device_detail(const nlohmann::json& json_data);
    double get_gcode_total_weight();
    void get_gcode_display_info(wxString& total_weight_str, wxString& print_time, Slic3r::GUI::PartPlate* plate);
    void notify_update_plate_thumbnail_data(const nlohmann::json& json_data);
    void post_notify_event(const std::vector<int>& plate_extruders, const std::vector<std::string>& extruder_match_colors, bool bUpdateSelf=true);
    void restore_extruder_colors();
    void OnCloseWindow(wxCloseEvent& event);
    void handle_set_error_cmd(const nlohmann::json& json_data);

    wxWebView* m_browser;
    long       m_zoomFactor;
    wxString   m_apikey;
    bool       m_apikey_sent;
    wxColour   m_colour_def_color{ wxColour(0, 255, 0) };

    bool m_show_status = false;
    Plater*	 m_plater{ nullptr };
    int      m_request_color_match_plateIndex {0};
    bool     m_is_first_show {true};
    wxString m_uploadingIp = wxEmptyString;
    // when open this SendToPrinterDialog, backup the extruder colors, then restore them when dialog closed
    std::vector<std::string> m_backup_extruder_colors;

    std::unordered_map<std::string, std::function<void(const nlohmann::json&)>> m_commandHandlers;
};

} // namespace GUI
} // namespace Slic3r