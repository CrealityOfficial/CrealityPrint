#include "DownloadService.h"

#include <thread>
#include <curl/curl.h>
#include <boost/nowide/fstream.hpp>
#include <boost/format.hpp>
#include <boost/log/trivial.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>

#include "format.hpp"
#include "GUI.hpp"
#include "I18N.hpp"
#include "libslic3r/Utils.hpp"

namespace Slic3r { namespace GUI {

const size_t DOWNLOAD_MAX_CHUNK_SIZE = 10 * 1024 * 1024;
const size_t DOWNLOAD_SIZE_LIMIT     = 1024 * 1024 * 1024;

struct DownloadService::priv
{
    const std::string       m_id;
    std::string             m_url;
    std::string             m_filename;
    std::thread             m_io_thread;
    boost::filesystem::path m_dest_folder;
    boost::filesystem::path m_tmp_path; // path when ongoing download
    std::atomic_bool        m_cancel{false};
    std::atomic_bool        m_pause{false};
    std::atomic_bool        m_stopped{false}; // either canceled or paused - download is not running
    size_t                  m_written{0};
    size_t                  m_absolute_size{0};
    priv(std::string                    ID,
         std::string&&                  url,
         const std::string&             filename,
         const boost::filesystem::path& dest_folder,
         std::function<void(int)> progress_callback = nullptr,
         std::function<void(std::string)> complete_callback = nullptr
        );
    std::function<void(int progress)> progress_callback_;
    std::function<void(int progress)> error_callback_;
    std::function<void(std::string path)> complete_callback_;

