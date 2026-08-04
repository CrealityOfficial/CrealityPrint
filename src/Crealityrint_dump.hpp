#include "slic3r/GUI/BBLStatusBar.hpp"
#include <wx/app.h>
#include <wx/wx.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/string.h>
#include <wx/statbmp.h>
#include <wx/artprov.h>
#include <wx/statline.h>
#include <wx/filename.h>
#include <filesystem>
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "libslic3r_version.h"
#include "nlohmann/json.hpp"
#include "miniz/miniz.h"
#include <curl/curl.h>
#include <iostream>
#include <fstream>
struct SystemInfo {
    wxString osDescription;
    wxString graphicsCardVendor;
    wxString openGLVersion;
    wxString build;
    wxString uuid;
};
class ErrorReportDialog : public wxDialog {
public:
    // 构造函数，接收父窗口指针和对话框标题
    ErrorReportDialog(wxWindow* parent, const wxString& title) : wxDialog(parent, wxID_ANY, title) {
        // 创建垂直方向的布局管理器
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

        // 创建水平方向的布局管理器，用于放置标题
        wxBoxSizer* titleSizer = new wxBoxSizer(wxHORIZONTAL);
        //std::filesystem::path imagePath = "resources\\images\\warning.png";
        wxIcon warningIcon = wxArtProvider::GetIcon(wxART_WARNING, wxART_MESSAGE_BOX);

    // 将图标转换为位图
        wxBitmap bitmap(warningIcon);
        // 创建 wxStaticBitmap 控件并添加到窗口
        wxStaticBitmap* staticBitmap = new wxStaticBitmap(this, wxID_ANY, bitmap, wxPoint(50, 50));
        wxStaticText* text = new wxStaticText(this, wxID_ANY, "A serious error has occurred in Some App. Please send this error report to us to fix the problem.\nPlease click the \"Send Report\" button to automatically publish the error report to our server.", wxPoint(20, 20));
        titleSizer->Add(staticBitmap, 0, wxALIGN_CENTER | wxALL, 10);
        titleSizer->Add(text, 0, wxALIGN_CENTER | wxALL, 10);
        sizer->Add(titleSizer, 0, wxALIGN_CENTER | wxALL, 10);
        // 分割线
        wxStaticLine* line = new wxStaticLine(this, wxID_ANY);
        sizer->Add(line, 0, wxALL | wxEXPAND, 5);
        
        
        GetErrorReport();
        wxString formattedString = wxString::Format(wxT("OS: %s\nGraphicsCard: %s\nOpenGLVersion: %s\nVersion: %s\nUid: %s\n"), m_info.osDescription, m_info.graphicsCardVendor, m_info.openGLVersion, m_info.build,m_info.uuid);
        // 创建水平方向的布局管理器，用于放置文本框
        wxStaticText* vtext = new wxStaticText(this, wxID_ANY, formattedString, wxPoint(20, 20));
        sizer->Add(vtext, 0, wxALIGN_LEFT | wxALL, 10);

        // 创建发送按钮
        wxButton* sendButton = new wxButton(this, wxID_OK, "SendReport");
        // 创建取消按钮
        wxButton* cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");

        // 创建水平方向的布局管理器，用于放置按钮
        wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
        buttonSizer->Add(sendButton, 0, wxALL, 5);
        buttonSizer->Add(cancelButton, 0, wxALL, 5);

        // 将按钮布局管理器添加到主布局管理器
        sizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 10);

        // 设置对话框的布局管理器
        SetSizer(sizer);
        // 调整对话框大小以适应内容
        sizer->Fit(this);
    }
    wxString getSystemInfo() {
        nlohmann::json j;
        j["osDescription"] = m_info.osDescription.ToStdString();
        j["graphicsCardVendor"] = m_info.graphicsCardVendor.ToStdString();
        j["openGLVersion"] = m_info.openGLVersion.ToStdString();
        j["build"] = m_info.build.ToStdString();
        j["uuid"] = m_info.uuid.ToStdString();
         try {
            // 获取临时目录路径
            std::filesystem::path tempDir(wxFileName::GetTempDir().ToStdString());
            // 生成一个唯一的临时文件名
            std::filesystem::path tempFilePath = tempDir / "system_info.json";

            // 打开临时文件以写入 JSON 数据
            std::ofstream tempFile(tempFilePath);
            if (tempFile.is_open()) {
                // 将 JSON 对象写入文件，使用 dump(4) 进行格式化输出
                tempFile << j.dump(4);
                tempFile.close();
                return tempFilePath.wstring();
                } else {
                    
                }
            } catch (const std::filesystem::filesystem_error& e) {
                
            } catch (const std::exception& e) {
                
            }

        return "";
    }
    void sendReport();
    wxString zipFiles();
    void setDumpFilePath(wxString dumpFilePath) {
        m_dumpFilePath = dumpFilePath;
    }
    private:
        SystemInfo m_info;
        wxString m_dumpFilePath;
        wxString m_systemInfoFilePath;
        void sendEmail(wxString zipFilePath);
        void GetErrorReport() {
            // 获取错误报告
            wxString osDescription = wxGetOsDescription();
            m_info.osDescription = osDescription;
            m_info.build = wxString(CREALITYPRINT_VERSION, wxConvUTF8);
            m_info.uuid = wxDateTime::Now().Format("%Y%m%d%H%M%S");
            // 获取显卡信息
           if (!glfwInit()) {
                std::cerr << "Failed to initialize GLFW!" << std::endl;
                return ;
            }

            // 创建隐藏（不可见）窗口
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
            GLFWwindow* window = glfwCreateWindow(1, 1, "", nullptr, nullptr);

            if (!window) {
                std::cerr << "Failed to create hidden GLFW window!" << std::endl;
                glfwTerminate();
                return ;
            }

            glfwMakeContextCurrent(window);

            // 初始化GLAD
            if (gladLoaderLoadGL() == 0) {
                std::cerr << "Failed to initialize GLAD!" << std::endl;
                return ;
            }

            // 获取显卡信息
            const GLubyte* renderer = glGetString(GL_RENDERER);
            const GLubyte* version = glGetString(GL_VERSION);

            std::cout << "Renderer: " << renderer << std::endl;
            std::cout << "OpenGL version: " << version << std::endl;
            m_info.graphicsCardVendor = wxString(reinterpret_cast<char*>(const_cast<GLubyte*>(renderer)), wxConvUTF8);
            m_info.openGLVersion = wxString(reinterpret_cast<char*>(const_cast<GLubyte*>(version)), wxConvUTF8);
            // 销毁窗口和清理
            glfwDestroyWindow(window);
            glfwTerminate();
            
            return ;
        }

};