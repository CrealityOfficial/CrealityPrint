#include "AppUtils.hpp"
#include "../Widgets/WebView.hpp"
#include "../GUI.hpp"

#include <boost/uuid/detail/md5.hpp>
#include "libslic3r/Utils.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#pragma execution_character_set("utf-8")
using namespace Slic3r::GUI;
using namespace boost::uuids::detail;
using namespace Slic3r;
namespace DM{

    bool is_uos_system()
    {
#ifdef __WXGTK__
        static int cached = -1;
        if (cached != -1)
            return cached == 1;
        std::ifstream f("/etc/os-release");
        if (!f.is_open()) {
            cached = 0;
            return false;
        }
        std::string line;
        while (std::getline(f, line)) {
            std::string lower = line;
            std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
            if (lower.find("uos") != std::string::npos || lower.find("uniontech") != std::string::npos) {
                cached = 1;
                return true;
            }
        }
        cached = 0;
        return false;
#else
        return false;
#endif
    }

    void AppUtils::PostMsg(wxWebView* browse, const std::string& data)
    {
        if (browse == nullptr || browse->IsBeingDeleted())
            return;
        WebView::RunScript(browse, from_u8(data));
    }

    void AppUtils::PostMsg(wxWebView* browse, nlohmann::json& data)
    {
        if (browse == nullptr || browse->IsBeingDeleted())
            return;
        std::string json   = data.dump(-1, ' ', true);
        std::string script = "window.handleStudioCmd(" + json + ");";
        WebView::RunScript(browse, from_u8(script));
    }

    std::string AppUtils::MD5(const std::string& file)
    {
        std::string ret;
        std::string filePath = std::string(file);
        Slic3r::bbl_calc_md5(filePath, ret);
        return ret;
    }

    std::string AppUtils::extractDomain(const std::string& url)
    {
        std::string domain;
        size_t start = 0;

        // 检查是否有协议头（如 http:// 或 https://）
        if (url.find("://") != std::string::npos) {
            start = url.find("://") + 3;
        }

        // 找到域名结束的位置，即第一个 /、? 或 # 字符的位置
        size_t end = url.find_first_of("/?#:", start);
        if (end == std::string::npos) {
            // 如果没有找到结束字符，说明域名一直到字符串末尾
            domain = url.substr(start);
        }
        else {
            // 提取域名部分
            domain = url.substr(start, end - start);
        }

        return domain;
    }
    bool LANConnectCheck::pingHostWithRetry(const std::string& ip, ThreadController& ctrl, int retries, int timeout_ms, int delay_ms) {
        int attempt = 0;

        while (attempt < retries) {
            if (ctrl.isStopRequested()) return false;  // 中断点1：执行前检查[3](@ref)
            // 构建 Ping 命令
#ifdef _WIN32
            std::string command = "ping -n 4 -w 2000 " + ip;            //ping ip
#else
            std::string cmd = "ping -c 4 -W " + std::to_string(timeout_ms / 1000) + " " + ip + " > /dev/null 2>&1";
#endif

#ifdef _WIN32
            // 配置进程启动信息
            STARTUPINFOA si{};
            PROCESS_INFORMATION pi{};
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;

            if (CreateProcessA(NULL, const_cast<char*>(command.c_str()),
                NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
            {
                // 非阻塞等待并检查中断
                while (WaitForSingleObject(pi.hProcess, 50) == WAIT_TIMEOUT) {
                    if (ctrl.isStopRequested()) {
                        TerminateProcess(pi.hProcess, 1);
                        break;
                    }
                }

                DWORD exitCode;
                GetExitCodeProcess(pi.hProcess, &exitCode);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);

                if (exitCode == 0) return true;
            }
#else
            // Linux/macOS 实现
            if (system(cmd.c_str()) == 0) {
                return true; // Ping 成功
            }
#endif

            // 重试前等待
            // 重试前等待并检查中断
            if (ctrl.waitForStop(std::chrono::milliseconds(delay_ms)))
                return false;

            attempt++;

            // 下次尝试增加等待时间（指数退避）
            if (attempt < retries) {
                delay_ms *= 2; // 增加等待时间
            }
        }
        int x = 0;
        return false; // 所有尝试均失败
    }
    // 使用 Boost.Asio 检查端口
    bool LANConnectCheck::isPortOpen(const std::string& ip, int port, ThreadController& ctrl) {
        using boost::asio::deadline_timer;
        using boost::asio::ip::tcp;
        try {
            boost::asio::io_service io_service;
            tcp::socket socket(io_service);
            tcp::endpoint endpoint(boost::asio::ip::address::from_string(ip), port);

            // 异步连接+超时控制
            bool connected = false;
            boost::system::error_code ec;
            socket.async_connect(endpoint, [&](const boost::system::error_code& error) {
                ec = error;
                io_service.stop();
                });

            // 可中断的IO等待
            std::thread io_thread([&] { io_service.run(); });
            while (io_service.run_one()) {
                if (ctrl.waitForStop(std::chrono::milliseconds(100))) {
                    socket.close();
                    break;
                }
            }
            io_thread.join();
            return !ec;
        }
        catch (const std::exception& e) {
            std::cerr << "Fuction isPortOpen Exception: " << e.what() << std::endl;
            return false;
        }
    }

#ifdef _WIN32
#include <Windows.h>
#include <string>
#include <cctype>

    float getWinPingLatency(const std::string& ip, ThreadController& ctrl) {
        std::string cmd = "ping -n 5 " + ip;

        // 使用 STARTUPINFO 结构隐藏窗口
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        SECURITY_ATTRIBUTES sa;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;  // 隐藏窗口

        ZeroMemory(&pi, sizeof(pi));

        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;

        // 创建管道捕获输出
        HANDLE hReadPipe, hWritePipe;
        if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
            return -1.0f;
        }

        SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

        // 重定向输出
        si.hStdOutput = hWritePipe;
        si.hStdError = hWritePipe;
        si.dwFlags |= STARTF_USESTDHANDLES;

        // 创建进程
        char command[256];
        sprintf_s(command, "cmd /C \"%s\"", cmd.c_str());

        if (!CreateProcessA(
            NULL,
            command,
            NULL,
            NULL,
            TRUE,
            CREATE_NO_WINDOW,
            NULL,
            NULL,
            &si,
            &pi))
        {
            CloseHandle(hWritePipe);
            CloseHandle(hReadPipe);
            return -1.0f;
        }

        // 关闭写入端
        CloseHandle(hWritePipe);

        // 读取输出
        char buffer[1024];
        DWORD bytesRead;
        std::string output;
        float avgLatency = -1.0f;

       
        //while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        //    buffer[bytesRead] = '\0';
        //    output += buffer;
        //}
        while (true) {
            if (ctrl.isStopRequested()) {
                TerminateProcess(pi.hProcess, 1);
                break;
            }

            if (!ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) || bytesRead == 0)
                break;

            buffer[bytesRead] = '\0';
            output += buffer;
        }
        BOOST_LOG_TRIVIAL(error) << "getWinPingLatency output : " << output;
        // 关闭读取端
        CloseHandle(hReadPipe);

