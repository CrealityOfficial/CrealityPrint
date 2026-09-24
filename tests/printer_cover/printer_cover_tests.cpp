#include "slic3r/GUI/PrinterCoverRoute.hpp"
#include "libslic3r/PrinterCover.hpp"
#include "slic3r/Utils/PrinterCover.hpp"
#include "slic3r/Utils/Http.hpp"
#include <wx/init.h>
#include <wx/image.h>
#include <wx/mstream.h>
#include <boost/nowide/fstream.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;
namespace {
std::string test_data, test_resources;
std::map<std::string, std::pair<unsigned, std::string>> responses;
int requests = 0;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
std::string read(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void write(const fs::path& p, const std::string& value) {
    fs::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary); out << value;
}
std::string encode(wxBitmapType type, unsigned char red) {
    wxImage image(2, 2); image.SetRGB(wxRect(0, 0, 2, 2), red, 80, 120);
    wxMemoryOutputStream out;
    require(image.SaveFile(out, type), "encode fixture");
    std::string bytes(out.GetSize(), '\0'); out.CopyTo(bytes.data(), bytes.size()); return bytes;
}
}

// Replace only the transport and application directories; exercise the production cache and decoder.
namespace Slic3r {
const std::string& data_dir() { return test_data; }
const std::string& resources_dir() { return test_resources; }
std::error_code rename_file(const std::string& from, const std::string& to) {
#ifdef _WIN32
    if (MoveFileExW(fs::u8path(from).c_str(), fs::u8path(to).c_str(), MOVEFILE_REPLACE_EXISTING)) return {};
    return {static_cast<int>(GetLastError()), std::system_category()};
#else
    std::error_code ec; fs::rename(fs::u8path(from), fs::u8path(to), ec); return ec;
#endif
}
struct Http::priv { std::string url; CompleteFn complete; ErrorFn error; };
Http::Http(const std::string& url) : p(new priv{url, {}, {}}) {}
Http::~Http() = default;
Http Http::get(std::string url) { return Http(url); }
Http& Http::timeout_connect(long) { return *this; }
Http& Http::timeout_max(long) { return *this; }
Http& Http::size_limit(size_t) { return *this; }
Http& Http::on_complete(CompleteFn fn) { p->complete = std::move(fn); return *this; }
Http& Http::on_error(ErrorFn fn) { p->error = std::move(fn); return *this; }
void Http::perform_sync() {
    ++requests;
    const auto response = responses.at(p->url);
    if (response.first == 200) p->complete(response.second, response.first);
    else p->error(response.second, "fixture error", response.first);
}
}

