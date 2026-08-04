#include "GUI_Init.hpp"

#include "libslic3r/AppConfig.hpp"

#include "slic3r/GUI/GUI.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/3DScene.hpp"
#include "slic3r/GUI/InstanceCheck.hpp"
#include "slic3r/GUI/format.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/Plater.hpp"
#include <boost/log/trivial.hpp>
#include <memory>

// To show a message box if GUI initialization ends up with an exception thrown.
#include <wx/msgdlg.h>

#include <boost/nowide/iostream.hpp>
#include <boost/nowide/convert.hpp>

#if __APPLE__
    #include <signal.h>
    #include <unistd.h> // getpid, getppid
#endif // __APPLE__
#ifdef TARGET_OS_MAC
    //#ifdef USE_BREAKPAD
    #include "client/mac/handler/exception_handler.h"
    //#endif
#endif
namespace Slic3r {
namespace GUI {

int GUI_Run(GUI_InitParams &params)
{
#if __APPLE__
    // On OSX, we use boost::process::spawn() to launch new instances of PrusaSlicer from another PrusaSlicer.
    // boost::process::spawn() sets SIGCHLD to SIGIGN for the child process, thus if a child PrusaSlicer spawns another
    // subprocess and the subrocess dies, the child PrusaSlicer will not receive information on end of subprocess
    // (posix waitpid() call will always fail).
    // https://jmmv.dev/2008/10/boostprocess-and-sigchld.html
    // The child instance of PrusaSlicer has to reset SIGCHLD to its default, so that posix waitpid() and similar continue to work.
    // See GH issue #5507
    signal(SIGCHLD, SIG_DFL);
#endif // __APPLE__

    //BBS: remove the try-catch and let exception goto above
    try {
        //GUI::GUI_App* gui = new GUI::GUI_App(params.start_as_gcodeviewer ? GUI::GUI_App::EAppMode::GCodeViewer : GUI::GUI_App::EAppMode::Editor);
        bool enable_test = false;
        if (params.argc >= 2 && params.argv != nullptr && std::string(params.argv[1]) == std::string("test#157369"))
            enable_test = true;
        GUI::GUI_App* gui = new GUI::GUI_App(enable_test);
        //if (gui->get_app_mode() != GUI::GUI_App::EAppMode::GCodeViewer) {
            // G-code viewer is currently not performing instance check, a new G-code viewer is started every time.
            bool gui_single_instance_setting = gui->app_config->get("app", "single_instance") == "true";
            BOOST_LOG_TRIVIAL(warning) << "GUI_Run: single_instance setting=" << (gui_single_instance_setting ? "true" : "false");
            // Extra diagnostics: current PID and whether we see a minidump argument
            bool has_minidump_arg = false;
            if (params.argc > 1 && params.argv && params.argv[1]) {
                has_minidump_arg = boost::starts_with(std::string(params.argv[1]), "minidump://file=");
            }
#if __APPLE__
            BOOST_LOG_TRIVIAL(warning) << "macOS GUI_Run: current pid=" << getpid() << ", has_minidump_arg=" << (has_minidump_arg ? "true" : "false");
#endif
            // In minidump relaunch scenario, skip single-instance check to allow a dedicated dump handler instance.
            if (has_minidump_arg) {
                BOOST_LOG_TRIVIAL(warning) << "GUI_Run: detected minidump argument -> skipping single-instance check to allow crash report flow";
            } else {
                if (Slic3r::instance_check(params.argc, params.argv, gui_single_instance_setting)) {
                    BOOST_LOG_TRIVIAL(error) << "GUI_Run: instance_check returned true (another instance found). Exiting this instance.";
                    //TODO: do we have delete gui and other stuff?
                    return -1;
                } else {
                    BOOST_LOG_TRIVIAL(warning) << "GUI_Run: instance_check returned false (no other instance).";
                }
            }
        //}

//      gui->autosave = m_config.opt_string("autosave");
        GUI::GUI_App::SetInstance(gui);
        gui->init_params = &params;
        
        #ifdef TARGET_OS_MAC
            boost::filesystem::path   tempPath = boost::filesystem::path(wxFileName::GetTempDir().ToStdString());
            BOOST_LOG_TRIVIAL(warning) << "macOS Breakpad: using tempPath=" << tempPath.string();
            auto dmpCallBack = [](const char* dump_dir,
                                   const char* minidump_id,
                                   void* context, bool succeeded) -> bool {
            //wxMessageBox(dump_dir,wxT("Message box caption"),wxNO_DEFAULT|wxYES_NO|wxCANCEL|wxICON_INFORMATION,nullptr);
                BOOST_LOG_TRIVIAL(warning) << "macOS Breakpad: dmpCallBack invoked, dump_dir=" << dump_dir << ", minidump_id=" << minidump_id << ", succeeded=" << (succeeded ? "true" : "false");
                if (succeeded) {
                    boost::filesystem::path oldPath(dump_dir);
                    oldPath.append(minidump_id).replace_extension(".dmp");
                    BOOST_LOG_TRIVIAL(warning) << "macOS Breakpad: oldPath=" << oldPath.string();

                    boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
                    // 创建一个 time_facet 对象，用于自定义时间格式
                    boost::posix_time::time_facet* timeFacet = new boost::posix_time::time_facet();
                    std::stringstream              ss;
                    // 设置时间格式为 yyyyMMDD_hhmmss
                    timeFacet->format("%Y%m%d_%H%M%S");
                    ss.imbue(std::locale(std::locale::classic(), timeFacet));
                    ss << now;

                    std::string timeStr        = ss.str();
                    std::string processNameStr = timeStr + std::string("_") + SLIC3R_PROCESS_NAME + std::string("_") + CREALITYPRINT_VERSION +
                                                std::string("_") + PROJECT_VERSION_EXTRA;
                    boost::filesystem::path newPath(dump_dir);
                    newPath.append(processNameStr).replace_extension(".dmp");
                    BOOST_LOG_TRIVIAL(warning) << "macOS Breakpad: newPath=" << newPath.string();
                    if (boost::filesystem::exists(oldPath)) {
                        // MessageBox(NULL, newPath.wstring().c_str(), minidump_id, MB_OK | MB_ICONINFORMATION);
                        try {
                            boost::filesystem::rename(oldPath, newPath);
                            BOOST_LOG_TRIVIAL(warning) << "macOS Breakpad: renamed dump to " << newPath.string();
                        } catch (const std::exception& e) {
                            BOOST_LOG_TRIVIAL(error) << "macOS Breakpad: rename failed: " << e.what();
                        }
                        #ifdef TARGET_OS_MAC
                        // 获取当前可执行文件路径
                        wxString exePath = wxStandardPaths::Get().GetExecutablePath();
                        BOOST_LOG_TRIVIAL(warning) << "macOS Breakpad: exePath=" << exePath.ToStdString();
                        // 提取 .app 包路径
                        wxString appBundlePath;
                        size_t   pos = exePath.rfind(wxT("/Contents/MacOS/"));
                        if (pos != wxString::npos) {
                            appBundlePath = exePath.substr(0, pos);
                            BOOST_LOG_TRIVIAL(warning) << "macOS Breakpad: appBundlePath=" << appBundlePath.ToStdString();
                        } else {
                            BOOST_LOG_TRIVIAL(error) << "macOS Breakpad: failed to derive app bundle path from exePath";
                        }
                        // Ack file path: create next to the minidump file to confirm the new instance reaches OnInit
                        wxString ackPath = wxString::FromUTF8((newPath.string() + ".ack").c_str());
                        // Whether the .app bundle directory exists
                        bool bundle_exists = false;
                        if (!appBundlePath.empty()) {
                            try {
                                bundle_exists = boost::filesystem::exists(boost::filesystem::path(appBundlePath.ToStdString()));
                            } catch (...) {
                                bundle_exists = false;
                            }
                        }
                        BOOST_LOG_TRIVIAL(warning) << "macOS Breakpad: bundle_exists=" << (bundle_exists ? "true" : "false");
                        // 记录当前进程 PID（用于对比是否真正拉起了新实例）
                        BOOST_LOG_TRIVIAL(warning) << "macOS Breakpad: current pid=" << getpid() << ", parent pid=" << getppid();
                        // 为后续构造命令准备参数字符串（保持简单），路径统一使用单引号包裹
                        wxString minidumpArgStr = wxString::Format(
                            "minidump://file=%s",
                            wxString::FromUTF8(newPath.string().c_str())
                        );
                        BOOST_LOG_TRIVIAL(warning) << "macOS Breakpad: minidumpArg=" << minidumpArgStr.ToStdString();
                        
                        // 优先尝试使用 Launch Services：open -n（强制新实例）/ -a（可能激活旧实例），最后兜底直接执行二进制
                        if (!bundle_exists) {
                            BOOST_LOG_TRIVIAL(error) << "macOS Breakpad: app bundle path missing, skip 'open -na'/'open -a' and try direct exec binary (single-quoted)";
                            wxString command_exec;
                            command_exec.Printf("'%s' '%s'", exePath, minidumpArgStr);
                            BOOST_LOG_TRIVIAL(warning) << "macOS Breakpad: direct-exec command=" << command_exec.ToStdString();
                            long pid_exec = wxExecute(command_exec, wxEXEC_ASYNC);
                            if (pid_exec <= 0) {
                                BOOST_LOG_TRIVIAL(error) << "macOS Breakpad: direct exec failed as well";
                            } else {
                                BOOST_LOG_TRIVIAL(warning) << "macOS Breakpad: direct exec started, pid=" << pid_exec;
                            }
                        } else {
                            // 首选：open -na 'AppBundle' --args 'minidump://file=...'
                            wxString command_force_new = wxString::Format("open -na '%s' --args '%s'", appBundlePath, minidumpArgStr);
                            BOOST_LOG_TRIVIAL(warning) << "macOS Breakpad: restart command (prefer -na)=" << command_force_new.ToStdString();
                            boost::log::core::get()->flush();
                            long pid_force = wxExecute(command_force_new, wxEXEC_ASYNC);
                            if (pid_force <= 0) {
                                BOOST_LOG_TRIVIAL(error) << "macOS Breakpad: preferred 'open -na' failed to start process. Trying fallback 'open -a' (may activate existing instance)";
                                // 回退1：允许激活旧实例
                                wxString command_allow_activate = wxString::Format("open -a '%s' --args '%s'", appBundlePath, minidumpArgStr);
                                BOOST_LOG_TRIVIAL(warning) << "macOS Breakpad: fallback1 command (open -a)=" << command_allow_activate.ToStdString();
                                long pid_a = wxExecute(command_allow_activate, wxEXEC_ASYNC);
                                if (pid_a <= 0) {
                                    BOOST_LOG_TRIVIAL(error) << "macOS Breakpad: fallback1 'open -a' failed. Trying fallback 'exec binary'";
                                    // 回退2：直接执行 Contents/MacOS 下的二进制
                                    wxString command_exec;
                                    command_exec.Printf("'%s' '%s'", exePath, minidumpArgStr);
                                    BOOST_LOG_TRIVIAL(warning) << "macOS Breakpad: fallback2 command (exec binary)=" << command_exec.ToStdString();
                                    long pid_exec = wxExecute(command_exec, wxEXEC_ASYNC);
                                    if (pid_exec <= 0) {
                                        BOOST_LOG_TRIVIAL(error) << "macOS Breakpad: fallback2 'exec binary' also failed to start process";
                                    } else {
                                        BOOST_LOG_TRIVIAL(warning) << "macOS Breakpad: fallback2 started, pid=" << pid_exec;
                                    }
                                } else {
                                    BOOST_LOG_TRIVIAL(warning) << "macOS Breakpad: fallback1 started (open -a), pid=" << pid_a;
                                    BOOST_LOG_TRIVIAL(warning) << "macOS Breakpad: note: the pid here is for the 'open' utility, not the target app; open -a may just activate an existing instance and forward args";
                                }
                            } else {
                                BOOST_LOG_TRIVIAL(warning) << "macOS Breakpad: restart started (open -na), pid=" << pid_force;
                                BOOST_LOG_TRIVIAL(warning) << "macOS Breakpad: note: the pid here is for the 'open' utility, not the target app";
                            }
                        }
                        //wxMessageBox(command,wxT("Message box caption"),wxNO_DEFAULT|wxYES_NO|wxCANCEL|wxICON_INFORMATION,nullptr);
                        #endif
                    } else {
                        BOOST_LOG_TRIVIAL(error) << "macOS Breakpad: expected dump file not found: " << oldPath.string();
                    }

                    
                } else {
                    BOOST_LOG_TRIVIAL(error) << "macOS Breakpad: dmpCallBack succeeded=false";
                }
                boost::log::core::get()->flush();
                return true;
            };
            // A minidump launcher is already handling a previous crash. Registering
            // Breakpad again would relaunch another handler if the report UI fails,
            // creating an unbounded restart loop.
            std::unique_ptr<google_breakpad::ExceptionHandler> crash_handler;
            if (!has_minidump_arg) {
                crash_handler.reset(new google_breakpad::ExceptionHandler(tempPath.string(), NULL, dmpCallBack, NULL, true, NULL));
                BOOST_LOG_TRIVIAL(warning) << "macOS Breakpad: ExceptionHandler registered, dump_dir=" << tempPath.string();
            } else {
                BOOST_LOG_TRIVIAL(warning) << "macOS Breakpad: minidump launcher detected, skip ExceptionHandler registration";
            }
        #endif
        if (params.argc > 1) {
            // STUDIO-273 wxWidgets report error when opening some files with specific names
            // wxWidgets does not handle parameters, so intercept parameters here, only keep the app name
            // Log incoming args before trimming them
            {
                std::stringstream ss;
                ss << "GUI_Run: argc=" << params.argc << ", argv=[";
                for (int i = 0; i < params.argc; ++i) {
                    ss << (i == 0 ? "" : ", ") << (params.argv && params.argv[i] ? params.argv[i] : "<null>");
                }
                ss << "]";
                BOOST_LOG_TRIVIAL(warning) << ss.str();
            }
            // If we are relaunching for minidump handling, keep the full argv to preserve the parameter.
            bool has_minidump_arg_local = false;
            if (params.argc > 1 && params.argv && params.argv[1])
                has_minidump_arg_local = boost::starts_with(std::string(params.argv[1]), "minidump://file=");
            if (has_minidump_arg_local) {
                BOOST_LOG_TRIVIAL(warning) << "GUI_Run: detected minidump argument -> NOT truncating argv, passing full argv to wxEntry()";
                return wxEntry(params.argc, params.argv);
            }
            BOOST_LOG_TRIVIAL(warning) << "GUI_Run: truncating argv to keep only argv[0] before wxEntry() (this may drop parameters)";
            int                 argc = 1;
            std::vector<char *> argv;
            argv.push_back(params.argv[0]);
            return wxEntry(argc, argv.data());
        } else {
            BOOST_LOG_TRIVIAL(warning) << "GUI_Run: argc <= 1, passing through original argv to wxEntry()";
            return wxEntry(params.argc, params.argv);
        }
    } catch (const Slic3r::Exception &ex) {
        BOOST_LOG_TRIVIAL(error) << ex.what() << std::endl;
        wxMessageBox(boost::nowide::widen(ex.what()), _L("Creality Print GUI initialization failed"), wxICON_STOP);
    } catch (const std::exception &ex) {
        BOOST_LOG_TRIVIAL(error) << ex.what() << std::endl;
        wxMessageBox(format_wxstr(_L("Fatal error, exception catched: %1%"), ex.what()), _L("Creality Print GUI initialization failed"), wxICON_STOP);
    }
    // error
    return 1;
}
}
}
