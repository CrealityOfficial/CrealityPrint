#pragma once

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
#include <curl/curl.h>
#include <iostream>
#include <fstream>
#include "slic3r/GUI/GUI_Utils.hpp"
#include "miniz/miniz.h"


struct SystemInfo {
    wxString osDescription;
    wxString cpuModel;
    wxString graphicsCardVendor;
    wxString openGLVersion;
    wxString build;
    wxString uuid;
};
class TextInput;
class ErrorReportDialog : public Slic3r::GUI::DPIDialog
{
public:
    // 构造函数，接收父窗口指针和对话框标题
    ErrorReportDialog(wxWindow* parent, const wxString& title);
    wxString getSystemInfo();
    void sendReport();
    wxString zipFiles();
    void     setDumpFilePath(wxString dumpFilePath);
	bool addLogFiles(mz_zip_archive& archive);

protected:
    void on_dpi_changed(const wxRect& suggested_rect) override;
    void on_sys_color_changed() override {}

private:
    TextInput* m_InfoInput = nullptr;
    SystemInfo m_info;
    wxString m_dumpFilePath;
    wxString m_systemInfoFilePath;
    void sendEmail(wxString zipFilePath);
    void       GetErrorReport();

    // 收集日志文件并按最后修改时间排序
    std::vector<std::filesystem::path> collectLogFiles(const std::filesystem::path& log_path);

    // 创建日志压缩包的临时路径
    wxString createLogZipPath();

    // 压缩日志文件到指定路径
    bool compressLogFiles(const std::vector<std::filesystem::path>& log_files, const wxString& logZipPath);

    // 尝试将单个日志文件添加到压缩包
    bool tryAddLogFileToArchive(const std::filesystem::path& log_file, mz_zip_archive& archive);

    // 尝试复制文件并添加到压缩包
    bool tryCopyAndAddFile(const std::filesystem::path& srcPath, mz_zip_archive& zip, const char* entryName);

    // 将日志压缩包添加到主压缩包
    bool addLogZipToMainArchive(mz_zip_archive& archive, const wxString& logZipPath);

};
