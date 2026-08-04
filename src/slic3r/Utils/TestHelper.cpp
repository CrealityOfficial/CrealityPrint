#include <thread>
#include <memory>

#include <miniz.h>
#include <boost/asio.hpp>

#include "TestHelper.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/ModelInstance.hpp"
#include "libslic3r/Utils.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "slic3r/GUI/GLCanvas3D.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/Widgets/WebView.hpp"
#include "slic3r/GUI/print_manage/AppMgr.hpp"
#include "slic3r/GUI/Tab.hpp"
#include "slic3r/GUI/OptionsGroup.hpp"
#include "slic3r/GUI/NotificationManager.hpp"
using boost::asio::ip::tcp;
using namespace Slic3r;
using namespace Slic3r::GUI;

static const std::pair<unsigned int, unsigned int> THUMBNAIL_SIZE_3MF = {300, 300};

using ASync_Callback_Type = function<void(nlohmann::json)>;
static std::unordered_map<std::string, ASync_Callback_Type> async_callback_list;
static std::unordered_map<std::string, std::vector<std::pair<nlohmann::json, ASync_Callback_Type>>> event_async_callback_list;

namespace Test {

bool enable_test;
/////////////////////////////////////BASE////////////////////////////////////////

// static inline void test_helper_app_respone(std::string cmd, nlohmann::json ret)
//{
//     ADD_TEST_RESPONE("TestHelper", cmd, 0, ret.dump(-1, ' ', true));
// }

class ClientSession
{
public:
    ClientSession(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() { do_read(); }

    void do_write(std::string datas)
    {
        auto p = std::make_shared<std::string>(std::move(datas)); // Keep buffer alive for async write
        boost::asio::async_write(socket_, boost::asio::buffer(*p), [this, p](boost::system::error_code /*ec*/, std::size_t /*length*/) {});

        // boost::asio::async_write(socket_, boost::asio::buffer(datas.c_str(), datas.size()),
        //                          [this](boost::system::error_code ec, std::size_t /*length*/) {
        //     });
    }

    std::function<void(std::string&)> read_callback = [](std::string&) {};

private:
    void do_read()
    {
        socket_.async_read_some(boost::asio::buffer(&readbuff, 2048), [this](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                std::string output(readbuff, length);
                read_callback(output);
            } else {
                if (ec == boost::asio::error::eof || ec == boost::asio::error::connection_reset) {
                    socket_.close();
                    return;
                }
            }
            do_read();
        });
    }

    tcp::socket socket_;
    int         buff_len;
    char        readbuff[2048];
};

class TCPServer
{
public:
    TCPServer(boost::asio::io_context& io_context, short port) : acceptor_(io_context)
    {
        boost::system::error_code ec;
        acceptor_.set_option(tcp::acceptor::reuse_address(true), ec);

        tcp::endpoint endpoint(tcp::v4(), port);
        acceptor_.open(endpoint.protocol());
        acceptor_.bind(endpoint);
        acceptor_.listen();
        do_accept();
    }

    std::function<void(ClientSession*)> accept_callback = [](ClientSession*) {};
    std::unique_ptr<ClientSession>      client_;

private:
    void do_accept()
    {
        acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                client_.reset();
                client_ = std::make_unique<ClientSession>(std::move(socket));
                accept_callback(client_.get());
            }
            do_accept();
        });
    }

    tcp::acceptor acceptor_;
};

class CmdChannel
{
public:
    CmdChannel(short port)
    {
        server                  = new TCPServer(io_context, port);
        server->accept_callback = [this](ClientSession* session) {
            session->read_callback = std::bind(&CmdChannel::parse_string_to_cmd, this, std::placeholders::_1);
            session->start();
        };
    }

    void start()
    {
        try {
            thd = std::thread([this]() { io_context.run(); });
        } catch (std::exception& e) {}
    }
    void stop()
    {
        io_context.stop();
        if (thd.joinable())
            thd.join();
    }
    void cmd_respone(std::string respone) 
    { 
        if (!server->client_) {
            BOOST_LOG_TRIVIAL(warning) << "No client connected to test socket";
            return;
        }
        server->client_->do_write(respone); 
    }

private:
    void parse_string_to_cmd(std::string& info)
    {
        auto cmd_index = info.find(";");
        auto cmd       = info.substr(0, cmd_index);
        auto arg       = info.substr(cmd_index + 1, info.length() - cmd_index);
        Visitor().call_cmd(cmd, arg);
    }

    boost::asio::io_context io_context;
    TCPServer*              server;
    std::thread             thd;
};

TestHelper::TestHelper(bool enable)
{
    if (enable) {
        register_cmd();
        call_cmd = std::bind(&TestHelper::call_cmd_inner, this, std::placeholders::_1, std::placeholders::_2);
        init_cmd_channel(12345); // open cmd socket
    }
}

TestHelper::~TestHelper()
{
    if (m_cmd_channel) {
        m_cmd_channel->stop();
    }
}

void TestHelper::cmd_respone(std::string cmd, nlohmann::json ret) { m_cmd_channel->cmd_respone(cmd + ";" + ret.dump(-1, ' ', true)); }

void TestHelper::init_cmd_channel(short port)
{
    m_cmd_channel = new CmdChannel(port);
    m_cmd_channel->start();
}

