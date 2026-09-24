#include "PrinterCover.hpp"
#include "Http.hpp"
#include "libslic3r/PrinterCover.hpp"
#include "libslic3r/Utils.hpp"

#include <atomic>
#include <chrono>
#include <cstring>
#include <map>
#include <mutex>
#include <vector>
#include <boost/nowide/fstream.hpp>
#include <wx/image.h>
#include <wx/mstream.h>
#include <wx/log.h>

#ifdef ENABLE_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}
#endif

namespace Slic3r {
namespace {
std::atomic<uint64_t> cover_revision{0};
std::mutex cover_mutex;

wxImage decode_cover(const std::string& body)
{
    wxLogNull suppress_image_errors;
    const bool webp = body.size() >= 12 && body.compare(0, 4, "RIFF") == 0 && body.compare(8, 4, "WEBP") == 0;
    if (!webp) {
        wxMemoryInputStream stream(body.data(), body.size());
        return wxImage(stream, wxBITMAP_TYPE_ANY);
    }
    // wxWidgets builds without a WebP handler can still use our video decoder.
#ifdef ENABLE_FFMPEG
    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_WEBP);
    if (!codec)
        return {};
    AVCodecContext* context = avcodec_alloc_context3(codec);
    AVFrame* frame = av_frame_alloc();
    AVPacket* packet = av_packet_alloc();
    wxImage image;
    if (context && frame && packet && avcodec_open2(context, codec, nullptr) >= 0 &&
        av_new_packet(packet, static_cast<int>(body.size())) >= 0) {
        std::memcpy(packet->data, body.data(), body.size());
        if (avcodec_send_packet(context, packet) >= 0 && avcodec_receive_frame(context, frame) >= 0 &&
            frame->width > 0 && frame->height > 0 && frame->width <= 4096 && frame->height <= 4096) {
            SwsContext* scaler = sws_getContext(frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
                                               frame->width, frame->height, AV_PIX_FMT_RGBA, SWS_BILINEAR,
                                               nullptr, nullptr, nullptr);
            if (scaler) {
                std::vector<unsigned char> pixels(static_cast<size_t>(frame->width) * frame->height * 4);
                uint8_t* dest[] = {pixels.data(), nullptr, nullptr, nullptr};
                int strides[] = {frame->width * 4, 0, 0, 0};
                if (sws_scale(scaler, frame->data, frame->linesize, 0, frame->height, dest, strides) == frame->height &&
                    image.Create(frame->width, frame->height)) {
                    image.InitAlpha();
                    for (size_t i = 0; i < pixels.size() / 4; ++i) {
                        std::memcpy(image.GetData() + i * 3, pixels.data() + i * 4, 3);
                        image.GetAlpha()[i] = pixels[i * 4 + 3];
                    }
                }
                sws_freeContext(scaler);
            }
        }
    }
    av_packet_free(&packet);
    av_frame_free(&frame);
    avcodec_free_context(&context);
    return image;
#else
    wxMemoryInputStream stream(body.data(), body.size());
    return wxImage(stream, wxBITMAP_TYPE_ANY);
#endif
}
} // namespace

uint64_t printer_cover_revision() { return cover_revision.load(); }

bool sync_printer_cover(const std::string& vendor, const std::string& model, const std::string& url)
{
    if (!valid_printer_cover_component(vendor) || !valid_printer_cover_component(model) ||
        (url.compare(0, 8, "https://") != 0 && url.compare(0, 7, "http://") != 0))
        return false;
    // Serialize duplicate nozzle packages and startup/update requests for the same file.
    std::lock_guard<std::mutex> lock(cover_mutex);
    const auto target = std::filesystem::u8path(data_dir()) / "system" /
                        std::filesystem::u8path(vendor) / std::filesystem::u8path(model + "_cover.png");
    const auto metadata = target.u8string() + ".url";
    const auto temporary = target.u8string() + ".tmp";
    try {
        const std::string existing = find_printer_cover(data_dir(), resources_dir(), vendor, model);
        std::string old_url;
        boost::nowide::ifstream input(metadata);
        std::getline(input, old_url);
        input.close();
        if (!existing.empty() && (old_url == url || (old_url.empty() && existing != target.u8string()))) {
            wxLogNull suppress_image_errors;
            wxImage cached(wxString::FromUTF8(existing.c_str()), wxBITMAP_TYPE_PNG);
            if (cached.IsOk())
                return false;
        }

        // Failed downloads may be retried on a later refresh without hammering the CDN.
        static std::map<std::string, std::chrono::steady_clock::time_point> attempts;
        const std::string key = target.u8string() + "\n" + url;
        const auto now = std::chrono::steady_clock::now();
        const auto previous = attempts.find(key);
        if (previous != attempts.end() && now - previous->second < std::chrono::minutes(5))
            return false;
        attempts[key] = now;

        std::string contents;
        Http::get(url).timeout_connect(2).timeout_max(10).size_limit(8 * 1024 * 1024)
            .on_complete([&](std::string body, unsigned status) {
                if (status == 200)
                    contents = std::move(body);
            })
            .on_error([&](std::string, std::string, unsigned status) {
                BOOST_LOG_TRIVIAL(warning) << "Printer cover download failed for " << model << ", status=" << status;
            }).perform_sync();
        if (contents.empty())
            return false;
        wxImage image = decode_cover(contents);
        if (!image.IsOk() || image.GetWidth() > 4096 || image.GetHeight() > 4096)
            return false;
        std::filesystem::create_directories(target.parent_path());
        if (!image.SaveFile(wxString::FromUTF8(temporary.c_str()), wxBITMAP_TYPE_PNG))
            throw std::runtime_error("Cannot save printer cover");
        // std::filesystem::rename cannot replace an existing file on Windows.
#ifdef _WIN32
        if (rename_file(temporary, target.u8string()))
            throw std::runtime_error("Cannot replace printer cover");
#else
        std::filesystem::rename(std::filesystem::u8path(temporary), target);
#endif
        boost::nowide::ofstream output(metadata, std::ios::trunc);
        output << url;
        ++cover_revision;
        return true;
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(warning) << "Printer cover update failed for " << model << ": " << e.what();
        std::error_code ec;
        std::filesystem::remove(std::filesystem::u8path(temporary), ec);
        return false;
    }
}
} // namespace Slic3r