    void get_perform();
};

DownloadService::priv::priv(std::string                    ID,
                            std::string&&                  url,
                            const std::string&             filename,
                            const boost::filesystem::path& dest_folder,
                            std::function<void(int)>       progress_callback,
                            std::function<void(std::string)>       complete_callback)
    : m_id(ID), m_url(std::move(url)), m_filename(filename), m_dest_folder(dest_folder), progress_callback_(progress_callback),complete_callback_(complete_callback)
{}

void DownloadService::priv::get_perform()
{
    assert(!m_url.empty());
    assert(!m_filename.empty());
    assert(boost::filesystem::is_directory(m_dest_folder));

    m_stopped = false;

    // open dest file
    std::string extension;
    if (m_written == 0) {
        boost::filesystem::path dest_path = m_dest_folder / m_filename;
        extension                         = dest_path.extension().string();
        std::string just_filename         = m_filename.substr(0, m_filename.size() - extension.size());
        std::string final_filename        = just_filename;
        // Find unsed filename
        // try {
        //     size_t version = 0;
        //     while (boost::filesystem::exists(m_dest_folder / (final_filename + extension)) ||
        //            boost::filesystem::exists(m_dest_folder /
        //                                      (final_filename + extension + "." + std::to_string(get_current_pid()) + ".download"))) {
        //         ++version;
        //         if (version > 999) {
        //             return;
        //         }
        //         final_filename = GUI::format("%1%(%2%)", just_filename, std::to_string(version));
        //     }
        // } catch (const boost::filesystem::filesystem_error& e) {
        //     return;
        // }

        m_filename = sanitize_filename(final_filename + extension);

        m_tmp_path = m_dest_folder / (m_filename + "." + std::to_string(get_current_pid()) + ".download");
    }

    boost::filesystem::path dest_path;
    if (!extension.empty())
        dest_path = m_dest_folder / m_filename;

    wxString temp_path_wstring(m_tmp_path.wstring());

    // std::cout << "dest_path: " << dest_path.string() << std::endl;
    // std::cout << "m_tmp_path: " << m_tmp_path.string() << std::endl;

    BOOST_LOG_TRIVIAL(info) << GUI::format("Starting download from %1% to %2%. Temp path is %3%", m_url, dest_path, m_tmp_path);

    FILE* file;
    // open file for writting
    if (m_written == 0)
        file = fopen(temp_path_wstring.c_str(), "wb");
    else
        file = fopen(temp_path_wstring.c_str(), "ab");

    // assert(file != NULL);
    if (file == NULL) {
        return;
    }

    std::string range_string = std::to_string(m_written) + "-";

    size_t written_previously   = m_written;
    size_t written_this_session = 0;
    Http::set_extra_headers({});
    Http::get(m_url)
        .size_limit(DOWNLOAD_SIZE_LIMIT) // more?
        .set_range(range_string)
        .on_header_callback([&](std::string header) {
            //if (dest_path.empty()) {
            //    std::string filename = extract_remote_filename(header);
            //    if (!filename.empty()) {
            //        m_filename          = filename;
            //        dest_path           = m_dest_folder / m_filename;
            //    }
            //}
        })
        .on_progress([&](Http::Progress progress, bool& cancel) {
            // to prevent multiple calls into following ifs (m_cancel / m_pause)
            if (m_stopped) {
                cancel = true;
                return;
            }
            if (m_cancel) {
                m_stopped = true;
                fclose(file);
                // remove canceled file
                std::remove(m_tmp_path.string().c_str());
                m_written           = 0;
                cancel              = true;
                return;
                // TODO: send canceled event?
            }
            if (m_pause) {
                m_stopped = true;
                fclose(file);
                cancel = true;
                if (m_written == 0)
                    std::remove(m_tmp_path.string().c_str());
                return;
            }

            if (m_absolute_size < progress.dltotal) {
                m_absolute_size = progress.dltotal;
            }

            if (progress.dlnow != 0) {
                if (progress.dlnow - written_this_session > DOWNLOAD_MAX_CHUNK_SIZE || progress.dlnow == progress.dltotal) {
                    try {
                        std::string part_for_write = progress.buffer.substr(written_this_session, progress.dlnow);
                        fwrite(part_for_write.c_str(), 1, part_for_write.size(), file);
                    } catch (const std::exception& e) {
                        // fclose(file); do it?
                        cancel = true;
                        return;
                    }
                    written_this_session = progress.dlnow;
                    m_written            = written_previously + written_this_session;
                }
                int percent_total = m_absolute_size == 0 ? 0 : (written_previously + progress.dlnow) * 100 / m_absolute_size;
                if (progress_callback_) {
                    progress_callback_(percent_total);
                }
            }
        })
        .on_error([&](std::string body, std::string error, unsigned http_status) {
            if (file != NULL)
                fclose(file);
        })
        .on_complete([&](std::string body, unsigned /* http_status */) {
            // TODO: perform a body size check
            //
            // size_t body_size = body.size();
            // if (body_size != expected_size) {
            //	return;
            //}
            try {
                // Orca: thingiverse need this
                if (m_written < body.size()) {
                    // this code should never be entered. As there should be on_progress call after last bit downloaded.
                    std::string part_for_write = body.substr(m_written);
                    fwrite(part_for_write.c_str(), 1, part_for_write.size(), file);

                }
                fclose(file);
                boost::filesystem::rename(m_tmp_path, dest_path);
                if (complete_callback_) {
                    complete_callback_(dest_path.string());
                }
            } catch (const std::exception& /*e*/) {
                // TODO: report?
                // error_message = GUI::format("Failed to write and move %1% to %2%", tmp_path, dest_path);
                return;
            }

        })
        .perform_sync();
}

DownloadService::DownloadService(std::string                    ID,
                                 std::string                    url,
                                 const std::string&             filename,
                                 const boost::filesystem::path& dest_folder,
                                 std::function<void(int)>       progress_cb,
                                 std::function<void(std::string)>      complete_cb,
                                 std::function<void(void)>      error_cb)
    : p(new priv(ID, std::move(url), filename, dest_folder, progress_cb, complete_cb))
{}

DownloadService::DownloadService(DownloadService&& other) noexcept : p(std::move(other.p)) {}

DownloadService::~DownloadService()
{
    if (p && p->m_io_thread.joinable()) {
        p->m_cancel = true;
        p->m_io_thread.join();
    }
}

void DownloadService::get()
{
    assert(p);
    if (p->m_io_thread.joinable()) {
        // This will stop transfers being done by the thread, if any.
        // Cancelling takes some time, but should complete soon enough.
        p->m_cancel = true;
        p->m_io_thread.join();
    }
    p->m_cancel    = false;
    p->m_pause     = false;
    p->m_io_thread = std::thread([this]() { p->get_perform(); });
}

void DownloadService::cancel()
{
    if (p && p->m_stopped) {
        if (p->m_io_thread.joinable()) {
            p->m_cancel = true;
            p->m_io_thread.join();
        }
    }

    if (p)
        p->m_cancel = true;
}

void DownloadService::pause()
{
    if (p) {
        p->m_pause = true;
    }
}
void DownloadService::resume()
{
    assert(p);
    if (p->m_io_thread.joinable()) {
        // This will stop transfers being done by the thread, if any.
        // Cancelling takes some time, but should complete soon enough.
        p->m_cancel = true;
        p->m_io_thread.join();
    }
    p->m_cancel    = false;
    p->m_pause     = false;
    p->m_io_thread = std::thread([this]() { p->get_perform(); });
}

}} // namespace Slic3r::GUI