std::string TestHelper::call_cmd_inner(std::string cmd, std::string json_str)
{
    constexpr int  FAILED = 1;
    constexpr int  OK     = 0;
    nlohmann::json ret;
    auto           gen_failed_json = [&ret, FAILED](std::string err) -> nlohmann::json& {
        ret["code"]    = FAILED;
        ret["error"]   = err;
        ret["payload"] = "";
        return ret;
    };
    auto gen_ok_json = [&ret, OK](std::string payload) -> nlohmann::json& {
        ret["code"]  = OK;
        ret["error"] = "OK";
        // ret["payload"] = payload;
        try {
            ret["payload"] = nlohmann::json::parse(payload); // Try to parse JSON payload
        } catch (...) {
            ret["payload"] = payload; // Fallback: store raw string payload
        }
        return ret;
    };
    // cp internal cmd
    auto inner_it = m_inner_cmd2func.find(cmd);
    if (inner_it != m_inner_cmd2func.end())
    {
        nlohmann::json arg;
        try
        {
            if (!json_str.empty())
                arg = json::parse(json_str);
            std::string err;
            std::string payload;
            inner_it->second(arg, payload, err);
            return payload;
        } catch (const std::exception& e) {
            return "";
        }
    }
    // socket exposed cmd
    auto it = m_cmd2func.find(cmd);
    if (it == m_cmd2func.end()) {
        cmd_respone(cmd, gen_failed_json("COMMAND NOT FAOUND"));
        return "_";
    }
    try {
        nlohmann::json arg;
        arg = json::parse(json_str);
        std::string err;                                 // Error message
        std::string payload;                             // Serialized payload
        int         res = it->second(arg, payload, err); // Registered handler should serialize payload (dump)
        if (res == 0)
            cmd_respone(cmd, gen_ok_json(payload));
        else if (res > 0)
            cmd_respone(cmd, gen_failed_json(err));
        else if (res < 0)
            void(); // is async;
    } catch (const nlohmann::json::parse_error& err) {
        cmd_respone(cmd, gen_failed_json("PARSE arg FAILED"));
    } catch (const std::exception& e) {
        cmd_respone(cmd, gen_failed_json(std::string("EXEC CMD ERROR, ") + e.what()));
    }
    return "_";
}

void call_when_target_eventloop_exec(std::string cmd, nlohmann::json& arg, ASync_Callback_Type callback)
{
    wxCommandEvent e(EVT_TEST_HELPER_CMD);
    std::srand(std::time(nullptr));
    std::string cmdkey = std::to_string(std::rand());
    cmd += cmdkey;
    arg["cmd"] = cmd;
    e.SetString(arg.dump(-1, ' ', true));
    async_callback_list.insert({arg["cmd"], callback});
    wxPostEvent(&wxGetApp(), e); // Post event so async_callback_list[cmd] callback runs in GUI thread
}

void respone_on_next_eventloop(std::string command, int code, std::string error = "", nlohmann::json ret = {})
{
    call_when_target_eventloop_exec(
        command, 
        ret,
        [code, error](nlohmann::json ret) 
        {
            ret["ret"] = code;
            if (!error.empty())
                ret["error"] = error;
            Test::Visitor().call_cmd("cmd_respone", ret.dump(-1, ' ', true));
        }
    );
}

void call_when_event_spread(std::string event, nlohmann::json& arg, ASync_Callback_Type callback)
{
    if (event_async_callback_list.find(event) == event_async_callback_list.end())
        event_async_callback_list[event] = {};
    event_async_callback_list[event].emplace_back(std::pair{arg, callback});
}

static int event_spread(nlohmann::json arg, std::string& payload, std::string& error)
{
    auto event = arg["event"].get<std::string>();
    auto param = arg["param"].get<std::string>();

    auto it = event_async_callback_list.find(event);
    if (it != event_async_callback_list.end())
    {
        //
        //    support event:
        //    "canvas_render_finished"
        //    "slice_compete"
        //    "slice_started"
        //    "sendToPrint_loaded"
        //    "test_exec_js_respone"
        //
        for (auto& pair : it->second)
        {
            auto para = pair.first;
            para["param"] = param;
            pair.second(para);
        }

        event_async_callback_list.erase(event);
    }
    return 0;
}


class Status
{
public:
    enum AsyncCmdStatus{
        Unfinished = 0,
        Finished = 1,
        None = 2
    };
    enum StatusType {
        AppReady = 0,
        AsyncCmd = 1
    };
    bool                               app_ready = false;
    std::string                        app_id;
    std::map<std::string, AsyncCmdStatus> cmd_status;
    bool                                  capture_mode = false;
    bool                                  ban_dialog   = true;

    Status()
    {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        auto millis   = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        app_id        = std::to_string(millis);
    }

    AsyncCmdStatus query_cmd_status(std::string cmd)
    { 
        if (cmd_status.find(cmd) == cmd_status.end())
            return None;
        auto cmd_state = cmd_status[cmd];
        if (cmd_state == Finished) // Burn after Finished and Reading
            cmd_status.erase(cmd);
        return cmd_state;
    }
    void set_cmd_status(std::string cmd, AsyncCmdStatus state) { 
        cmd_status[cmd] = state;
    }
} status;

/////////////////////////////////////CMD////////////////////////////////////////

static int app_ready(nlohmann::json arg, std::string& payload, std::string& error)
{
    status.app_ready = true;
    return 0;
}

