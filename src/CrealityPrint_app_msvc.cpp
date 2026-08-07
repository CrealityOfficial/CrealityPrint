// Why?
#define _WIN32_WINNT 0x0502
// The standard Windows includes.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>
#include <wchar.h>

#ifdef SLIC3R_GUI
extern "C"
{
    // Ask hybrid-graphics drivers to run the process on the high-performance GPU.
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif /* SLIC3R_GUI */

#include <stdlib.h>
#include <stdio.h>

#ifdef SLIC3R_GUI
    #include <GL/GL.h>
#endif /* SLIC3R_GUI */

#include <string>
#include <vector>
#include <algorithm>
#include <iterator>

//#include <boost/algorithm/string/split.hpp>
//#include <boost/algorithm/string/classification.hpp>

#include <stdio.h>
#include "buildinfo.h"
#ifdef SLIC3R_GUI
class OpenGLVersionCheck
{
public:
    std::string version;
    std::string glsl_version;
    std::string vendor;
    std::string renderer;

    HINSTANCE   hOpenGL = nullptr;
    bool 		success = false;
    int         window_x = 0;
    int         window_y = 0;

    bool load_opengl_dll(int x = 0, int y = 0)
    {
        version.clear();
        glsl_version.clear();
        vendor.clear();
        renderer.clear();
        success = false;
        window_x = x;
        window_y = y;
        MSG      msg     = {0};
        WNDCLASS wc      = {0};
        wc.lpfnWndProc   = OpenGLVersionCheck::supports_opengl2_wndproc;
        wc.hInstance     = (HINSTANCE)GetModuleHandle(nullptr);
        wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
        wc.lpszClassName = L"CrealityPrint_opengl_version_check";
        wc.style = CS_OWNDC;
        if (RegisterClass(&wc) || GetLastError() == ERROR_CLASS_ALREADY_EXISTS) {
            HWND hwnd = CreateWindowW(wc.lpszClassName, L"CrealityPrint_opengl_version_check", WS_OVERLAPPEDWINDOW, window_x, window_y, 640, 480, 0, 0, wc.hInstance, (LPVOID)this);
            if (hwnd) {
                message_pump_exit = false;
                while (GetMessage(&msg, NULL, 0, 0 ) > 0 && ! message_pump_exit)
                    DispatchMessage(&msg);
            }
        }
        return this->success;
    }

    bool probe_any_display(unsigned int major, unsigned int minor)
    {
        std::vector<POINT> probe_points;
        probe_points.push_back({0, 0});
        EnumDisplayMonitors(nullptr, nullptr, enum_monitor_proc, reinterpret_cast<LPARAM>(&probe_points));

        for (const POINT& point : probe_points) {
            printf("OpenGL probe display point: x=%ld, y=%ld\n", point.x, point.y);
            if (load_opengl_dll(point.x, point.y) && is_version_greater_or_equal_to(major, minor))
                return true;
            unload_opengl_dll();
        }

        return false;
    }
    void unload_opengl_dll()
    {
        if (this->hOpenGL) {
            BOOL released = FreeLibrary(this->hOpenGL);
            if (released)
                printf("System OpenGL library released\n");
            else
                printf("System OpenGL library NOT released\n");
            this->hOpenGL = nullptr;
        }
    }

    bool is_version_greater_or_equal_to(unsigned int major, unsigned int minor) const
    {
        // printf("is_version_greater_or_equal_to, version: %s\n", version.c_str());
        //split by space delimiter
        std::vector<std::string> tokens = split_string(version, " ");
        //boost::split(tokens, version, boost::is_any_of(" "), boost::token_compress_on);       
         
        if (tokens.empty())
            return false;

         std::vector<std::string> numbers = split_string(tokens[0], ".");
//         boost::split(numbers, tokens[0], boost::is_any_of("."), boost::token_compress_on);

        unsigned int gl_major = 0;
        unsigned int gl_minor = 0;
        if (numbers.size() > 0)
            gl_major = ::atoi(numbers[0].c_str());
        if (numbers.size() > 1)
            gl_minor = ::atoi(numbers[1].c_str());
        // printf("Major: %d, minor: %d\n", gl_major, gl_minor);
        if (gl_major < major)
            return false;
        else if (gl_major > major)
            return true;
        else
            return gl_minor >= minor;
    }
protected:
    static bool message_pump_exit;

    void check(HWND hWnd)
    {
        hOpenGL = LoadLibraryExW(L"opengl32.dll", nullptr, 0);
        if (hOpenGL == nullptr) {
            printf("Failed loading the system opengl32.dll\n");
            return;
        }

        typedef HGLRC 		(WINAPI *Func_wglCreateContext)(HDC);
        typedef BOOL 		(WINAPI *Func_wglMakeCurrent  )(HDC, HGLRC);
        typedef BOOL     	(WINAPI *Func_wglDeleteContext)(HGLRC);
        typedef GLubyte* 	(WINAPI *Func_glGetString     )(GLenum);

        Func_wglCreateContext 	wglCreateContext = (Func_wglCreateContext)GetProcAddress(hOpenGL, "wglCreateContext");
        Func_wglMakeCurrent 	wglMakeCurrent 	 = (Func_wglMakeCurrent)  GetProcAddress(hOpenGL, "wglMakeCurrent");
        Func_wglDeleteContext 	wglDeleteContext = (Func_wglDeleteContext)GetProcAddress(hOpenGL, "wglDeleteContext");
        Func_glGetString 		glGetString 	 = (Func_glGetString)	  GetProcAddress(hOpenGL, "glGetString");

        if (wglCreateContext == nullptr || wglMakeCurrent == nullptr || wglDeleteContext == nullptr || glGetString == nullptr) {
            printf("Failed loading the system opengl32.dll: The library is invalid.\n");
            return;
        }

        PIXELFORMATDESCRIPTOR pfd =
        {
            sizeof(PIXELFORMATDESCRIPTOR),
            1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            PFD_TYPE_RGBA,            	// The kind of framebuffer. RGBA or palette.
            32,                        	// Color depth of the framebuffer.
            0, 0, 0, 0, 0, 0,
            0,
            0,
            0,
            0, 0, 0, 0,
            24,                        	// Number of bits for the depthbuffer
            8,                        	// Number of bits for the stencilbuffer
            0,                        	// Number of Aux buffers in the framebuffer.
            PFD_MAIN_PLANE,
            0,
            0, 0, 0
        };
        HDC ourWindowHandleToDeviceContext = ::GetDC(hWnd);
        if (ourWindowHandleToDeviceContext == nullptr) {
            printf("OpenGL probe failed: GetDC returned nullptr\n");
            return;
        }
        // Gdi32.dll
        int letWindowsChooseThisPixelFormat = ::ChoosePixelFormat(ourWindowHandleToDeviceContext, &pfd);
        if (letWindowsChooseThisPixelFormat == 0 || !SetPixelFormat(ourWindowHandleToDeviceContext, letWindowsChooseThisPixelFormat, &pfd)) {
            printf("OpenGL probe failed: pixel format unavailable\n");
            ::ReleaseDC(hWnd, ourWindowHandleToDeviceContext);
            return;
        }
        // Opengl32.dll
        HGLRC glcontext = wglCreateContext(ourWindowHandleToDeviceContext);
        if (glcontext == nullptr || !wglMakeCurrent(ourWindowHandleToDeviceContext, glcontext)) {
            printf("OpenGL probe failed: context unavailable\n");
            if (glcontext != nullptr)
                wglDeleteContext(glcontext);
            ::ReleaseDC(hWnd, ourWindowHandleToDeviceContext);
            return;
        }
        // Opengl32.dll
        const char *data = (const char*)glGetString(GL_VERSION);
        if (data != nullptr)
            this->version = data;
        // printf("check -version: %s\n", version.c_str());
        data = (const char*)glGetString(0x8B8C); // GL_SHADING_LANGUAGE_VERSION
        if (data != nullptr)
            this->glsl_version = data;
        data = (const char*)glGetString(GL_VENDOR);
        if (data != nullptr)
            this->vendor = data;
        data = (const char*)glGetString(GL_RENDERER);
        if (data != nullptr)
            this->renderer = data;
        printf("OpenGL probe: version=%s, vendor=%s, renderer=%s\n", version.c_str(), vendor.c_str(), renderer.c_str());
        // Opengl32.dll
        wglDeleteContext(glcontext);
        ::ReleaseDC(hWnd, ourWindowHandleToDeviceContext);
        this->success = true;
    }

    static BOOL CALLBACK enum_monitor_proc(HMONITOR, HDC, LPRECT rect, LPARAM data)
    {
        auto* probe_points = reinterpret_cast<std::vector<POINT>*>(data);
        if (probe_points != nullptr && rect != nullptr)
            probe_points->push_back({rect->left + 1, rect->top + 1});
        return TRUE;
    }
    static LRESULT CALLBACK supports_opengl2_wndproc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch(message)
        {
        case WM_CREATE:
        {
            CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            OpenGLVersionCheck *ogl_data = reinterpret_cast<OpenGLVersionCheck*>(pCreate->lpCreateParams);
            ogl_data->check(hWnd);
            DestroyWindow(hWnd);
            return 0;
        }
        case WM_NCDESTROY:
            message_pump_exit = true;
            return 0;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }

    std::vector<std::string> split_string(std::string str, std::string delimiter) const
	{
		std::string::size_type pos;
		std::vector<std::string> tokens;
		str += delimiter;
		size_t size = str.size();
		for (int i = 0; i < size; i++)
		{
			pos = str.find(delimiter, i);
			if (pos < size)
			{
				std::string s = str.substr(i, pos - i);
				tokens.push_back(s);
				i = pos + delimiter.size() - 1;
			}
		}

        auto not_space = [](unsigned char ch) {return !std::isspace(ch); };

		//remove the front space
        for (size_t i = 0; i < tokens.size(); i++)
        {    
			tokens[i].erase(tokens[i].begin(), std::find_if(tokens[i].begin(), tokens[i].end(), not_space));
        }

 		//remove the tail space
        for (size_t i = 0; i < tokens.size(); i++)
		{
            tokens[i].erase(std::find_if(tokens[i].rbegin(), tokens[i].rend(), not_space).base(), tokens[i].end());
		}

        //copy the non-empty string
       std::vector<std::string> result;
       std::copy_if(tokens.begin(), tokens.end(), std::back_inserter(result),[](const std::string& str) {return !str.empty(); });

		return tokens;
	}
};
// Convert multibyte string to wide string
std::wstring ConvertToWide(const char* str)
{
    int          length = MultiByteToWideChar(CP_ACP, 0, str, -1, nullptr, 0);
    if (length == 0) {
        return L"";
    }
    std::wstring wstr(length, 0);
    MultiByteToWideChar(CP_ACP, 0, str, -1, &wstr[0], length);
    return wstr;
}
bool OpenGLVersionCheck::message_pump_exit = false;
#endif /* SLIC3R_GUI */

extern "C" {
    typedef int (__stdcall *Slic3rMainFunc)(int argc, wchar_t **argv);
    Slic3rMainFunc crealityprint_main = nullptr;
}

extern "C" {
#ifdef SLIC3R_WRAPPER_NOCONSOLE
int APIENTRY wWinMain(HINSTANCE /* hInstance */, HINSTANCE /* hPrevInstance */, PWSTR /* lpCmdLine */, int /* nCmdShow */)
{
    int 	  argc;
    wchar_t **argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
#else
int wmain(int argc, wchar_t **argv)
{
#endif
    // Allow the asserts to open message box, such message box allows to ignore the assert and continue with the application.
    // Without this call, the seemingly same message box is being opened by the abort() function, but that is too late and
    // the application will be killed even if "Ignore" button is pressed.
    _set_error_mode(_OUT_TO_MSGBOX);

    std::vector<wchar_t*> argv_extended;
    argv_extended.emplace_back(argv[0]);

#ifdef SLIC3R_WRAPPER_GCODEVIEWER
    wchar_t gcodeviewer_param[] = L"--gcodeviewer";
    argv_extended.emplace_back(gcodeviewer_param);
#endif /* SLIC3R_WRAPPER_GCODEVIEWER */

#ifdef SLIC3R_GUI
    // Here one may push some additional parameters based on the wrapper type.
    bool force_mesa = false;
#endif /* SLIC3R_GUI */
    for (int i = 1; i < argc; ++ i) {
#ifdef SLIC3R_GUI
        if (wcscmp(argv[i], L"--sw-renderer") == 0)
            force_mesa = true;
        else if (wcscmp(argv[i], L"--no-sw-renderer") == 0)
            force_mesa = false;
#endif /* SLIC3R_GUI */
        argv_extended.emplace_back(argv[i]);
    }
    argv_extended.emplace_back(nullptr);

#ifdef SLIC3R_GUI
    OpenGLVersionCheck opengl_version_check;
    const bool opengl_probe_has_required_version = opengl_version_check.probe_any_display(2, 0);
    bool load_mesa =
        // Forced from the command line.
        force_mesa ||
        // Try every display-backed OpenGL path before falling back to software rendering.
        !opengl_probe_has_required_version;
#endif /* SLIC3R_GUI */

    wchar_t path_to_exe[MAX_PATH + 1] = { 0 };
    ::GetModuleFileNameW(nullptr, path_to_exe, MAX_PATH);
    wchar_t drive[_MAX_DRIVE];
    wchar_t dir[_MAX_DIR];
    wchar_t fname[_MAX_FNAME];
    wchar_t ext[_MAX_EXT];
    _wsplitpath(path_to_exe, drive, dir, fname, ext);
    _wmakepath(path_to_exe, drive, dir, nullptr, nullptr);

#ifdef SLIC3R_GUI
// https://wiki.qt.io/Cross_compiling_Mesa_for_Windows
// http://download.qt.io/development_releases/prebuilt/llvmpipe/windows/
    if (load_mesa) {
        opengl_version_check.unload_opengl_dll();
        wchar_t path_to_mesa[MAX_PATH + 1] = { 0 };
        wcscpy(path_to_mesa, path_to_exe);
        wcscat(path_to_mesa, L"mesa\\opengl32.dll");
        printf("Loading MESA OpenGL library: %S\n", path_to_mesa);
        HINSTANCE hInstance_OpenGL = LoadLibraryExW(path_to_mesa, nullptr, 0);
        if (hInstance_OpenGL == nullptr) {
            printf("MESA OpenGL library was not loaded, error=%lu\n", GetLastError());
        } else {
            printf("MESA OpenGL library was loaded successfully\n");
        }
        SetEnvironmentVariableW(L"CREALITYPRINT_OPENGL_SOFTWARE_RENDERER",
            hInstance_OpenGL != nullptr ? (force_mesa ? L"2" : L"1") : L"0");
        SetEnvironmentVariableW(L"CREALITYPRINT_OPENGL_HARDWARE_AVAILABLE",
            (opengl_probe_has_required_version || hInstance_OpenGL != nullptr) ? L"1" : L"0");
    } else {
        SetEnvironmentVariableW(L"CREALITYPRINT_OPENGL_SOFTWARE_RENDERER", L"0");
        SetEnvironmentVariableW(L"CREALITYPRINT_OPENGL_HARDWARE_AVAILABLE",
            opengl_probe_has_required_version ? L"1" : L"0");
    }
#endif /* SLIC3R_GUI */
    wchar_t path_to_slic3r[MAX_PATH + 1] = { 0 };
    std::wstring slicer_library = ConvertToWide(PROJECT_DLL_NAME_WIN32) + L".dll";
    //std::wstring slicer_library = std::wstring(PROJECT_DLL_NAME_WIN32) + L".dll";

    wcscpy(path_to_slic3r, path_to_exe);
    wcscat(path_to_slic3r, slicer_library.c_str());
//	printf("Loading Slic3r library: %S\n", path_to_slic3r);
    HINSTANCE hInstance_Slic3r = LoadLibraryExW(path_to_slic3r, nullptr, 0);
    if (hInstance_Slic3r == nullptr) {
        printf("%S was not loaded, error=%d\n", slicer_library.c_str(), GetLastError());
        return -1;
    }

    // resolve function address here
    crealityprint_main = (Slic3rMainFunc)GetProcAddress(hInstance_Slic3r,
#ifdef _WIN64
        // there is just a single calling conversion, therefore no mangling of the function name.
        "crealityprint_main"
#else	// stdcall calling convention declaration
        "_bambustu_main@8"
#endif
        );
    if (crealityprint_main == nullptr) {
        printf("could not locate the function crealityprint_main in CrealityPrint_Slicer.dll\n");
        return -1;
    }
    // argc minus the trailing nullptr of the argv
    return crealityprint_main((int)argv_extended.size() - 1, argv_extended.data());
}
}
