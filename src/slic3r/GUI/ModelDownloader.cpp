#include "ModelDownloader.h"
#include "GUI_App.hpp"
#include "NotificationManager.hpp"
#include "format.hpp"
#include "MainFrame.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/log/trivial.hpp>

namespace Slic3r { namespace GUI {

DownloadTask::DownloadTask(std::string                    ID,
                           std::string                    url,
                           const std::string&             filename,
                           const boost::filesystem::path& dest_folder,
                           std::function<void(int)>       progress_cb,
                           std::function<void(std::string)>      complete_cb,
                           std::function<void(void)>      error_cb)
    : m_id(ID), m_filename(filename), m_dest_folder(dest_folder)
{
    assert(boost::filesystem::is_directory(dest_folder));
    m_final_path = dest_folder / m_filename;
    m_file_get   = std::make_shared<DownloadService>(ID, url, m_filename, dest_folder, progress_cb, complete_cb);
}

void DownloadTask::start()
{
    m_state = DownloadState::DownloadOngoing;
    m_file_get->get();
}
void DownloadTask::cancel()
{
    m_state = DownloadState::DownloadStopped;
    m_file_get->cancel();
}
void DownloadTask::pause()
{
    if (m_state != DownloadState::DownloadOngoing)
        return;
    m_state = DownloadState::DownloadPaused;
    m_file_get->pause();
}
void DownloadTask::resume()
{
    if (m_state != DownloadState::DownloadPaused)
        return;
    m_state = DownloadState::DownloadOngoing;
    m_file_get->resume();
}

ModelDownloader::ModelDownloader() {}

ModelDownloader::ModelDownloader(const std::string& user_id) : user_id_(user_id) { init(); }

void ModelDownloader::init()
{
    if (user_id_.empty()) {
        return;
    }
    load_cache_from_storage();
}

void ModelDownloader::start_download_model_group(const std::string& full_url,
                                                 const std::string& modelId,
                                                 const std::string& fileId,
                                                 const std::string& fileFormat,
                                                 const std::string& fileName
                                                 
    )
{
    auto target_path = dest_folder_;
    if (!fs::exists(target_path.append(modelId))) {
        fs::create_directories(target_path);
    }
    auto cache_path  = target_path;
    auto file_path   = cache_path.append(fileName + fileFormat).string();
    auto progress_cb = [&, modelId, fileId, file_path = std::move(file_path)](int progress) {
        std::lock_guard<std::mutex> lock_guard(cache_json_mutex_);
        if (cache_json_.is_object() && cache_json_.contains("models")) {
            for (auto& model : cache_json_["models"]) {
                std::string model_id = model["modelId"];
                if (model_id != modelId) {
                    continue;
                }
                for (auto& file : model["files"]) {
                    std::string file_id = file["fileId"];
                    if (file_id != fileId) {
                        continue;
                    }
                    auto& cache_progress = file["progress"];
                    if (cache_progress < progress) {
                        cache_progress = progress;
                        if (progress == 100) {
                            file["path"]    = file_path;
                        }
                        save_cache_to_storage();
                        return;
                    }
                }
            }
        }
    };
    download_tasks_.emplace_back(std::make_unique<DownloadTask>(modelId, full_url, fileName + fileFormat, target_path, progress_cb));
    download_tasks_.back()->start();
    BOOST_LOG_TRIVIAL(debug) << "started download";
    std::lock_guard<std::mutex> lock_guard(cache_json_mutex_);
    if (cache_json_.is_object() && cache_json_.contains("models")) {
        bool found = false;
        for (auto& model : cache_json_["models"]) {
            std::string model_id = model["modelId"];
            if (model_id == modelId) {
                auto& files_array = model["files"];
                json  object;
                object["fileId"]   = fileId;
                object["progress"] = 0;
                object["path"]     = "";
                files_array.push_back(object);
                found = true;
                break;
            }
        }
        if (!found) {
            auto&          model_array = cache_json_["models"];
            nlohmann::json model_object;
            model_object["createTime"] = time(nullptr);
            model_object["modelId"]    = modelId;
            json files_array;
            json file_object;
            file_object["fileId"]   = fileId;
            file_object["progress"] = 0;
            file_object["path"]     = "";
            files_array.push_back(file_object);
            model_object["files"] = files_array;
            model_array.push_back(model_object);
        }
    } else {
        nlohmann::json model_array;
        nlohmann::json model_object;
        model_object["createTime"] = time(nullptr);
        model_object["modelId"]    = modelId;
        json files_array;
        json file_object;
        file_object["fileId"]   = fileId;
        file_object["progress"] = 0;
        file_object["path"]     = "";
        files_array.push_back(file_object);
        model_object["files"] = files_array;
        model_array.push_back(model_object);
        cache_json_["models"] = model_array;
    }

    save_cache_to_storage();
}


void ModelDownloader::start_download_3mf_group(const std::string& full_url,
                                               const std::string& modelId,
                                               const std::string& fileId,
                                               const std::string& fileFormat,
                                               const std::string& name)
{
    //wxGetApp().mainframe->select_tab((size_t) MainFrame::TabPosition::tp3DEditor);
    auto target_path = cache_3mf_folder_;
    /* if (!fs::exists(target_path.append(modelId))) {
         fs::create_directories(target_path);
     }*/
    auto cache_path  = target_path;
    auto file_path   = cache_path.append(name + fileFormat).string();
    auto progress_cb = [&, modelId, fileId, file_path = std::move(file_path)](int progress) {
        std::lock_guard<std::mutex> lock_guard(cache_json_mutex_);
        if (cache_json_.is_object() && cache_json_.contains("3mfs")) {
            for (auto& file : cache_json_["3mfs"]) {
                if (file["fileId"] != fileId)
                    continue;
                auto& cache_progress = file["progress"];
                if (cache_progress < progress) {
                    cache_progress = progress;
                    if (progress == 100) {
                        file["path"]                        = file_path;
                    }
                    save_cache_to_storage();
                    return;
                }
            }
        }
    };

    auto complete_cb = [&, modelId, fileId, file_path = std::move(file_path)](std::string path) {
        std::lock_guard<std::mutex> lock_guard(cache_json_mutex_);
        if (cache_json_.is_object() && cache_json_.contains("3mfs")) {
            boost::filesystem::path target_path = fs::path(path);
            /*Plater* plater                      = wxGetApp().plater();
            plater->load_project(target_path.wstring());
            plater->set_project_filename(target_path.wstring());
            plater->get_notification_manager()->push_import_finished_notification(target_path.string(), target_path.parent_path().string(),
                                                                              false);*/
        }
    };

    download_tasks_.emplace_back(
        std::make_unique<DownloadTask>(modelId, full_url, name + fileFormat, target_path, progress_cb, complete_cb));
    download_tasks_.back()->start();
    BOOST_LOG_TRIVIAL(debug) << "started download";
    std::lock_guard<std::mutex> lock_guard(cache_json_mutex_);
    if (cache_json_.is_object() && cache_json_.contains("3mfs")) {
        bool found = false;
        for (auto& file : cache_json_["3mfs"]) {
            std::string file_id = file["fileId"];
            if (file_id == fileId) {
                file["fileId"]     = fileId;
                file["progress"] = 0;
                file["path"]       = "";
                file["name"]       = name;
                file["modelGroupId"] = modelId;
                found = true;
                break;
            }
        }
        if (!found) {
            auto&          model_array = cache_json_["3mfs"];
            nlohmann::json model_object;
            model_object["createTime"] = time(nullptr);
            model_object["fileId"]     = fileId;
            json files_array;
            model_object["progress"] = 0;
            model_object["path"]     = "";
            model_object["name"]     = name;
            model_object["modelGroupId"] = modelId;
            model_array.push_back(model_object);
        }
    } else {
        nlohmann::json model_array;
        nlohmann::json model_object;
        model_object["createTime"] = time(nullptr);
        model_object["fileId"]     = fileId;
        json file_object;
        model_object["progress"] = 0;
        model_object["path"]     = "";
        model_object["name"]     = name;
        model_object["modelGroupId"] = modelId;
        model_array.push_back(model_object);
        cache_json_["3mfs"] = model_array;
    }

    save_cache_to_storage();
}


void ModelDownloader::cancel_download_model_group(const std::string& modelId)
{
    for (auto& task : download_tasks_) {
        if (task->get_id() == modelId) {
            task->cancel();
        }
    }

    std::lock_guard<std::mutex> lock_guard(cache_json_mutex_);
    if (cache_json_.is_object() && cache_json_.contains("models")) {
        bool found = false;
        int  idx   = -1;
        std::string removePath = "";
        for (auto& model : cache_json_["models"]) {
            idx++;
            std::string model_id = model["modelId"];
           
        
            if (model_id == modelId) {
                auto model_files = model["files"];
                std::string path = "";
                if (model_files.size()) 
                    path = model_files.at(0)["path"];
                if (path != "") {
                    size_t pos = path.find_last_of('\\');
                    if (pos != std::string::npos) {
                        path = path.substr(0, pos);
                    }
                    fs::remove_all(path);
                }
                found = true;
                break;
            }
        }
        if (found) {
            cache_json_["models"].erase(idx);
            save_cache_to_storage();
        }
    }
}

void ModelDownloader::cancel_download_3mf_group(const std::string& fileId)
{
    for (auto& task : download_tasks_) {
        if (task->get_id() == fileId) {
            task->cancel();
        }
    }

    std::lock_guard<std::mutex> lock_guard(cache_json_mutex_);
    if (cache_json_.is_object() && cache_json_.contains("3mfs")) {
        bool found = false;
        int  idx   = -1;
        std::string removePath = "";
        for (auto& file : cache_json_["3mfs"]) {
            idx++;
            std::string file_id = file["fileId"];
            removePath          = file["path"];
            if (file_id == fileId) {
                found = true;
                break;
            }
        }
        if (found) {
            cache_json_["3mfs"].erase(idx);
            fs::remove(removePath);
            save_cache_to_storage();
        }
    }
}

nlohmann::json ModelDownloader::get_cache_json()
{
    std::lock_guard<std::mutex> lock_guard(cache_json_mutex_);
    return cache_json_;
}

void ModelDownloader::load_cache_from_storage() {
    dest_folder_ = fs::path(data_dir());
    dest_folder_.append("cloud_download_data").append(user_id_);

    cache_file_ = dest_folder_;
    boost::filesystem::path cache_id_file_ = dest_folder_;
    if (fs::exists(cache_file_.append("cloud_download_data.json"))) {
        boost::nowide::ifstream ifs(cache_file_.string());
        ifs >> cache_json_;
    }
    cache_id_file_.append("3mfs");
    if (!fs::exists(cache_id_file_)) {
        fs::create_directories(cache_id_file_);
    }

    cache_3mf_folder_ = cache_id_file_;

    dest_folder_.append("models");
    if (!fs::exists(dest_folder_)) {
        fs::create_directories(dest_folder_);
    }
}

void ModelDownloader::save_cache_to_storage()
{
    boost::nowide::ofstream c;
    c.open(cache_file_.string(), std::ios::out | std::ios::trunc);
    c << std::setw(4) << cache_json_ << std::endl;
    c.close();
}

}} // namespace Slic3r::GUI