static int query_status(nlohmann::json arg, std::string& payload, std::string& error) 
{
    int status_type = arg["type"].get<int>();
    if (status_type == Status::AppReady)
    {
        if (!status.app_ready)
        {
            error = "app not ready";
            return 3;
        }
        payload = status.app_id; 
        return 0;
    }
    else if (status_type == Status::AsyncCmd)
    {
        auto cmd = arg["cmd"].get<std::string>();
        nlohmann::json ret;
        ret["cmd"]    = cmd;
        ret["status"] = (int) status.query_cmd_status(cmd);
        payload = ret.dump(-1, ' ', true);
        return 0;
    }    
    error = "unsupport status type";
    return 1;
}

static int capture(nlohmann::json arg, std::string& payload, std::string& error)
{
    nlohmann::json output;
    auto           plater = wxGetApp().plater();
    if (!plater) {
        error = "plater object is nullptr";
        return 1;
    }

    auto callback = [](nlohmann::json arg) {
        std::string    save_as_fmt, type;
        int            capture_w, capture_h;
        nlohmann::json output;
        try {
            save_as_fmt = arg["save_as"].get<std::string>();
            type        = arg["type"].get<std::string>();
            capture_w   = arg["w"].get<int>();
            capture_h   = arg["h"].get<int>();
        } catch (nlohmann::json::parse_error& e) {
            nlohmann::json output;
            output["ret"] = 1;
            Test::Visitor().call_cmd("cmd_respone", output.dump(-1, ' ', true));
            return;
        }

        auto                                    plater     = wxGetApp().plater();
        decltype(plater->get_view3D_canvas3D()) canvas_ins = nullptr;
        if (arg["type"].get<std::string>() == "preview")
            canvas_ins = plater->get_preview_canvas3D();
        else if (arg["type"].get<std::string>() == "ready")
            canvas_ins = plater->get_view3D_canvas3D();
        else {
            output["ret"]   = 1;
            output["error"] = "not support capture type";
            Test::Visitor().call_cmd("cmd_respone", output.dump(-1, ' ', true));
            return;
        }
        auto plater_size = plater->get_partplate_list().get_plate_count();
        for (auto i = 0; i < plater_size; ++i) {
            int                    THUMBNAIL_SIZE_3MF[] = {capture_w, capture_h};
            ThumbnailData          data;
            const ThumbnailsParams thumbnail_params = {{}, false, true, true, true, i};
            canvas_ins->render_thumbnail(data, THUMBNAIL_SIZE_3MF[0], THUMBNAIL_SIZE_3MF[1], thumbnail_params, Camera::EType::Ortho);

            size_t png_size = 0;
            void*  png_data = tdefl_write_image_to_png_file_in_memory_ex((const void*) data.pixels.data(), data.width, data.height, 4,
                                                                         &png_size, MZ_DEFAULT_COMPRESSION, 1);
            // save as png
            std::string save_as = save_as_fmt;
            auto        rep_inx = save_as.find("%s");
            save_as.replace(rep_inx, 2, std::to_string(i + 1));
            std::ofstream ofs(save_as, std::ofstream::binary);
            ofs.write((char*) png_data, png_size);
            ofs.close();
        }

        output["ret"] = 0;
        Test::Visitor().call_cmd("cmd_respone", output.dump(-1, ' ', true));
    };

    call_when_target_eventloop_exec("capture", arg, callback);
    return -1;
}

// This function handles EVT_TEST_HELPER_CMD events
static int handle_app_cmd(nlohmann::json arg, std::string& payload, std::string& error)
{
    std::string cmd = arg["cmd"].get<std::string>();
    async_callback_list[cmd](arg); // GUI thread executes stored callback
    async_callback_list.erase(cmd);
    return -1;
}

static int cmd_respone_wrapper(nlohmann::json arg, std::string& payload, std::string& error)
{
    int ret = 0;
    if (arg.contains("ret")) {
        ret = arg["ret"].get<int>();
        arg.erase("ret");
    }
    if (arg.contains("error")) {
        error = arg["error"].get<std::string>();
        arg.erase("error");
    }

    payload = arg.dump(-1, ' ', true);
    return ret;
}

static int empty_respone(nlohmann::json arg, std::string& payload, std::string& error) { return 0; }

static int trigger_load_project(nlohmann::json arg, std::string& payload, std::string& error)
{
    status.set_cmd_status("trigger_load_project", Status::Unfinished);
    call_when_target_eventloop_exec(
        "trigger_load_project", 
        arg, 
        [](nlohmann::json arg) {
            wxGetApp().plater()->load_project(wxString::FromUTF8(arg["file"].get<std::string>()));
            status.set_cmd_status("trigger_load_project", Status::Finished);
        });
    return 0; 
}

static int trigger_load_project2(nlohmann::json arg, std::string& payload, std::string& error)
{
    call_when_target_eventloop_exec("trigger_load_project2", arg, [](nlohmann::json j) {
        nlohmann::json out;
        try {
            auto file                   = j.at("file").get<std::string>();
            bool discard_preset_changes = j.value("discard_preset_changes", false);

            if (discard_preset_changes) {
                wxGetApp().discard_all_current_preset_changes(); // Reset presets
            }

            wxGetApp().plater()->load_project(wxString::FromUTF8(file)); // Load project synchronously

            out["ret"]    = 0;
            out["error"]  = "OK";
            out["file"]   = file;
            out["status"] = "loaded";
        } catch (const std::exception& e) {
            out["ret"]   = 1;
            out["error"] = std::string("load_project exception: ") + e.what();
        } catch (...) {
            out["ret"]   = 1;
            out["error"] = "load_project unknown error";
        }
        Test::Visitor().call_cmd("cmd_respone", out.dump(-1, ' ', true));
    });
    return -1; // Async: prevent call_cmd_inner from sending automatic OK
}

