#include "slic3r/GUI/FileDownloader.hpp"
#include <boost/nowide/args.hpp>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

void require(bool value, const char* message)
{
    if (!value)
        throw std::runtime_error(message);
}

int main(int argc, char** argv)
{
    boost::nowide::args utf8_args(argc, argv);
    if (argc != 5)
        return 2;
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
        return 3;
    int result = 0;
    try {
        const std::string base = argv[1];
        const fs::path output = fs::u8path(argv[2]);
        const int count = std::stoi(argv[3]);
        const std::string mode = argv[4];
        CurlConnectionPool pool(mode == "zero" ? 0 : 3);
#ifdef _WIN32
        DWORD initial_handles = 0;
        GetProcessHandleCount(GetCurrentProcess(), &initial_handles);
#endif
        for (int i = 0; i < count; ++i) {
            const std::string route = mode == "cancel" ? "/slow/" : mode == "failure" && i == 0 ? "/broken/" : "/file/";
            const fs::path filename = output / (std::to_string(i) + ".json");
            require(pool.addDownload(base + route + std::to_string(i), filename.u8string()), "queue failed");
            require(!fs::exists(filename), "queue opened a file before downloading");
        }
        const auto start = std::chrono::steady_clock::now();
        bool checked_handles = false;
        const bool success = pool.performDownloads([&] {
#ifdef _WIN32
            DWORD handles = 0;
            GetProcessHandleCount(GetCurrentProcess(), &handles);
            require(handles <= initial_handles + 48, "unbounded file/socket handles");
#endif
            checked_handles = true;
            return mode == "cancel" && std::chrono::steady_clock::now() - start > std::chrono::milliseconds(200);
        });
        require(checked_handles || count == 0, "cancellation check was never called");
        require(success == (mode != "failure" && mode != "cancel" && mode != "open_failure"), "incorrect result");
        if (mode == "cancel")
            require(std::chrono::steady_clock::now() - start < std::chrono::seconds(2), "cancellation was not prompt");
        // Exercise reuse and immediate completion of an empty batch.
        require(pool.performDownloads(), "empty/reused batch failed");
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        result = 1;
    }
    curl_global_cleanup();
    return result;
}