int main() {
    wxInitializer init;
    if (!init.IsOk()) return 1;
    wxInitAllImageHandlers();
    const auto root = fs::temp_directory_path() / fs::u8path("printer-cover-中文-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        test_data = (root / "data").u8string(); test_resources = (root / "resources").u8string();
        const auto user = root / "data/system/Creality/Creality K3_cover.png";
        const auto builtin = root / "resources/profiles/Creality/Creality K3_cover.png";
        const auto png = encode(wxBITMAP_TYPE_PNG, 200);
        const auto jpeg = encode(wxBITMAP_TYPE_JPEG, 30);
        responses["https://test.invalid/first"] = {200, jpeg};
        responses["https://test.invalid/new"] = {200, png};
        responses["https://test.invalid/bad"] = {200, "<html>invalid image</html>"};
        responses["https://test.invalid/fail"] = {503, "unavailable"};
        require(Slic3r::find_printer_cover(test_data, test_resources, "Creality", "Creality K3").empty(), "missing cover");
        write(builtin, png);
        require(fs::equivalent(fs::u8path(Slic3r::find_printer_cover(test_data, test_resources, "Creality", "Creality K3")), builtin), "builtin fallback");
        require(!Slic3r::sync_printer_cover("Creality", "Creality K3", "https://test.invalid/first") && requests == 0, "keep valid bundled cover");
        fs::remove(builtin);
        require(Slic3r::sync_printer_cover("Creality", "Creality K3", "https://test.invalid/first"), "new model JPEG download");
        require(read(user).compare(0, 8, "\x89PNG\r\n\x1a\n") == 0, "real PNG conversion");
        write(builtin, png);
        require(fs::equivalent(fs::u8path(Slic3r::find_printer_cover(test_data, test_resources, "Creality", "Creality K3")), user), "user image precedes bundled image");
        const auto route = [](const std::string& vendor, const std::string& model) {
            const auto url = Slic3r::GUI::printer_cover_url(13666, vendor, model);
            return url.substr(url.find('/', url.find("://") + 3));
        };
        const auto resolve = [&](const std::string& path) {
            return Slic3r::GUI::printer_cover_route_path(path, test_data, test_resources);
        };
        const auto cover_route = route("Creality", "Creality K3");
        require(fs::equivalent(fs::u8path(resolve(cover_route)), user), "HTTP route prefers user cover");
        fs::remove(user);
        require(fs::equivalent(fs::u8path(resolve(cover_route)), builtin), "HTTP route falls back to builtin");
        fs::remove(builtin);
        require(fs::u8path(resolve(cover_route)) == root / "resources/images/printer_default.png", "HTTP route falls back to default");
        write(builtin, png); write(user, png);
        require(fs::equivalent(fs::u8path(resolve(cover_route)), user), "same URL finds newly available cover");
        const std::string unicode_model = u8"测试 mBox #1% &+";
        const auto unicode_cover = root / "data/system/Creality" / fs::u8path(unicode_model + "_cover.png");
        write(unicode_cover, png);
        require(fs::equivalent(fs::u8path(resolve(route("Creality", unicode_model))), unicode_cover), "HTTP route preserves UTF-8 and URL special characters");
        require(resolve(route("../escape", "model")).empty(), "HTTP route rejects vendor traversal");
        require(resolve(route("Creality", "../escape")).empty(), "HTTP route rejects model traversal");
        require(resolve(route("Creality", std::string("a\0b", 3))).empty(), "HTTP route rejects NUL");
        require(resolve(cover_route + "/extra").empty(), "HTTP route rejects extra path segments");
        require(resolve("/printer-cover/43/xyz").empty(), "HTTP route rejects invalid hex");
        require(resolve("/printer-cover/43").empty(), "HTTP route rejects incomplete path");
        const int count = requests;
        require(!Slic3r::sync_printer_cover("Creality", "Creality K3", "https://test.invalid/first") && requests == count, "deduplicate nozzle packages");
        const auto revision = Slic3r::printer_cover_revision();
        require(Slic3r::sync_printer_cover("Creality", "Creality K3", "https://test.invalid/new"), "changed URL replaces cover");
        require(Slic3r::printer_cover_revision() == revision + 1, "same-model texture invalidation");
        const auto good = read(user);
        require(!Slic3r::sync_printer_cover("Creality", "Creality K3", "https://test.invalid/bad") && read(user) == good, "bad image preserves old cover");
        require(!Slic3r::sync_printer_cover("Creality", "Creality K3", "https://test.invalid/fail") && read(user) == good, "HTTP failure preserves old cover");
        const int failed_count = requests;
        require(!Slic3r::sync_printer_cover("Creality", "Creality K3", "https://test.invalid/fail") && requests == failed_count, "failure cooldown");
        require(!Slic3r::sync_printer_cover("Creality", "Creality K3", ""), "missing thumbnail is optional");
        require(!Slic3r::sync_printer_cover("../escape", "Creality K3", "https://test.invalid/new"), "reject unsafe vendor");
        require(!Slic3r::sync_printer_cover("Creality", "../escape", "https://test.invalid/new"), "reject unsafe model");
        const auto broken = root / "data/system/Creality/Creality New_cover.png";
        write(broken, "broken"); write(fs::u8path(broken.u8string() + ".url"), "https://test.invalid/new");
        require(Slic3r::sync_printer_cover("Creality", "Creality New", "https://test.invalid/new"), "repair corrupt cover without parameter version change");
#ifdef ENABLE_FFMPEG
        const unsigned char webp[] = {82, 73, 70, 70, 30, 0, 0, 0, 87, 69, 66, 80, 86, 80, 56, 76, 17, 0, 0, 0, 47, 1, 64, 0, 16, 7, 80, 168, 146, 21, 175, 128, 129, 136, 232, 127, 0, 0};
        responses["https://test.invalid/webp"] = {200, std::string(reinterpret_cast<const char*>(webp), sizeof(webp))};
        require(Slic3r::sync_printer_cover("Creality", "Creality WebP", "https://test.invalid/webp"), "WebP conversion");
        wxImage decoded(wxString::FromUTF8((root / "data/system/Creality/Creality WebP_cover.png").u8string().c_str()), wxBITMAP_TYPE_PNG);
        require(decoded.IsOk() && decoded.HasAlpha() && decoded.GetAlpha(0, 0) == 128, "preserve WebP alpha");
#endif
        fs::remove_all(root);
        std::cout << "Printer cover tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n'; fs::remove_all(root); return 1;
    }
}