static wxWindow* FindButton(wxWindow* root, int id, const wxString& name, const wxString& label)
{
    if (!root)
        return nullptr;

    // 1) Find by id
    if (id != wxID_ANY) {
        if (auto* w = wxWindow::FindWindowById(id, root))
            return w;
    }
    // 2) Then by name
    if (!name.empty()) {
        if (auto* w = wxWindow::FindWindowByName(name, root))
            return w;
    }
    // 3) Finally recurse by label
    if (!label.empty()) {
        if (auto* btn = wxDynamicCast(root, wxButton)) {
            if (btn->GetLabel() == label)
                return btn;
        }
        for (auto* child : root->GetChildren()) {
            if (auto* found = FindButton(child, wxID_ANY, "", label))
                return found;
        }
    }
    return nullptr;
}

static void PostButtonClick(wxWindow* btn)
{
    // Post a button click event to the target control
    wxCommandEvent evt(wxEVT_BUTTON, btn->GetId());
    evt.SetEventObject(btn);
    wxPostEvent(btn, evt);
}

static int click_button_cmd(nlohmann::json arg, std::string& payload, std::string& error)
{
    call_when_target_eventloop_exec("click_button", arg, [](nlohmann::json j) {
        nlohmann::json out;

        int      id    = j.contains("id") ? j["id"].get<int>() : wxID_ANY;
        wxString name  = j.contains("name") ? wxString::FromUTF8(j["name"].get<std::string>()) : "";
        wxString label = j.contains("label") ? wxString::FromUTF8(j["label"].get<std::string>()) : "";

        int timeout_ms  = j.contains("timeout_ms") ? j["timeout_ms"].get<int>() : 5000;
        int interval_ms = j.contains("interval_ms") ? j["interval_ms"].get<int>() : 100;

        wxWindow* top = wxTheApp->GetTopWindow();
        if (!top) {
            out["ret"]   = 1;
            out["error"] = "no top window";
            Test::Visitor().call_cmd("cmd_respone", out.dump(-1, ' ', true));
            return;
        }

        wxWindow* btn = FindButton(top, id, name, label);
        if (!btn) {
            out["ret"]   = 1;
            out["error"] = "button not found (id/name/label unmatched)";
            Test::Visitor().call_cmd("cmd_respone", out.dump(-1, ' ', true));
            return;
        }

        auto try_click = [&](wxWindow* found_btn) -> bool {
            if (!found_btn)
                return false;
            if (!found_btn->IsEnabled() || !found_btn->IsShown())
                return false;
            try {
                PostButtonClick(found_btn);
                out["ret"]   = 0;
                out["info"]  = "wxEVT_BUTTON posted";
                out["id"]    = found_btn->GetId();
                out["name"]  = std::string(found_btn->GetName().ToUTF8());
                out["label"] = std::string(wxDynamicCast(found_btn, wxButton) ? wxDynamicCast(found_btn, wxButton)->GetLabel().ToUTF8() :
                                                                                "");
            } catch (const std::exception& ex) {
                out["ret"]   = 1;
                out["error"] = std::string("exception: ") + ex.what();
            }
            Test::Visitor().call_cmd("cmd_respone", out.dump(-1, ' ', true));
            return true;
        };

        if (try_click(btn))
            return;

        // Start timer polling: Button exists but is currently unavailable/invisible
        wxTimer*   timer    = new wxTimer(top);
        wxLongLong start_ll = wxGetUTCTimeMillis(); 

        timer->Bind(wxEVT_TIMER, [=](wxTimerEvent&) mutable {
            wxWindow* cur_btn = FindButton(top, id, name, label);
            if (try_click(cur_btn)) {
                timer->Stop();
                delete timer; 
                return;
            }

            wxLongLong diff       = wxGetUTCTimeMillis() - start_ll;
            long       elapsed_ms = diff.ToLong(); 

            if (elapsed_ms >= timeout_ms) {
                timer->Stop();
                out["ret"]   = 1;
                out["error"] = "timeout waiting for button to become enabled/visible";
                Test::Visitor().call_cmd("cmd_respone", out.dump(-1, ' ', true));
                delete timer;
            }
        });

        timer->Start(interval_ms, wxTIMER_CONTINUOUS);
    });

    return -1;
}

