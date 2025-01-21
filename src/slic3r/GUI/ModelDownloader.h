#ifndef MODEL_DOWNLOADER_H
#define MODEL_DOWNLOADER_H

#include "Downloader.hpp"
#include "DownloadService.h"
#include <boost/filesystem/path.hpp>
#include "nlohmann/json.hpp"

namespace Slic3r { namespace GUI {

class DownloadTask
{
public:
    DownloadTask(std::string                    ID,
                 std::string                    url,
                 const std::string&             filename,
                 const boost::filesystem::path& dest_folder,
                 std::function<void(int)>       progress_cb = nullptr,
                 std::function<void(std::string)>       complete_cb = nullptr,
                 std::function<void(void)>       error_cb = nullptr);
    void start();
    void cancel();
    void pause();
    void resume();

    std::string             get_id() const { return m_id; }
    boost::filesystem::path get_final_path() const { return m_final_path; }
    std::string             get_filename() const { return m_filename; }
    DownloadState           get_state() const { return m_state; }
    void                    set_state(DownloadState state) { m_state = state; }
    std::string             get_dest_folder() { return m_dest_folder.string(); }

private:
    const std::string                m_id;
    std::string                      m_filename;
    boost::filesystem::path          m_final_path;
    boost::filesystem::path          m_dest_folder;
    std::shared_ptr<DownloadService> m_file_get;
    DownloadState                    m_state{DownloadState::DownloadPending};
};

class ModelDownloader
{
public:
    ModelDownloader();
    ModelDownloader(const std::string& user_id);

    void set_user_id(const std::string& user_id) { user_id_ = user_id; }
    void init();
    void start_download_model_group(const std::string& full_url,
                                    const std::string& modelId,
                                    const std::string& fileId,
                                    const std::string& fileFormat,
                                    const std::string& fileName);
    void start_download_3mf_group(const std::string& full_url,
                                    const std::string& modelId,
                                    const std::string& fileId,
                                    const std::string& fileFormat,
                                    const std::string& name);
    void cancel_download_model_group(const std::string& modelId);
    void cancel_download_3mf_group(const std::string& fileId);
    
    nlohmann::json get_cache_json();

private:
    void        load_cache_from_storage();
    void        save_cache_to_storage();
    std::string user_id_;

    std::list<std::unique_ptr<DownloadTask>> download_tasks_;
    boost::filesystem::path                  dest_folder_;
    boost::filesystem::path                  cache_3mf_folder_;
    boost::filesystem::path                  cache_file_;
    nlohmann::json                           cache_json_;
    std::mutex                               cache_json_mutex_;
};

}} // namespace Slic3r::GUI
#endif