        // 等待进程结束
        WaitForSingleObject(pi.hProcess, INFINITE);

        // 关闭进程句柄
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        // 解析输出
        size_t pos = output.find(encode_path(std::string("平均 =").c_str()));
        
        if (pos == std::string::npos) {
            pos = output.find("Average =");
        }

        if (pos != std::string::npos) {
            std::string avgStr;

            // 查找等号后的数字部分
            size_t eqPos = output.find('=', pos);
            if (eqPos != std::string::npos) {
                for (size_t i = eqPos + 1; i < output.length(); i++) {
                    char c = output[i];
                    if (isdigit(c) || c == '.') {
                        avgStr += c;
                    }
                    else if (!avgStr.empty()) {
                        // 遇到非数字字符且已有数字，停止
                        break;
                    }
                }

                if (!avgStr.empty()) {
                    try {
                        avgLatency = std::stof(avgStr);
                    }
                    catch (...) {
                        avgLatency = -1.0f;
                    }
                }
            }
        }

        return avgLatency;
    } 
#else
    // 使用原来的Linux方式获取平均延迟
    float getLinuxPingLatency(const std::string& ip) {
        // 第三阶段：网络质量检测（5次ping平均延迟）
        std::string cmd = "ping -c 5 -i 0.2 " + ip + " | tail -1 | awk -F '/' '{print $5}'";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return -1;
        float avgLatency = 0.0;
        char buffer[32];
        if (fgets(buffer, sizeof(buffer), pipe)) {
            avgLatency = atof(buffer);
            pclose(pipe);
        }
        else {
            pclose(pipe);
            avgLatency = -1.0;
        }
        return avgLatency;
    }
#endif
#include <string>
#include <cstdlib>
    int LANConnectCheck::checkLan(const std::string& ip, bool secure_connection, int wss_port, ThreadController& ctrl)
    {
        std::string deviceIP = ip; // 替换为目标设备IP
        int errorcode = 0;
        // 第一阶段：检查设备是否在线（ping测试）
        if (!pingHostWithRetry(deviceIP, ctrl)) {
            if (ctrl.isStopRequested()) return -1;  // 中断代码
            errorcode = 1;
            return errorcode;
        }
        // 第二阶段：端口连通性检查
        const int info_port = secure_connection ? 443 : 80;
        const int message_port = secure_connection
            ? (wss_port > 0 && wss_port <= 65535 ? wss_port : 443)
            : 9999;

        if (!isPortOpen(deviceIP, info_port, ctrl)) {
            if (ctrl.isStopRequested()) return -1;
            return 2;  // 端口不通
        }
        // WSS 端口可能与 HTTPS 端口相同，避免重复探测。
        if (message_port != info_port && !isPortOpen(deviceIP, message_port, ctrl)) {
            if (ctrl.isStopRequested()) return -1;
            return 2;  // 端口不通
        }

        // 第三阶段：网络质量检测（5次ping平均延迟）
#ifdef _WIN32
        float avgLatency = getWinPingLatency(deviceIP,ctrl);
#else 
        float avgLatency = getLinuxPingLatency(deviceIP);
#endif
        if (avgLatency < 0) {
            errorcode = 31;
        }
        else if (avgLatency > 1000.0f) {
            errorcode = 32;
        }
        else {
            errorcode = 0;
        }
        return errorcode;
    }
}