static int export_gcode_cmd(nlohmann::json arg, std::string& payload, std::string& error)
{
    // Run in GUI thread
    call_when_target_eventloop_exec("export_gcode_3mf", arg, [](nlohmann::json j) {
        nlohmann::json out;

        try {
            auto plater = wxGetApp().plater();
            if (!plater) {
                out["ret"]   = 1;
                out["error"] = "plater is null";
                Test::Visitor().call_cmd("cmd_respone", out.dump(-1, ' ', true));
                return;
            }

            // Parse parameters
            bool     export_all = false;
            fs::path output;
            try {
                if (j.contains("export_all"))
                    export_all = j["export_all"].get<bool>();
                if (j.contains("output"))
                    output = j["output"].get<std::string>();
                else {
                    out["ret"]   = 1;
                    out["error"] = "missing 'output'";
                    Test::Visitor().call_cmd("cmd_respone", out.dump(-1, ' ', true));
                    return;
                }
            } catch (const std::exception& e) {
                out["ret"]   = 1;
                out["error"] = std::string("bad args: ") + e.what();
                Test::Visitor().call_cmd("cmd_respone", out.dump(-1, ' ', true));
                return;
            }

            // Call headless export without touching UI
            std::string err;
            int         ret = plater->export_gcode_3mf_headless(output, export_all, err);

            if (ret == 0) {
                out["ret"]        = 0;
                out["export_all"] = export_all;
                out["output"]     = output.string();
            } else {
                out["ret"]        = ret;
                out["error"]      = err;
                out["export_all"] = export_all;
                out["output"]     = output.string();
            }
        } catch (const std::exception& e) {
            out["ret"]   = 1;
            out["error"] = e.what();
        }

        // Response will be converted to code/error/payload by cmd_respone_wrapper
        Test::Visitor().call_cmd("cmd_respone", out.dump(-1, ' ', true));
    });

    return -1; // Async
}
static int get_error_cmd(nlohmann::json arg, std::string& payload, std::string& error)
{
    call_when_target_eventloop_exec("get_error", arg, [](nlohmann::json j) {
        nlohmann::json out;
        try {
            out["ret"]   = 0;
            auto notification_manager = wxGetApp().plater()->get_notification_manager();
            out["data"] = notification_manager->get_all_notification();
        
        } catch (const std::exception& e) {
            out["ret"]   = 1;
            out["error"] = e.what();
        }
        BOOST_LOG_TRIVIAL(warning) << out.dump(-1, ' ', true);
        Test::Visitor().call_cmd("cmd_respone", out.dump(-1, ' ', true)); 
        });
    return -1;
}
static int reset_project_cmd(nlohmann::json arg, std::string& payload, std::string& error)
{
    call_when_target_eventloop_exec("reset_project", arg, [](nlohmann::json j) {
        nlohmann::json out;
        try {
            bool        skip_confirm = j.value("skip_confirm", true); // Default: skip confirmation
            bool        silent       = j.value("silent", true);       // Default: run silently without switching UI
            std::string project_name = j.value("project_name", "");
            int         rc           = -1;
            if (project_name.empty()) {
                rc = wxGetApp().plater()->new_project(skip_confirm, silent);
            } else {
                rc = wxGetApp().plater()->new_project(skip_confirm, silent, project_name);
            }

            out["ret"]          = (rc == wxID_CANCEL) ? 1 : 0;
            out["result_code"]  = rc; // wxID_YES / wxID_CANCEL
            out["skip_confirm"] = skip_confirm;
            out["silent"]       = silent;
            if (rc == wxID_CANCEL)
                out["error"] = "cancelled by confirm dialog";
            else
                out["code_explanation"] = "5103 means wxID_YES.";
        } catch (const std::exception& e) {
            out["ret"]   = 1;
            out["error"] = e.what();
        }
        Test::Visitor().call_cmd("cmd_respone", out.dump(-1, ' ', true));
    });
    return -1; // Async: real response sent later in callback
}

static int is_capture_mode(nlohmann::json arg, std::string& payload, std::string& error)
{
    nlohmann::json output;
    output["enable"] = status.capture_mode? 1:0;
    payload = output.dump(-1, ' ', true);
    return 0;
}

static int set_capture_mode(nlohmann::json arg, std::string& payload, std::string& error)
{ 
    int enable  = arg["enable"].get<int>();
    status.capture_mode = enable == 0? false:true;
    wxGetApp().plater()->set_current_canvas_as_dirty();
    wxGetApp().plater()->get_current_canvas3D()->render();

    call_when_event_spread("canvas_render_finished", arg, [](nlohmann::json arg) {
        nlohmann::json output;
        output["ret"] = 0;
        Test::Visitor().call_cmd("cmd_respone", output.dump(-1, ' ', true));
    });
    return -1;
}

static int get_widget_geometry(nlohmann::json arg, std::string& payload, std::string& error)
{
    int x = -1, y = -1, w = -1, h = -1;
    int widget_id       = arg["widget_id"].get<int>();
    if  (widget_id == 0x01) { // BBL
        auto size = wxGetApp().mainframe->topbar()->GetSize();
        auto pos = wxGetApp().mainframe->topbar()->GetPosition();
        x         = pos.x;
        y         = pos.y;
        w         = size.x;
        h         = size.y;
        
    } 
    else if (widget_id == 0x02)
    {
        auto size = wxGetApp().plater()->get_current_canvas3D()->get_canvas_size();
        w         = size.get_width();
        h         = size.get_height();
    }
    nlohmann::json ret;
    ret["x"] = x;
    ret["y"] = y;
    ret["w"] = w;
    ret["h"] = h;
    payload = ret.dump(0, ' ', -1);
    return 0;
}

static int new_project(nlohmann::json arg, std::string& payload, std::string& error)
{
    call_when_target_eventloop_exec("new_project", arg, [](nlohmann::json arg) {
        wxGetApp().plater()->new_project();
        nlohmann::json output;
        output["ret"] = 0;
        Test::Visitor().call_cmd("cmd_respone", output.dump(-1, ' ', true));
    });
    return -1;
}
static int move_model_instance(nlohmann::json arg, std::string& payload, std::string& error)
{
    call_when_target_eventloop_exec("move_model_instance", arg, [](nlohmann::json arg) {
        nlohmann::json output;
        try {
            auto plater = wxGetApp().plater();
            if (plater == nullptr) {
                output["ret"] = 1;
                output["error"] = "plater is null";
                Test::Visitor().call_cmd("cmd_respone", output.dump(-1, ' ', true));
                return;
            }

            Model& model = plater->model();
            size_t object_index = arg.value("object_index", 0);
            size_t instance_index = arg.value("instance_index", 0);
            if (object_index >= model.objects.size()) {
                output["ret"] = 1;
                output["error"] = "object_index out of range";
                Test::Visitor().call_cmd("cmd_respone", output.dump(-1, ' ', true));
                return;
            }

            ModelObject* object = model.objects[object_index];
            if (object == nullptr || instance_index >= object->instances.size()) {
                output["ret"] = 1;
                output["error"] = "instance_index out of range";
                Test::Visitor().call_cmd("cmd_respone", output.dump(-1, ' ', true));
                return;
            }

            ModelInstance* instance = object->instances[instance_index];
            Vec3d old_offset = instance->get_offset();
            Vec3d delta(
                arg.value("delta_x", 10.0),
                arg.value("delta_y", 0.0),
                arg.value("delta_z", 0.0)
            );
            Vec3d new_offset = old_offset + delta;
            instance->set_offset(new_offset);
            object->invalidate_bounding_box();
            plater->changed_objects({object_index});
            plater->set_current_canvas_as_dirty();
            plater->get_current_canvas3D()->render();

            output["ret"] = 0;
            output["object_index"] = object_index;
            output["instance_index"] = instance_index;
            output["old_offset"] = {old_offset.x(), old_offset.y(), old_offset.z()};
            output["new_offset"] = {new_offset.x(), new_offset.y(), new_offset.z()};
        } catch (const std::exception& e) {
            output["ret"] = 1;
            output["error"] = std::string("move_model_instance exception: ") + e.what();
        } catch (...) {
            output["ret"] = 1;
            output["error"] = "move_model_instance unknown error";
        }
        Test::Visitor().call_cmd("cmd_respone", output.dump(-1, ' ', true));
    });
    return -1;
}
static int trigger_slice(nlohmann::json arg, std::string& payload, std::string& error)
{
    call_when_target_eventloop_exec("trigger_slice", arg, [](nlohmann::json arg) {
        // Slice start
        call_when_event_spread("slice_started", arg, [](nlohmann::json arg) {
            status.set_cmd_status("trigger_slice", Status::Unfinished);
            nlohmann::json output;
            output["ret"] = 0;
            Test::Visitor().call_cmd("cmd_respone", output.dump(-1, ' ', true));
        });
        wxGetApp().mainframe->slice_plate(MainFrame::eSliceAll); // Default: slice all plates
    });
    return -1;
}

static int get_slicing_progress(nlohmann::json arg, std::string& payload, std::string& error)
{
    call_when_target_eventloop_exec("get_slicing_progress", arg, [](nlohmann::json) {
        nlohmann::json ret;
        auto*          nm = wxGetApp().notification_manager();
        if (!nm) {
            ret["ret"]   = 1;
            ret["error"] = "no notification manager";
        } else {
            auto*  plater              = wxGetApp().plater();
            bool   all_plate_finished  = true;
            size_t slicable_plate_cnt  = 0;
            auto   plate_count         = plater->get_partplate_list().get_plate_count();
            for (int i = 0; i < plate_count; ++i) {
                auto* plate = plater->get_partplate_list().get_plate(i);
                if (plate == nullptr || plate->empty() || !plate->has_printable_instances())
                    continue;
                ++slicable_plate_cnt;
                if (!plate->is_slice_result_ready_for_print()) {
                    all_plate_finished = false;
                    break;
                }
            }
            if (slicable_plate_cnt == 0)
                all_plate_finished = false;

            ret["ret"]                = 0;
            ret["progress"]           = nm->get_slicing_progress_snapshot();
            ret["all_plate_finished"] = all_plate_finished;
        }
        Test::Visitor().call_cmd("cmd_respone", ret.dump(-1, ' ', true));
    });
    return -1;
}

static int select_printer(nlohmann::json arg, std::string& payload, std::string& error)
{
    SidebarPrinter& bar = wxGetApp().plater()->sidebar_printer();
    std::vector<std::string> items = bar.texts_of_combo_printer();
    auto printer_name = arg["printer_name"].get<std::string>();
    int                     index        = -1;

    auto cur_preset_name = wxGetApp().preset_bundle->printers.get_selected_preset_name();
    if (cur_preset_name.find(printer_name) != std::string::npos)
    {
        return 0;
    }

    for (int i = 0; i < items.size(); ++i)
    {
        const auto& name = items[i];
        
        if (name == printer_name)
        {
            index = i;
            break;
        }
    }
    if (index == -1)
    {
        error = "no found printer";
        return 1;
    }

    bar.select_printer_preset(items[index], index);
    // Pending 
    respone_on_next_eventloop("select_printer", 0);
    return -1;
}

static int binding_phy_printer(nlohmann::json arg, std::string& payload, std::string& error)
{ 
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    call_when_target_eventloop_exec("binding_phy_printer", arg, 
        [](nlohmann::json arg) 
        {
            auto ip_address = arg["ip_or_name"].get<std::string>();
            if (wxGetApp().obj_list()->bind_phy_printer_by_ip_or_name(ip_address)) {
                wxGetApp().plater()->set_current_canvas_as_dirty();
                wxGetApp().plater()->get_current_canvas3D()->render();

                call_when_event_spread("canvas_render_finished", arg, [](nlohmann::json arg) {
                    // Pending
                    respone_on_next_eventloop("binding_phy_printer11", 0);
                });
            } 
            else
                respone_on_next_eventloop("binding_phy_printer22", 1, "not ip found");
        });
    return -1;
} 

static int trigger_send_to_print(nlohmann::json arg, std::string& payload, std::string& error)
{
    auto type = arg["type"].get<std::string>();
    if (type == "single") {
        wxCommandEvent e(EVT_GLTOOLBAR_SEND_TO_LOCAL_NET_PRINTER);
        wxPostEvent(wxGetApp().plater(), e);
    } else {
        wxCommandEvent e(EVT_GLTOOLBAR_PRINT_MULTI_MACHINE);
        wxPostEvent(wxGetApp().plater(), e);
    }
    call_when_event_spread("sendToPrint_loaded", arg, [](nlohmann::json arg) {
        nlohmann::json ret;
        ret["ret"] = 0;
        Test::Visitor().call_cmd("cmd_respone", ret.dump(-1, ' ', true));
    });    
    return -1;
}

static int close_send_to_print(nlohmann::json arg, std::string& payload, std::string& error)
{
    call_when_target_eventloop_exec("close_send_to_print", arg, [](nlohmann::json arg) {
        std::string web_name = "SentToPrinter";
        DM::Apps    apps;
        DM::AppMgr::Ins().GetAppsByName(web_name, apps);
        if (apps.empty()) {
            nlohmann::json ret;
            ret["ret"]   = 1;
            ret["error"] = "no found webview";
            Test::Visitor().call_cmd("cmd_respone", ret.dump(-1, ' ', true));
        } else {
            apps[0].browser->GetParent()->Close();
            nlohmann::json ret;
            ret["ret"]   = 0;
            Test::Visitor().call_cmd("cmd_respone", ret.dump(-1, ' ', true));
        }
    });
    return -1;
}

static int exec_js_in_webview(nlohmann::json arg, std::string& payload, std::string& error)
{
    call_when_target_eventloop_exec("exec_js_in_webview", arg, [](nlohmann::json arg) {
        auto which_webview = arg["which_webview"].get<std::string>();
        std::string js_script;
        if (arg.find("js_script_localtion") != arg.end())
        {
            auto js_script_localtion = arg["js_script_localtion"].get<std::string>();
            fstream fs(js_script_localtion, std::ios::in);
            if (!fs.is_open())
            {
                nlohmann::json ret;
                ret["ret"]   = 1;
                ret["error"] = "cannot open js script file";
                Test::Visitor().call_cmd("cmd_respone", ret.dump(-1, ' ', true));
                return;
            }
            std::stringstream ssbuffer;
            ssbuffer << fs.rdbuf();
            js_script = std::move(ssbuffer.str());
            fs.close();
        }
        else
        {
            js_script = arg["js_script"].get<std::string>();
        }

        DM::Apps apps;
        DM::AppMgr::Ins().GetAppsByName(which_webview, apps);
        if (apps.empty()) {
            nlohmann::json ret;
            ret["ret"] = 1;
            ret["error"] = "no found webview";
            Test::Visitor().call_cmd("cmd_respone", ret.dump(-1, ' ', true));
        } 
        else
        {
            WebView::RunScript(apps[0].browser, js_script);
        }
            
    });
    // pending
    call_when_event_spread("test_exec_js_respone", arg, [](nlohmann::json arg) {
        nlohmann::json ret;
        ret["ret"] = 0;
        auto paramstr = arg["param"].get<std::string>();
        nlohmann::json param     = nlohmann::json::parse(arg["param"].get<std::string>());
        ret["arg"] = param["arg"];
        Test::Visitor().call_cmd("cmd_respone", ret.dump(-1, ' ', true));
    });
    return -1;
}

static int is_ban_dialog(nlohmann::json arg, std::string& payload, std::string& error) 
{ 
    payload = status.ban_dialog ? "y" : "n";
    return 0; 
}

static int ban_dialog(nlohmann::json arg, std::string& payload, std::string& error)
{
    status.ban_dialog = true;
    return 0;
}

static int allow_dialog(nlohmann::json arg, std::string& payload, std::string& error)
{
    status.ban_dialog = false;
    return 0;
}

static int set_process_paramter(nlohmann::json arg, std::string& payload, std::string& error)
{
    auto callback = [](nlohmann::json arg)
    {
        std::string error;
        TabPrint* process_tab = nullptr;
        for (auto tab : wxGetApp().tabs_list) {
            if (tab->title() == _(L("Process"))) {
                process_tab = dynamic_cast<TabPrint*>(tab);
                break;
            }
        }
        if (process_tab == nullptr) {
            error = "not found process tab, checked it";
            respone_on_next_eventloop("set_process_paramter", 1, error);
            return;
        }

        auto which_page       = arg["which_page"].get<std::string>();
        auto paramter_kv_list = arg["paramter_list"];

        if (paramter_kv_list.empty()) {
            error = "empty paramters list";
            respone_on_next_eventloop("set_process_paramter", 1, error);
            return;
        }

        std::unordered_map<std::string, std::string> kv_hash;
        for (auto paramter_kv_pair : paramter_kv_list) {
            auto key   = paramter_kv_pair["key"].get<std::string>();
            auto value = paramter_kv_pair["value"].get<std::string>();
            kv_hash.insert({key, value});
        }

        const auto&                                        pages = process_tab->get_pages();
        std::unordered_map<std::string, std::string> invaild_kv;
        for (auto& page : pages) {
            if (!which_page.empty()) {
                if (page->title() != which_page)
                    continue;
            }
            process_tab->select_item(page->title());
            for (auto& optgroup : page->m_optgroups) {
                for (auto opt : optgroup->opt_map()) {
                    auto key = opt.second.first;
                    if (kv_hash.find(key) == kv_hash.end())
                        continue;

                    auto field = optgroup->get_field(key);
                    if (dynamic_cast<TextCtrl*>(field)) {
                        auto v = wxString(kv_hash[key]);
                        optgroup->set_value(key, v);
                        field->m_on_change(key, field->get_value());
                    } else if (dynamic_cast<GUI::ColourPicker*>(field) || dynamic_cast<GUI::StaticText*>(field)) {
                        optgroup->set_value(key, kv_hash[key]);
                        field->m_on_change(key, field->get_value());
                    } else if (dynamic_cast<GUI::SliderCtrl*>(field) || dynamic_cast<GUI::SpinCtrl*>(field) ||
                               dynamic_cast<GUI::Choice*>(field)) {
                        int v = std::stoi(kv_hash[key]);
                        optgroup->set_value(key, v);
                        field->m_on_change(key, field->get_value());
                    } else if (dynamic_cast<GUI::PointCtrl*>(field)) {
                        auto point_inx = kv_hash[key].find(',');
                        int  x         = std::stoi(kv_hash[key].substr(point_inx));
                        int  y         = std::stoi(kv_hash[key].substr(point_inx, kv_hash[key].size() - point_inx));
                        auto v         = Vec2d{x, y};
                        optgroup->set_value(key, v);
                        field->m_on_change(key, field->get_value());
                    } else if (dynamic_cast<GUI::CheckBox*>(field)) {
                        bool v = kv_hash[key] == "true" ? true : false;
                        optgroup->set_value(key, v);
                        field->m_on_change(key, field->get_value());
                    } else {
                        invaild_kv.insert({key, kv_hash[key]});
                    }

                    kv_hash.erase(key);
                    if (kv_hash.empty()) {
                        // finished
                        goto end;
                    }
                }
            }
        }

    end:
        if (!invaild_kv.empty()) {
            std::string invaild_kv_msg = "had invaild type paramter: ";
            for (auto kv : invaild_kv) {
                invaild_kv_msg.append("  ");
                invaild_kv_msg.append(kv.first);
                invaild_kv_msg.append("=>");
                invaild_kv_msg.append(kv.second);
            }
            error = invaild_kv_msg;
        }
        if (!kv_hash.empty()) {
            std::string remain_kv_msg = "had not found paramter: ";
            for (auto kv : kv_hash) {
                remain_kv_msg.append("  ");
                remain_kv_msg.append(kv.first);
                remain_kv_msg.append("=>");
                remain_kv_msg.append(kv.second);
            }
            error.append(remain_kv_msg);
        }

        nlohmann::json ret;
        if (!error.empty())
        {
            ret["ret"]   = 1;
            ret["error"] = error;
            Test::Visitor().call_cmd("cmd_respone", ret.dump(-1, ' ', true));
            //respone_on_next_eventloop("set_process_paramter", 1, error);
        }
        else
        {
            ret["ret"] = 0;
            Test::Visitor().call_cmd("cmd_respone", ret.dump(-1, ' ', true));
            //respone_on_next_eventloop("set_process_paramter", 0);
        }
    };

    call_when_target_eventloop_exec("set_process_paramter", arg, callback);

    return -1;
}

void TestHelper::register_cmd() 
{ 
    // cp internal module uses cmds that return std::string; they are not exposed over socket
    m_inner_cmd2func["is_capture_mode"] = is_capture_mode;
    m_inner_cmd2func["event_spread"]   = event_spread;
    m_inner_cmd2func["is_ban_dialog"]     = is_ban_dialog;
    m_inner_cmd2func["ban_dialog"]        = ban_dialog;
    m_inner_cmd2func["allow_dialog"]      = allow_dialog;
    // socket-exposed cmds are for external processes, usually without return value
    m_cmd2func["cmd_respone"]          = cmd_respone_wrapper;
    m_cmd2func["handle_app_cmd"]       = handle_app_cmd;
    m_cmd2func["capture"]              = capture;
    m_cmd2func["set_capture_mode"]     = set_capture_mode;
    m_cmd2func["ping"]                 = empty_respone;
    m_cmd2func["trigger_load_project"] = trigger_load_project;
    m_cmd2func["query_status"]         = query_status; // status
    m_cmd2func["app_ready"]            = app_ready;
    m_cmd2func["get_widget_geometry"]  = get_widget_geometry;
    m_cmd2func["new_project"]          = new_project;
    m_cmd2func["move_model_instance"]  = move_model_instance;
    m_cmd2func["trigger_slice"]        = trigger_slice;
    m_cmd2func["get_slicing_progress"] = get_slicing_progress;
    m_cmd2func["select_printer"]       = select_printer;
    m_cmd2func["binding_phy_printer"]  = binding_phy_printer;
    m_cmd2func["trigger_send_to_print"] = trigger_send_to_print;
    m_cmd2func["close_send_to_print"]   = close_send_to_print;
    m_cmd2func["exec_js_in_webview"]    = exec_js_in_webview;
    m_cmd2func["set_process_paramter"]  = set_process_paramter;

    // LHX TODO: refactor and document these additional commands
    m_cmd2func["trigger_load_project2"] = trigger_load_project2;
    m_cmd2func["click_button"]          = click_button_cmd;
    m_cmd2func["export_gcode"]          = export_gcode_cmd;
    m_cmd2func["reset_project"]         = reset_project_cmd;
    m_cmd2func["get_error"]         = get_error_cmd;
}

} // namespace Test
