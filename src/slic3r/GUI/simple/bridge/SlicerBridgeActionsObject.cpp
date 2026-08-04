#include "SlicerBridge.hpp"

#include "slic3r/GUI/GUI.hpp"
#include "slic3r/GUI/format.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "slic3r/GUI/Tab.hpp"
#include "slic3r/GUI/NotificationManager.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/ModelObject.hpp"
#include "libslic3r/ModelVolume.hpp"
#include "libslic3r/ModelInstance.hpp"
#include "libslic3r/CutUtils.hpp"
#include "libslic3r/Geometry.hpp"
#include "libslic3r/QuadricEdgeCollapse.hpp"
#include "slic3r/GUI/Selection.hpp"
#include "slic3r/GUI/GUI_ObjectList.hpp"
#include "slic3r/GUI/GLCanvas3D.hpp"
#include "slic3r/GUI/WebModelLibraryView.hpp"
#include "slic3r/GUI/print_manage/data/DataCenter.hpp"
#include "slic3r/Utils/Http.hpp"

#include <boost/log/trivial.hpp>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

using json = nlohmann::json;

namespace Slic3r {
namespace GUI {
namespace Bridge {

json SlicerBridge::DoSetObjectColor(const json& params)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    Model& model = plater->model();
    std::vector<int> valid_indices;
    valid_indices.reserve(model.objects.size());
    for (int i = 0; i < (int)model.objects.size(); ++i) {
        if (model.objects[i] != nullptr)
            valid_indices.push_back(i);
    }
    if (valid_indices.empty())
        return {{"success", false}, {"message", "No model loaded"}};

    auto trim = [](std::string s) {
        const auto is_ws = [](unsigned char ch) { return std::isspace(ch) != 0; };
        while (!s.empty() && is_ws((unsigned char)s.front())) s.erase(s.begin());
        while (!s.empty() && is_ws((unsigned char)s.back())) s.pop_back();
        return s;
    };

    auto to_lower_ascii = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return (char)std::tolower(c);
        });
        return s;
    };

    auto parse_int = [&trim](const json& value, int& out) -> bool {
        try {
            if (value.is_number_integer()) {
                out = value.get<int>();
                return true;
            }
            if (value.is_number()) {
                out = (int)std::llround(value.get<double>());
                return true;
            }
            if (value.is_string()) {
                const std::string s = trim(value.get<std::string>());
                if (s.empty())
                    return false;
                out = std::stoi(s);
                return true;
            }
        } catch (...) {
        }
        return false;
    };

    std::vector<int> target_obj_indices;
    std::unordered_set<int> seen;
    auto add_target = [&](int idx) {
        if (idx >= 0 && idx < (int)model.objects.size() && model.objects[idx] && seen.insert(idx).second)
            target_obj_indices.push_back(idx);
    };

    if (params.contains("object_indices")) {
        const json& indices = params["object_indices"];
        if (indices.is_array()) {
            for (const auto& item : indices) {
                int idx = -1;
                if (parse_int(item, idx))
                    add_target(idx);
            }
        } else {
            int idx = -1;
            if (parse_int(indices, idx))
                add_target(idx);
        }
    }

    if (params.contains("object_index")) {
        int idx = -1;
        if (parse_int(params["object_index"], idx))
            add_target(idx);
    }

    auto collect_by_name = [&](const std::string& input_name) {
        const std::string target_name = trim(input_name);
        if (target_name.empty())
            return;

        const size_t before = target_obj_indices.size();
        for (int i = 0; i < (int)model.objects.size(); ++i) {
            if (model.objects[i] && model.objects[i]->name == target_name)
                add_target(i);
        }

        if (target_obj_indices.size() == before) {
            const std::string target_name_lower = to_lower_ascii(target_name);
            for (int i = 0; i < (int)model.objects.size(); ++i) {
                if (!model.objects[i])
                    continue;
                if (to_lower_ascii(model.objects[i]->name) == target_name_lower)
                    add_target(i);
            }
        }
    };

    if (params.contains("object_name") && params["object_name"].is_string())
        collect_by_name(params["object_name"].get<std::string>());

    if (params.contains("object_names") && params["object_names"].is_array()) {
        for (const auto& item : params["object_names"]) {
            if (item.is_string())
                collect_by_name(item.get<std::string>());
        }
    }

    if (target_obj_indices.empty()) {
        if (auto* canvas = plater->canvas3D()) {
            const Selection& selection = canvas->get_selection();
            const auto& selected_content = selection.get_content();
            for (const auto& kv : selected_content)
                add_target(kv.first);
        }
    }

    if (target_obj_indices.empty() && valid_indices.size() == 1)
        add_target(valid_indices.front());

    if (target_obj_indices.empty())
        return {{"success", false}, {"message", "object_name/object_index/object_indices is required when multiple objects exist"}};

    const std::vector<std::string> extruder_colors = plater->get_extruder_colors_from_plater_config();

    auto normalize_hex = [](std::string s) -> std::string {
        s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char ch) { return std::isspace(ch) != 0; }), s.end());
        if (!s.empty() && s.front() == '#') s.erase(s.begin());
        if (s.size() == 3) {
            std::string x;
            x.reserve(6);
            x.push_back(s[0]); x.push_back(s[0]);
            x.push_back(s[1]); x.push_back(s[1]);
            x.push_back(s[2]); x.push_back(s[2]);
            s = x;
        }
        if (s.size() != 6) return "";
        for (unsigned char ch : s) {
            if (!std::isxdigit(ch)) return "";
        }
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        return "#" + s;
    };

    auto hex_to_rgb = [&](const std::string& hex, int& r, int& g, int& b) -> bool {
        std::string n = normalize_hex(hex);
        if (n.empty()) return false;
        try {
            r = std::stoi(n.substr(1, 2), nullptr, 16);
            g = std::stoi(n.substr(3, 2), nullptr, 16);
            b = std::stoi(n.substr(5, 2), nullptr, 16);
            return true;
        } catch (...) {
            return false;
        }
    };

    int target_extruder_id = 0;
    if (params.contains("extruder_id")) {
        if (params["extruder_id"].is_number_integer())
            target_extruder_id = params["extruder_id"].get<int>();
        else if (params["extruder_id"].is_string()) {
            try { target_extruder_id = std::stoi(params["extruder_id"].get<std::string>()); }
            catch (...) {}
        }
    }

    std::string input_color = trim(params.value("color", ""));
    if (input_color.empty())
        input_color = trim(params.value("color_name", ""));

    if (target_extruder_id <= 0 && !input_color.empty()) {
        const std::string color_lower = to_lower_ascii(input_color);
        static const std::unordered_map<std::string, std::string> kColorAliases = {
            {"red", "#ff0000"}, {"green", "#00ff00"}, {"blue", "#0000ff"},
            {"yellow", "#ffff00"}, {"black", "#000000"}, {"white", "#ffffff"},
            {"orange", "#ffa500"}, {"purple", "#800080"}, {"pink", "#ff69b4"},
            {"gray", "#808080"}, {"grey", "#808080"}, {"cyan", "#00ffff"},
            {"teal", "#008080"},
            {"hong", "#ff0000"}, {"lv", "#00ff00"}, {"lan", "#0000ff"},
            {"huang", "#ffff00"}, {"hei", "#000000"}, {"bai", "#ffffff"},
            {"cheng", "#ffa500"}, {"zi", "#800080"}, {"hui", "#808080"},
            {"qing", "#00ffff"},
            {"hongse", "#ff0000"}, {"lvse", "#00ff00"}, {"lanse", "#0000ff"},
            {"huangse", "#ffff00"}, {"heise", "#000000"}, {"baise", "#ffffff"},
            {"chengse", "#ffa500"}, {"zise", "#800080"}, {"huise", "#808080"},
            {"qingse", "#00ffff"}
        };

        std::string wanted_hex;
        auto it_alias = kColorAliases.find(color_lower);
        if (it_alias != kColorAliases.end())
            wanted_hex = it_alias->second;
        else {
            auto it_raw_alias = kColorAliases.find(input_color);
            if (it_raw_alias != kColorAliases.end())
                wanted_hex = it_raw_alias->second;
            else
                wanted_hex = normalize_hex(input_color);
        }

        if (wanted_hex.empty()) {
            bool is_digit_string = !color_lower.empty() &&
                                   std::all_of(color_lower.begin(), color_lower.end(),
                                               [](unsigned char c) { return std::isdigit(c) != 0; });
            if (is_digit_string) {
                try { target_extruder_id = std::stoi(color_lower); } catch (...) {}
            } else {
                return {{"success", false}, {"message", "Invalid color value: " + input_color}};
            }
        } else {
            if (extruder_colors.empty()) {
                return {{"success", false}, {"message", "No filament colors available for color mapping"}};
            }

            int wr = 0, wg = 0, wb = 0;
            if (!hex_to_rgb(wanted_hex, wr, wg, wb))
                return {{"success", false}, {"message", "Invalid color value: " + input_color}};

            int best_idx = -1;
            int best_dist = std::numeric_limits<int>::max();

            for (int i = 0; i < (int)extruder_colors.size(); ++i) {
                int cr = 0, cg = 0, cb = 0;
                if (!hex_to_rgb(extruder_colors[i], cr, cg, cb))
                    continue;
                const int dr = cr - wr;
                const int dg = cg - wg;
                const int db = cb - wb;
                const int dist = dr * dr + dg * dg + db * db;
                if (dist < best_dist) {
                    best_dist = dist;
                    best_idx = i;
                }
            }

            if (best_idx < 0) {
                return {{"success", false}, {"message", "Failed to map color to an available filament color"}};
            }
            target_extruder_id = best_idx + 1;
        }
    }

    if (target_extruder_id <= 0)
        return {{"success", false}, {"message", "extruder_id or color is required"}};

    if (!extruder_colors.empty() && target_extruder_id > (int)extruder_colors.size()) {
        return {{"success", false},
                {"message", "extruder_id out of range: " + std::to_string(target_extruder_id) +
                            " (available: 1-" + std::to_string((int)extruder_colors.size()) + ")"}};
    }

    std::string snapshot_name = "Set object color";
    if (target_obj_indices.size() == 1) {
        const int idx = target_obj_indices.front();
        if (idx >= 0 && idx < (int)model.objects.size() && model.objects[idx] && !model.objects[idx]->name.empty())
            snapshot_name = "Set color for " + model.objects[idx]->name;
    } else {
        snapshot_name = "Set color for multiple objects";
    }
    plater->take_snapshot(snapshot_name);

    int affected_objects = 0;
    int affected_volumes_total = 0;
    std::vector<int> applied_indices;
    json updated_objects = json::array();

    for (int idx : target_obj_indices) {
        ModelObject* obj = model.objects[idx];
        if (!obj)
            continue;

        obj->config.set("extruder", target_extruder_id);

        int affected_volumes = 0;
        for (ModelVolume* vol : obj->volumes) {
            if (!vol || !vol->is_model_part())
                continue;
            vol->config.set("extruder", target_extruder_id);
            ++affected_volumes;
        }

        ++affected_objects;
        affected_volumes_total += affected_volumes;
        applied_indices.push_back(idx);

        json item;
        item["object_index"] = idx;
        item["object_name"] = obj->name.empty() ? ("object_" + std::to_string(idx)) : obj->name;
        item["affected_volumes"] = affected_volumes;
        updated_objects.push_back(std::move(item));
    }

    if (affected_objects == 0)
        return {{"success", false}, {"message", "No valid target objects to set color"}};

    plater->update();

    json out;
    out["success"] = true;
    out["message"] = (affected_objects == 1)
        ? "Object color updated"
        : ("Updated color for " + std::to_string(affected_objects) + " objects");
    out["object_count"] = affected_objects;
    out["object_indices"] = applied_indices;
    out["updated_objects"] = updated_objects;
    out["extruder_id"] = target_extruder_id;
    out["affected_volumes"] = affected_volumes_total;
    if (affected_objects == 1) {
        out["object_index"] = applied_indices.front();
        const ModelObject* only_obj = model.objects[applied_indices.front()];
        out["object_name"] = (only_obj && !only_obj->name.empty()) ? only_obj->name : ("object_" + std::to_string(applied_indices.front()));
    }
    if (target_extruder_id >= 1 && target_extruder_id <= (int)extruder_colors.size())
        out["color"] = extruder_colors[target_extruder_id - 1];
    if (!input_color.empty())
        out["requested_color"] = input_color;
    return out;
}

json SlicerBridge::DoImportModel(const json& params)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    std::string path = params.value("path", "");
    if (path.empty())
        path = params.value("file_path", "");
    if (path.empty()) {
        return {
            {"success", false},
            {"message", "Missing model file path"},
            {"details", {{"expected", "path"}, {"accepted_aliases", json::array({"file_path"})}}}
        };
    }

    const auto import_path = boost::filesystem::path(path);
    if (!boost::filesystem::exists(import_path) || !boost::filesystem::is_regular_file(import_path)) {
        return {
            {"success", false},
            {"message", "Model file not found"},
            {"path", path}
        };
    }

    // Take a snapshot before importing so that the operation can be undone.
    // TakeSnapshot also suppresses any nested snapshots triggered during load_files.
    Plater::TakeSnapshot snapshot(plater, std::string("Import Model"));

    wxArrayString paths;
    paths.Add(from_u8(path));
    plater->load_files(paths);
    return {{"success", true}, {"message", "Model imported"}, {"path", path}};
}


void SlicerBridge::ClearPendingModelSearchCache()
{
    m_cached_model_search_result = CachedModelSearchResult{};
}

json SlicerBridge::DoOpenModelLibrary(const json& params)
{
    auto* mainframe = wxGetApp().mainframe;
    if (!mainframe)
        return {{"success", false}, {"message", "MainFrame not available"}};

    std::string query = params.value("query", "");
    wxString trimmed_query = wxString::FromUTF8(query.c_str());
    trimmed_query.Trim(true).Trim(false);

    if (mainframe->topbar())
        mainframe->topbar()->SetSelection(size_t(MainFrame::tpOnlineModel));
    mainframe->select_tab(MainFrame::tpOnlineModel);

    if (!trimmed_query.IsEmpty()) {
        if (auto* model_view = mainframe->get_modellibrary_view())
            model_view->search(trimmed_query);
    }

    return {
        {"success", true},
        {"message", trimmed_query.IsEmpty() ? "Model library opened" : "Model library opened with query"},
        {"query", query}
    };
}

json SlicerBridge::DoRecommendModel(const json& params)
{
    auto parse_int_param = [&params](const char* key, int fallback) {
        int value = fallback;
        try {
            if (params.contains(key)) {
                if (params[key].is_number_integer())
                    value = params[key].get<int>();
                else if (params[key].is_number())
                    value = (int)std::llround(params[key].get<double>());
                else if (params[key].is_string())
                    value = std::stoi(params[key].get<std::string>());
            }
        } catch (...) {
            value = fallback;
        }
        return value;
    };

    int count = parse_int_param("count", parse_int_param("limit", 1));
    int candidate_pool_size = parse_int_param("candidate_pool_size", 30);
    count = std::max(1, std::min(count, 5));
    candidate_pool_size = std::max(5, std::min(candidate_pool_size, 50));

    std::string api_base = Slic3r::GUI::get_cloud_api_url();
    Http::set_extra_headers(wxGetApp().get_extra_header());

    std::vector<json> candidates;
    std::string trend_error;
    const std::string trending_url = api_base + "/api/cxy/v3/model/listTrend";
    json trending_body = {
        {"page", 1},
        {"pageSize", candidate_pool_size},
        {"filterType", 10},
        {"printers", json::array()},
        {"displayVersions", json::array({"cxy-gen2"})},
        {"promoType", 0},
        {"isVip", 0},
        {"trendType", 1},
        {"multiMark", 0},
        {"isPay", 0},
        {"hasCubeMeModel", 1}
    };

    Http::post(trending_url)
        .header("Content-Type", "application/json")
        .timeout_connect(10)
        .timeout_max(30)
        .set_post_body(trending_body.dump())
        .on_complete([&](std::string body, unsigned status) {
            if (status != 200) {
                trend_error = "Trending request failed, HTTP status: " + std::to_string(status);
                return;
            }
            try {
                json response = json::parse(body);
                if (response["code"] == 0 && response.contains("result") && response["result"].contains("list")) {
                    for (const auto& item : response["result"]["list"]) {
                        const std::string model_group_id = item.value("id", "");
                        if (model_group_id.empty())
                            continue;

                        json model;
                        model["model_id"] = model_group_id;
                        model["model_group_id"] = model_group_id;
                        model["model_name"] = item.value("groupName", item.value("name", "Unknown Model"));
                        std::string cover_image;
                        if (item.contains("covers") && item["covers"].is_array() && !item["covers"].empty())
                            cover_image = item["covers"][0].value("url", "");
                        model["cover_image"] = cover_image;
                        model["likes"] = item.value("likeCount", item.value("like", 0));
                        model["downloads"] = item.value("downloadCount", 0);
                        if (item.contains("userInfo") && item["userInfo"].is_object())
                            model["author"] = item["userInfo"].value("nickname", "");
                        else
                            model["author"] = "";
                        candidates.push_back(std::move(model));
                    }
                } else {
                    trend_error = response.value("msg", "Failed to load trending models");
                }
            } catch (const std::exception& e) {
                trend_error = std::string("Failed to parse trending models: ") + e.what();
            }
        })
        .on_error([&](std::string body, std::string error, unsigned status) {
            (void)body;
            trend_error = "Trending request error: " + error + ", status=" + std::to_string(status);
        })
        .perform_sync();

    if (candidates.empty()) {
        return {
            {"success", false},
            {"message", trend_error.empty() ? "No trending models available" : trend_error},
            {"code", "RECOMMEND_MODEL_FAILED"}
        };
    }

    unsigned int seed = 0;
    bool has_seed = false;
    try {
        if (params.contains("random_seed")) {
            if (params["random_seed"].is_number_integer()) {
                seed = (unsigned int)params["random_seed"].get<int>();
                has_seed = true;
            } else if (params["random_seed"].is_number()) {
                seed = (unsigned int)std::llround(params["random_seed"].get<double>());
                has_seed = true;
            } else if (params["random_seed"].is_string()) {
                seed = (unsigned int)std::stoul(params["random_seed"].get<std::string>());
                has_seed = true;
            }
        }
    } catch (...) {
        has_seed = false;
    }

    std::mt19937 rng(has_seed ? seed : std::random_device{}());
    std::shuffle(candidates.begin(), candidates.end(), rng);

    const std::string threemf_list_url = api_base + "/api/cxy/v3/model/3mfList";
    json recommended_models = json::array();

    for (const auto& candidate : candidates) {
        const std::string model_group_id = candidate.value("model_group_id", "");
        if (model_group_id.empty())
            continue;

        std::string threemf_id;
        std::string resolved_model_name = candidate.value("model_name", "");
        bool list_success = false;
        std::string list_error;
        json threemf_body = {{"modelGroupId", model_group_id}, {"page", 1}, {"pageSize", 1}};
        Http::post(threemf_list_url)
            .header("Content-Type", "application/json")
            .timeout_connect(10)
            .timeout_max(30)
            .set_post_body(threemf_body.dump())
            .on_complete([&](std::string body, unsigned status) {
                if (status != 200) {
                    list_error = "Get 3mf list failed, HTTP status: " + std::to_string(status);
                    return;
                }
                try {
                    json response = json::parse(body);
                    if (response["code"] == 0 && response.contains("result") && response["result"].contains("list")) {
                        auto& list = response["result"]["list"];
                        if (!list.empty()) {
                            threemf_id = list[0].value("id", "");
                            resolved_model_name = list[0].value("name", resolved_model_name);
                            list_success = !threemf_id.empty();
                            if (!list_success)
                                list_error = "Matched model does not contain a valid 3mf id";
                        } else {
                            list_error = "No downloadable 3mf found under matched model group";
                        }
                    } else {
                        list_error = response.value("msg", "Get 3mf list failed");
                    }
                } catch (const std::exception& e) {
                    list_error = std::string("Failed to parse 3mf list: ") + e.what();
                }
            })
            .on_error([&](std::string body, std::string error, unsigned status) {
                (void)body;
                list_error = "Get 3mf list error: " + error + ", status=" + std::to_string(status);
            })
            .perform_sync();

        if (!list_success)
            continue;

        json model = candidate;
        model["model_name"] = resolved_model_name;
        model["threemf_id"] = threemf_id;
        model["import_action"] = ActionID::IMPORT_MODEL_FROM_SEARCH;
        recommended_models.push_back(std::move(model));
        if ((int)recommended_models.size() >= count)
            break;
    }

    if (recommended_models.empty()) {
        return {
            {"success", false},
            {"message", "Failed to find a recommendable online model with downloadable 3mf."},
            {"code", "NO_RECOMMENDABLE_3MF_MODEL"}
        };
    }

    return {
        {"success", true},
        {"message", "Model recommendation completed"},
        {"card_type", "model-recommendations"},
        {"title", "Recommended Models"},
        {"description", "Randomly selected trending online models with downloadable 3mf files."},
        {"recommendation_source", "trending_random"},
        {"random", true},
        {"models", recommended_models},
        {"recommended_model", recommended_models.front()},
        {"total_models", (int)recommended_models.size()}
    };
}

json SlicerBridge::DoSmartModelSearch(const json& params)
{
    auto trim = [](std::string value) {
        const auto is_ws = [](unsigned char ch) { return std::isspace(ch) != 0; };
        while (!value.empty() && is_ws(static_cast<unsigned char>(value.front()))) value.erase(value.begin());
        while (!value.empty() && is_ws(static_cast<unsigned char>(value.back()))) value.pop_back();
        return value;
    };

    std::string keyword = trim(params.value("keyword", ""));
    if (keyword.empty())
        keyword = trim(params.value("query", ""));
    if (keyword.empty()) {
        json fallback_params = params;
        if (!fallback_params.contains("count") && fallback_params.contains("limit"))
            fallback_params["count"] = fallback_params["limit"];
        json result = DoRecommendModel(fallback_params);
        if (result.value("success", false)) {
            result["message"] = "No keyword provided. Fell back to recommend_model.";
            result["fallback_action"] = ActionID::RECOMMEND_MODEL;
        } else {
            result["message"] = result.value("message", std::string("Failed to recommend online model"));
        }
        return result;
    }

    std::string sort_by = trim(params.value("sort_by", "default"));
    std::string sort_label = trim(params.value("sort_label", ""));
    int limit = 16;
    try {
        if (params.contains("limit")) {
            if (params["limit"].is_number_integer())
                limit = params["limit"].get<int>();
            else if (params["limit"].is_number())
                limit = (int)std::llround(params["limit"].get<double>());
            else if (params["limit"].is_string())
                limit = std::stoi(params["limit"].get<std::string>());
        }
    } catch (...) {
        limit = 6;
    }
    limit = std::max(1, std::min(limit, 30));

    if (sort_label.empty()) {
        sort_label = "default";
        if (sort_by == "likes") sort_label = "likes";
        else if (sort_by == "favorites") sort_label = "favorites";
        else if (sort_by == "date") sort_label = "date";
        else if (sort_by == "downloads") sort_label = "downloads";
        else if (sort_by == "views") sort_label = "views";
        else if (sort_by == "score") sort_label = "score";
    }

    int sort_type = 10;  // 默认综合排序
    if (sort_by == "likes") sort_type = 1;
    else if (sort_by == "favorites") sort_type = 2;
    else if (sort_by == "date") sort_type = 3;
    else if (sort_by == "downloads") sort_type = 7;
    else if (sort_by == "views") sort_type = 9;
    else if (sort_by == "score") sort_type = 10;
    else if (sort_by == "default") sort_type = 10;  // default也是综合排序

    std::string api_base = Slic3r::GUI::get_cloud_api_url();
    Http::set_extra_headers(wxGetApp().get_extra_header());

    bool search_success = false;
    std::string search_error;
    json models = json::array();
    int total_models = 0;

    const std::string search_url = api_base + "/api/cxy/search/model";
    json search_body = {
        {"page", 1},
        {"pageSize", limit},
        {"keyword", keyword},
        {"sortType", sort_type},
        {"hasCubeMeModel", 1},
        {"displayVersions", json::array({"cxy-gen2"})},
        {"promoType", 0},
        {"isVip", 0},
        {"trendType", 1},
        {"multiMark", 0},
        {"hasCfgFile", 1},
        {"isPay", 0}
    };

    Http::post(search_url)
        .header("Content-Type", "application/json")
        .timeout_connect(10)
        .timeout_max(30)
        .set_post_body(search_body.dump())
        .on_complete([&](std::string body, unsigned status) {
            if (status != 200) {
                search_error = "Search request failed, HTTP status: " + std::to_string(status);
                return;
            }
            try {
                json response = json::parse(body);
                if (response["code"] == 0 && response.contains("result") && response["result"].contains("list")) {
                    auto& list = response["result"]["list"];
                    total_models = response["result"].value("total", (int)list.size());
                    for (const auto& item : list) {
                        json model;
                        model["model_id"] = item.value("id", "");
                        model["model_group_id"] = item.value("id", "");
                        model["model_name"] = item.value("groupName", item.value("name", keyword));
                        std::string cover_image;
                        if (item.contains("covers") && item["covers"].is_array() && !item["covers"].empty()) {
                            cover_image = item["covers"][0].value("url", "");
                        }
                        model["cover_image"] = cover_image;
                        model["likes"] = item.value("likeCount", item.value("like", 0));
                        model["downloads"] = item.value("downloadCount", 0);
                        if (item.contains("userInfo") && item["userInfo"].is_object())
                            model["author"] = item["userInfo"].value("nickname", "");
                        else
                            model["author"] = "";
                        models.push_back(std::move(model));
                    }
                    search_success = !models.empty();
                    if (!search_success)
                        search_error = "No matched model found";
                } else {
                    search_error = response.value("msg", "Search failed");
                }
            } catch (const std::exception& e) {
                search_error = std::string("Failed to parse search response: ") + e.what();
            }
        })
        .on_error([&](std::string body, std::string error, unsigned status) {
            (void)body;
            search_error = "Search request error: " + error + ", status=" + std::to_string(status);
        })
        .perform_sync();

    if (!search_success) {
        ClearPendingModelSearchCache();
        return {
            {"success", false},
            {"message", search_error.empty() ? "Search failed" : search_error},
            {"error", search_error.empty() ? "Search failed" : search_error},
            {"keyword", keyword}
        };
    }

    m_cached_model_search_result = CachedModelSearchResult{};
    m_cached_model_search_result.keyword = keyword;
    m_cached_model_search_result.sort_by = sort_by;
    m_cached_model_search_result.sort_label = sort_label;

    return {
        {"success", true},
        {"message", "Model search completed"},
        {"card_type", "model-search-results"},
        {"title", "Model Search Results"},
        {"description", "Found matching models. Click a card to preview the model."},
        {"keyword", keyword},
        {"sort_by", sort_by},
        {"sort_label", sort_label},
        {"models", models},
        {"total_models", total_models > 0 ? total_models : (int)models.size()}
    };
}

json SlicerBridge::DoImportModelFromSearch(const json& params)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    std::string model_group_id = params.value("model_group_id", "");
    if (model_group_id.empty())
        model_group_id = params.value("model_id", "");
    if (model_group_id.empty()) {
        return {{"success", false}, {"message", "Missing model id for import_model_from_search"}, {"code", "MISSING_MODEL_ID"}};
    }

    std::string api_base = Slic3r::GUI::get_cloud_api_url();
    Http::set_extra_headers(wxGetApp().get_extra_header());

    std::string threemf_id;
    std::string model_name = params.value("model_name", "");
    bool list_success = false;
    std::string list_error;
    const std::string threemf_list_url = api_base + "/api/cxy/v3/model/3mfList";
    json threemf_body = {{"modelGroupId", model_group_id}, {"page", 1}, {"pageSize", 1}};
    Http::post(threemf_list_url)
        .header("Content-Type", "application/json")
        .timeout_connect(10)
        .timeout_max(30)
        .set_post_body(threemf_body.dump())
        .on_complete([&](std::string body, unsigned status) {
            if (status != 200) {
                list_error = "Get 3mf list failed, HTTP status: " + std::to_string(status);
                return;
            }
            try {
                json response = json::parse(body);
                if (response["code"] == 0 && response.contains("result") && response["result"].contains("list")) {
                    auto& list = response["result"]["list"];
                    if (!list.empty()) {
                        threemf_id = list[0].value("id", "");
                        model_name = list[0].value("name", model_name);
                        list_success = true;
                    } else {
                        list_error = "No downloadable file under matched model group";
                    }
                } else {
                    list_error = response.value("msg", "Get 3mf list failed");
                }
            } catch (const std::exception& e) {
                list_error = std::string("Failed to parse 3mf list: ") + e.what();
            }
        })
        .on_error([&](std::string body, std::string error, unsigned status) {
            (void)body;
            list_error = "Get 3mf list error: " + error + ", status=" + std::to_string(status);
        })
        .perform_sync();

    if (!list_success) {
        return {{"success", false}, {"message", list_error.empty() ? "Get model file list failed" : list_error}, {"code", "GET_3MF_LIST_FAILED"}};
    }

    std::string download_url;
    bool download_api_success = false;
    std::string download_api_error;
    const std::string download_api_url = api_base + "/api/cxy/v3/model/3mfDownload";
    json download_body = {{"id", threemf_id}};
    Http::post(download_api_url)
        .header("Content-Type", "application/json")
        .timeout_connect(10)
        .timeout_max(30)
        .set_post_body(download_body.dump())
        .on_complete([&](std::string body, unsigned status) {
            if (status != 200) {
                download_api_error = "Get download url failed, HTTP status: " + std::to_string(status);
                return;
            }
            try {
                json response = json::parse(body);
                if (response["code"] == 0 && response.contains("result") && response["result"].contains("downloadUrl")) {
                    download_url = response["result"]["downloadUrl"].get<std::string>();
                    download_api_success = true;
                } else {
                    download_api_error = response.value("msg", "Get download url failed");
                }
            } catch (const std::exception& e) {
                download_api_error = std::string("Failed to parse download url: ") + e.what();
            }
        })
        .on_error([&](std::string body, std::string error, unsigned status) {
            (void)body;
            download_api_error = "Get download url error: " + error + ", status=" + std::to_string(status);
        })
        .perform_sync();

    if (!download_api_success) {
        return {{"success", false}, {"message", download_api_error.empty() ? "Get download url failed" : download_api_error}, {"code", "GET_DOWNLOAD_URL_FAILED"}};
    }

    if (wxGetApp().mainframe)
        wxGetApp().mainframe->select_tab((size_t) MainFrame::TabPosition::tp3DEditor);

    wxString import_info = wxString::Format("%s name=%s.3mf",
        wxString::FromUTF8(download_url.c_str()),
        wxString::FromUTF8(model_name.c_str()));
    plater->import_model_id(import_info);

    return {
        {"success", true},
        {"message", "Model import started. The model will be available shortly."},
        {"model_id", threemf_id},
        {"model_group_id", model_group_id},
        {"model_name", model_name},
        {"source_action", "import_model_from_search"},
        {"model_import_status", "importing_async"}
    };
}

json SlicerBridge::DoMoveObject(const json& params)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    std::string target_name = params.value("object_name", "");
    int obj_idx = params.value("object_index", -1);
    double dx = params.value("dx", 0.0);
    double dy = params.value("dy", 0.0);
    double dz = params.value("dz", 0.0);

    auto nearly_zero = [](double v) { return std::fabs(v) < 1e-9; };

    auto trim = [](std::string s) {
        auto is_ws = [](unsigned char ch) { return std::isspace(ch) != 0; };
        while (!s.empty() && is_ws((unsigned char)s.front())) s.erase(s.begin());
        while (!s.empty() && is_ws((unsigned char)s.back())) s.pop_back();
        return s;
    };

    auto lower_ascii = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return (char)std::tolower(c);
        });
        return s;
    };

    auto parse_bool = [&lower_ascii, &trim](const json& value, bool default_value = false) -> bool {
        try {
            if (value.is_boolean())
                return value.get<bool>();
            if (value.is_number_integer())
                return value.get<int>() != 0;
            if (value.is_string()) {
                const std::string s = lower_ascii(trim(value.get<std::string>()));
                if (s == "true" || s == "1" || s == "yes" || s == "on")
                    return true;
                if (s == "false" || s == "0" || s == "no" || s == "off")
                    return false;
            }
        } catch (...) {
        }
        return default_value;
    };

    auto parse_int_relaxed = [&trim](const json& value, int& out) -> bool {
        try {
            if (value.is_number_integer()) {
                out = value.get<int>();
                return true;
            }
            if (value.is_number()) {
                out = (int)std::llround(value.get<double>());
                return true;
            }
            if (value.is_string()) {
                const std::string s = trim(value.get<std::string>());
                if (s.empty())
                    return false;

                try {
                    size_t pos = 0;
                    int v = std::stoi(s, &pos);
                    if (pos == s.size()) {
                        out = v;
                        return true;
                    }
                } catch (...) {
                }

                std::string token;
                bool started = false;
                for (unsigned char ch : s) {
                    if (!started) {
                        if (std::isdigit(ch) || ch == '+' || ch == '-') {
                            token.push_back((char)ch);
                            started = true;
                        }
                    } else {
                        if (std::isdigit(ch))
                            token.push_back((char)ch);
                        else
                            break;
                    }
                }

                if (!token.empty() && token != "+" && token != "-") {
                    out = std::stoi(token);
                    return true;
                }
            }
        } catch (...) {
        }
        return false;
    };

    const bool auto_fix_bounds = params.contains("auto_fix_bounds") && parse_bool(params["auto_fix_bounds"]);
    const bool auto_fix_clearance = params.contains("auto_fix_clearance") && parse_bool(params["auto_fix_clearance"]);
    const bool has_auto_mode = auto_fix_bounds || auto_fix_clearance;
    const bool has_relative_offset = !(nearly_zero(dx) && nearly_zero(dy) && nearly_zero(dz));
    const double bounds_margin_mm = std::max(0.0, params.value("bounds_margin_mm", 1.0));
    const double min_clearance_mm = std::max(0.0, params.value("min_clearance_mm", 8.0));
    const double clearance_margin_mm = std::max(0.0, params.value("clearance_margin_mm", 2.0));
    const double target_clearance_mm = min_clearance_mm + clearance_margin_mm;
    std::string movable_value = params.value("movable", "");
    if (trim(movable_value).empty())
        movable_value = params.value("target_kind", "");
    const std::string movable = lower_ascii(trim(movable_value));
    const bool move_wipe_tower =
        movable == "wipe_tower" || movable == "prime_tower" ||
        lower_ascii(trim(target_name)) == "wipe_tower" ||
        lower_ascii(trim(target_name)) == "prime_tower";

    Model& model = plater->model();
    std::vector<int> target_obj_indices;
    std::unordered_set<int> seen;
    auto add_target = [&](int idx) {
        if (idx >= 0 && idx < (int)model.objects.size() && seen.insert(idx).second)
            target_obj_indices.push_back(idx);
    };

    // Check if plate scope is explicitly requested
    const bool has_plate_param =
        params.contains("plate_number") || params.contains("plateNumber") ||
        params.contains("plate_no") || params.contains("plateNo") ||
        params.contains("target_plate_number") || params.contains("targetPlateNumber") ||
        params.contains("target_plate") || params.contains("targetPlate") ||
        params.contains("plate_index") || params.contains("plateIndex") ||
        params.contains("plate");

    if (params.contains("object_indices") && params["object_indices"].is_array()) {
        for (const auto& it : params["object_indices"]) {
            int idx = -1;
            if (parse_int_relaxed(it, idx))
                add_target(idx);
        }
    }

    // When plate_number is specified, ignore object_index from params
    if (obj_idx >= 0 && !has_plate_param)
        add_target(obj_idx);

    if (target_obj_indices.empty() && !target_name.empty()) {
        for (int i = 0; i < (int)model.objects.size(); ++i) {
            if (model.objects[i] && model.objects[i]->name == target_name) {
                add_target(i);
                break;
            }
        }
    }

    // ---- Try current selection ----
    if (target_obj_indices.empty()) {
        if (auto* canvas = plater->canvas3D()) {
            const Selection& selection = canvas->get_selection();
            const auto& selected_content = selection.get_content();
            for (const auto& kv : selected_content)
                add_target(kv.first);
        }
    }

    if (target_obj_indices.empty() && model.objects.size() == 1)
        add_target(0);

    PartPlateList& plate_list = plater->get_partplate_list();
    const int plate_count = plate_list.get_plate_count();

    auto parse_key = [&](const char* key, int& out) -> bool {
        return params.contains(key) && parse_int_relaxed(params[key], out);
    };

    auto find_plate_by_public_index = [&](int requested) -> int {
        for (int pi = 0; pi < plate_count; ++pi) {
            PartPlate* plate = plate_list.get_plate(pi);
            if (plate && plate->get_index() == requested)
                return pi;
        }
        return -1;
    };

    auto resolve_plate_index_like = [&](int requested) -> int {
        if (requested >= 0 && requested < plate_count)
            return requested;
        return find_plate_by_public_index(requested);
    };

    auto resolve_plate_number = [&](int plate_number) -> int {
        if (plate_number > 0 && plate_number <= plate_count)
            return plate_number - 1;
        return -1;
    };

    bool has_target_plate = false;
    int requested_plate = -1;
    int target_plate_idx = -1;

    if (parse_key("plate_number", requested_plate) || parse_key("plateNumber", requested_plate) ||
        parse_key("plate_no", requested_plate) || parse_key("plateNo", requested_plate) ||
        parse_key("target_plate_number", requested_plate) || parse_key("targetPlateNumber", requested_plate)) {
        target_plate_idx = resolve_plate_number(requested_plate);
        has_target_plate = true;
    } else if (parse_key("plate", requested_plate) || parse_key("target_plate", requested_plate) || parse_key("targetPlate", requested_plate)) {
        target_plate_idx = resolve_plate_number(requested_plate);
        if (target_plate_idx < 0)
            target_plate_idx = resolve_plate_index_like(requested_plate);
        has_target_plate = true;
    } else if (parse_key("plate_index", requested_plate) || parse_key("plateIndex", requested_plate)) {
        target_plate_idx = resolve_plate_index_like(requested_plate);
        has_target_plate = true;
    }

    if (target_obj_indices.empty() && !move_wipe_tower) {
        // Fallback: when no explicit target and no selection, operate on all models
        // on the current (or specified) plate.
        if (!has_target_plate) {
            target_plate_idx = plate_list.get_curr_plate_index();
        }
        if (target_plate_idx >= 0 && target_plate_idx < plate_count) {
            PartPlate* plate = plate_list.get_plate(target_plate_idx);
            if (plate) {
                ModelObjectPtrs objs = plate->get_objects_on_this_plate();
                for (const ModelObject* obj : objs) {
                    for (int oi = 0; oi < (int)model.objects.size(); ++oi) {
                        if (model.objects[oi] == obj) {
                            add_target(oi);
                            break;
                        }
                    }
                }
                // Fallback: try intersect check if still empty
                if (target_obj_indices.empty()) {
                    for (int oi = 0; oi < (int)model.objects.size(); ++oi) {
                        const ModelObject* mobj = model.objects[oi];
                        if (!mobj || mobj->instances.empty()) continue;
                        for (int ii = 0; ii < (int)mobj->instances.size(); ++ii) {
                            if (plate->intersect_instance(oi, ii)) {
                                add_target(oi);
                                break;
                            }
                        }
                    }
                }
            }
        }
        // Last resort: if we still have no targets (e.g. all models are off-plate),
        // just use all models in the scene. The center-to-plate logic will move
        // them to the target plate regardless of where they currently are.
        if (target_obj_indices.empty()) {
            for (int oi = 0; oi < (int)model.objects.size(); ++oi) {
                if (model.objects[oi] && !model.objects[oi]->instances.empty())
                    add_target(oi);
            }
        }
    }
    if (target_obj_indices.empty() && !move_wipe_tower)
        return {{"success", false}, {"message", "Target object not found"}};

    // 没有相对偏移、没有指定盘、没有自动模式 → 理解为"居中到当前盘"
    if (!has_relative_offset && !has_target_plate && !has_auto_mode) {
        target_plate_idx = plate_list.get_curr_plate_index();
        has_target_plate = true;
    }

    if (dz > 0.0)
        return {{"success", false}, {"message", "Positive dz is not supported because the model must stay on the bed"}};

    if (has_target_plate) {
        if (plate_count <= 0)
            return {{"success", false}, {"message", "No plate available"}};
        // 如果 plate_number 解析失败（比如 LLM 猜错了），fallback 到当前盘
        if (target_plate_idx < 0 || target_plate_idx >= plate_count)
            target_plate_idx = plate_list.get_curr_plate_index();
    }

    auto resolve_plate_for_object = [&](int object_index) -> int {
        if (has_target_plate)
            return target_plate_idx;
        int plate_idx = plate_list.find_instance(object_index, 0);
        if (plate_idx < 0)
            plate_idx = plate_list.get_curr_plate_index();
        return plate_idx;
    };

    auto resolve_bounds_delta = [&](const BoundingBoxf3& moving_bbox, const BoundingBoxf3& build_volume, double margin, Vec3d& out_delta, std::string& error) -> bool {
        const BoundingBoxf3 active_bbox(moving_bbox.min + out_delta, moving_bbox.max + out_delta);
        const double min_x = build_volume.min.x() + margin;
        const double max_x = build_volume.max.x() - margin;
        const double min_y = build_volume.min.y() + margin;
        const double max_y = build_volume.max.y() - margin;
        const double width = active_bbox.max.x() - active_bbox.min.x();
        const double depth = active_bbox.max.y() - active_bbox.min.y();
        if (width > max_x - min_x || depth > max_y - min_y) {
            error = "Object is larger than the current plate build volume; move_object cannot fix bounds";
            return false;
        }
        if (active_bbox.max.z() > build_volume.max.z() + 1e-6) {
            error = "Object exceeds build height; move_object cannot fix height limits";
            return false;
        }

        double auto_dx = 0.0;
        double auto_dy = 0.0;
        if (active_bbox.min.x() < min_x)
            auto_dx = min_x - active_bbox.min.x();
        if (active_bbox.max.x() + auto_dx > max_x)
            auto_dx += max_x - (active_bbox.max.x() + auto_dx);
        if (active_bbox.min.y() < min_y)
            auto_dy = min_y - active_bbox.min.y();
        if (active_bbox.max.y() + auto_dy > max_y)
            auto_dy += max_y - (active_bbox.max.y() + auto_dy);

        out_delta += Vec3d(auto_dx, auto_dy, 0.0);
        return true;
    };

    auto axis_gap_and_direction = [](double active_min, double active_max, double other_min, double other_max) -> std::pair<double, double> {
        const double active_center = (active_min + active_max) / 2.0;
        const double other_center = (other_min + other_max) / 2.0;
        const double center_direction = active_center >= other_center ? 1.0 : -1.0;
        if (active_max < other_min)
            return {other_min - active_max, -1.0};
        if (other_max < active_min)
            return {active_min - other_max, 1.0};
        const double overlap = std::min(active_max, other_max) - std::max(active_min, other_min);
        return {-overlap, center_direction};
    };

    auto clearance_value = [&](const BoundingBoxf3& moving_bbox, const BoundingBoxf3& other_bbox) -> double {
        const auto gap_x_info = axis_gap_and_direction(moving_bbox.min.x(), moving_bbox.max.x(), other_bbox.min.x(), other_bbox.max.x());
        const auto gap_y_info = axis_gap_and_direction(moving_bbox.min.y(), moving_bbox.max.y(), other_bbox.min.y(), other_bbox.max.y());
        const double gap_x = gap_x_info.first;
        const double gap_y = gap_y_info.first;
        if (gap_x > 0.0 && gap_y > 0.0)
            return std::hypot(gap_x, gap_y);
        if (gap_x > 0.0)
            return gap_x;
        if (gap_y > 0.0)
            return gap_y;
        return std::max(gap_x, gap_y);
    };

    auto resolve_clearance_delta = [&](const BoundingBoxf3& moving_bbox, int moving_object_index, int moving_plate_idx, const BoundingBoxf3& build_volume, double margin, Vec3d& out_delta, std::string& error) -> bool {
        const BoundingBoxf3 active_bbox(moving_bbox.min + out_delta, moving_bbox.max + out_delta);
        const double min_x = build_volume.min.x() + margin;
        const double max_x = build_volume.max.x() - margin;
        const double min_y = build_volume.min.y() + margin;
        const double max_y = build_volume.max.y() - margin;
        const double width = active_bbox.max.x() - active_bbox.min.x();
        const double depth = active_bbox.max.y() - active_bbox.min.y();
        if (width > max_x - min_x || depth > max_y - min_y) {
            error = "Object is larger than the current plate build volume; move_object cannot fix clearance";
            return false;
        }
        if (active_bbox.max.z() > build_volume.max.z() + 1e-6) {
            error = "Object exceeds build height; move_object cannot fix clearance";
            return false;
        }

        bool found = false;
        double nearest_clearance = std::numeric_limits<double>::max();
        std::vector<BoundingBoxf3> obstacle_bboxes;
        for (int oi = 0; oi < (int)model.objects.size(); ++oi) {
            if (oi == moving_object_index)
                continue;
            ModelObject* other = model.objects[oi];
            if (!other || other->instances.empty())
                continue;
            if (moving_plate_idx >= 0 && plate_list.find_instance(oi, 0) != moving_plate_idx)
                continue;
            const BoundingBoxf3 other_bbox = other->instance_convex_hull_bounding_box(size_t(0));
            const double c = clearance_value(active_bbox, other_bbox);
            obstacle_bboxes.push_back(other_bbox);
            if (!found || c < nearest_clearance) {
                found = true;
                nearest_clearance = c;
            }
        }
        if (!found) {
            error = "No reference object found for clearance repair";
            return false;
        }
        if (nearest_clearance >= target_clearance_mm)
            return true;

        std::vector<double> candidate_dx = {0.0};
        std::vector<double> candidate_dy = {0.0};
        candidate_dx.push_back(min_x - active_bbox.min.x());
        candidate_dx.push_back(max_x - active_bbox.max.x());
        candidate_dy.push_back(min_y - active_bbox.min.y());
        candidate_dy.push_back(max_y - active_bbox.max.y());
        for (const BoundingBoxf3& other_bbox : obstacle_bboxes) {
            candidate_dx.push_back(other_bbox.max.x() + target_clearance_mm - active_bbox.min.x());
            candidate_dx.push_back(other_bbox.min.x() - target_clearance_mm - active_bbox.max.x());
            candidate_dy.push_back(other_bbox.max.y() + target_clearance_mm - active_bbox.min.y());
            candidate_dy.push_back(other_bbox.min.y() - target_clearance_mm - active_bbox.max.y());
        }

        auto normalize_candidates = [](std::vector<double>& values) {
            std::sort(values.begin(), values.end());
            values.erase(std::unique(values.begin(), values.end(), [](double a, double b) {
                return std::fabs(a - b) < 1e-6;
            }), values.end());
        };
        normalize_candidates(candidate_dx);
        normalize_candidates(candidate_dy);

        auto inside_build_volume = [&](const BoundingBoxf3& bbox) {
            return bbox.min.x() >= min_x - 1e-6 && bbox.max.x() <= max_x + 1e-6 &&
                   bbox.min.y() >= min_y - 1e-6 && bbox.max.y() <= max_y + 1e-6;
        };
        auto satisfies_all_clearance = [&](const BoundingBoxf3& bbox) {
            for (const BoundingBoxf3& other_bbox : obstacle_bboxes) {
                if (clearance_value(bbox, other_bbox) < target_clearance_mm - 1e-6)
                    return false;
            }
            return true;
        };

        bool has_candidate = false;
        Vec3d best_delta = Vec3d::Zero();
        double best_score = std::numeric_limits<double>::max();
        for (double dx_candidate : candidate_dx) {
            for (double dy_candidate : candidate_dy) {
                BoundingBoxf3 candidate_bbox(active_bbox.min + Vec3d(dx_candidate, dy_candidate, 0.0), active_bbox.max + Vec3d(dx_candidate, dy_candidate, 0.0));
                if (!inside_build_volume(candidate_bbox) || !satisfies_all_clearance(candidate_bbox))
                    continue;
                const double score = std::hypot(dx_candidate, dy_candidate);
                if (!has_candidate || score < best_score) {
                    has_candidate = true;
                    best_score = score;
                    best_delta = Vec3d(dx_candidate, dy_candidate, 0.0);
                }
            }
        }
        if (!has_candidate) {
            error = "No collision-free clearance move found within the current plate";
            return false;
        }
        out_delta += best_delta;
        return true;
    };
    auto apply_wipe_tower_delta = [&](int plate_idx, const Vec3d& delta, json& response) -> bool {
        if (plate_idx < 0 || plate_idx >= plate_count)
            return false;
        if (!wxGetApp().preset_bundle)
            return false;
        DynamicConfig& proj_cfg = wxGetApp().preset_bundle->project_config;
        ConfigOptionFloats* wipe_x_option = proj_cfg.option<ConfigOptionFloats>("wipe_tower_x", true);
        ConfigOptionFloats* wipe_y_option = proj_cfg.option<ConfigOptionFloats>("wipe_tower_y", true);
        ConfigOptionFloat default_x(40.f);
        ConfigOptionFloat default_y(200.f);
        if ((int)wipe_x_option->values.size() <= plate_idx)
            wipe_x_option->resize((size_t)plate_idx + 1, &default_x);
        if ((int)wipe_y_option->values.size() <= plate_idx)
            wipe_y_option->resize((size_t)plate_idx + 1, &default_y);

        const double old_x = wipe_x_option->get_at(plate_idx);
        const double old_y = wipe_y_option->get_at(plate_idx);
        ConfigOptionFloat new_x(old_x + delta.x());
        ConfigOptionFloat new_y(old_y + delta.y());
        wipe_x_option->set_at(&new_x, plate_idx, 0);
        wipe_y_option->set_at(&new_y, plate_idx, 0);
        if ((int)model.wipe_tower.positions.size() <= plate_idx)
            model.wipe_tower.positions.resize((size_t)plate_idx + 1, Vec2d::Zero());
        model.wipe_tower.positions[plate_idx] = Vec2d(old_x + delta.x(), old_y + delta.y());
        response["wipe_tower_position"] = {old_x + delta.x(), old_y + delta.y()};
        return true;
    };

    if (move_wipe_tower) {
        const int plate_idx = has_target_plate ? target_plate_idx : plate_list.get_curr_plate_index();
        if (plate_idx < 0 || plate_idx >= plate_count)
            return {{"success", false}, {"message", "No valid plate for wipe tower movement"}};
        auto* canvas = plater->canvas3D();
        if (!canvas)
            return {{"success", false}, {"message", "Canvas not available"}};
        GLCanvas3D::WipeTowerInfo wti = canvas->get_wipe_tower_info(plate_idx);
        if (!wti)
            return {{"success", false}, {"message", "Wipe tower is not available on the target plate"}};

        Vec3d auto_delta(dx, dy, dz);
        std::string auto_error;
        PartPlate* plate = plate_list.get_plate(plate_idx);
        if (!plate)
            return {{"success", false}, {"message", "Target plate not found"}};
        const Vec2d wipe_origin = to_2d(plate->get_origin()) + wti.pos();
        BoundingBoxf3 wipe_bbox(to_3d(wipe_origin, 0.0), to_3d(wipe_origin + wti.bb_size(), 1.0));
        if (auto_fix_bounds && !resolve_bounds_delta(wipe_bbox, plate->get_build_volume(), bounds_margin_mm, auto_delta, auto_error))
            return {{"success", false}, {"message", auto_error}};
        if (auto_fix_clearance) {

            std::vector<BoundingBoxf3> obstacle_bboxes;
            double nearest_clearance = std::numeric_limits<double>::max();
            bool found = false;
            const BoundingBoxf3 active_wipe_bbox(wipe_bbox.min + auto_delta, wipe_bbox.max + auto_delta);
            for (int oi = 0; oi < (int)model.objects.size(); ++oi) {

                ModelObject* other = model.objects[oi];
                if (!other || other->instances.empty())
                    continue;
                if (plate_list.find_instance(oi, 0) != plate_idx)
                    continue;
                const BoundingBoxf3 other_bbox = other->instance_convex_hull_bounding_box(size_t(0));
                obstacle_bboxes.push_back(other_bbox);
                const double current_clearance = clearance_value(active_wipe_bbox, other_bbox);
                if (!found || current_clearance < nearest_clearance) {
                    found = true;
                    nearest_clearance = current_clearance;
                }
            }
            if (!found) {
                return {{"success", false}, {"message", "No nearby object found for wipe tower clearance repair"}};
            }
            if (nearest_clearance < target_clearance_mm) {
                const double min_x = plate->get_build_volume().min.x() + bounds_margin_mm;
                const double max_x = plate->get_build_volume().max.x() - bounds_margin_mm;
                const double min_y = plate->get_build_volume().min.y() + bounds_margin_mm;
                const double max_y = plate->get_build_volume().max.y() - bounds_margin_mm;
                std::vector<double> candidate_dx = {0.0};
                std::vector<double> candidate_dy = {0.0};
                candidate_dx.push_back(min_x - active_wipe_bbox.min.x());
                candidate_dx.push_back(max_x - active_wipe_bbox.max.x());
                candidate_dy.push_back(min_y - active_wipe_bbox.min.y());
                candidate_dy.push_back(max_y - active_wipe_bbox.max.y());
                for (const BoundingBoxf3& other_bbox : obstacle_bboxes) {
                    candidate_dx.push_back(other_bbox.max.x() + target_clearance_mm - active_wipe_bbox.min.x());
                    candidate_dx.push_back(other_bbox.min.x() - target_clearance_mm - active_wipe_bbox.max.x());
                    candidate_dy.push_back(other_bbox.max.y() + target_clearance_mm - active_wipe_bbox.min.y());
                    candidate_dy.push_back(other_bbox.min.y() - target_clearance_mm - active_wipe_bbox.max.y());
                }

                auto normalize_candidates = [](std::vector<double>& values) {
                    std::sort(values.begin(), values.end());
                    values.erase(std::unique(values.begin(), values.end(), [](double a, double b) {
                        return std::fabs(a - b) < 1e-6;
                    }), values.end());
                };
                normalize_candidates(candidate_dx);
                normalize_candidates(candidate_dy);

                auto inside_build_volume = [&](const BoundingBoxf3& bbox) {
                    return bbox.min.x() >= min_x - 1e-6 && bbox.max.x() <= max_x + 1e-6 &&
                           bbox.min.y() >= min_y - 1e-6 && bbox.max.y() <= max_y + 1e-6;
                };
                auto satisfies_all_clearance = [&](const BoundingBoxf3& bbox) {
                    for (const BoundingBoxf3& other_bbox : obstacle_bboxes) {
                        if (clearance_value(bbox, other_bbox) < target_clearance_mm - 1e-6)
                            return false;
                    }
                    return true;
                };

                bool has_candidate = false;
                Vec3d best_delta = Vec3d::Zero();
                double best_score = std::numeric_limits<double>::max();
                for (double dx_candidate : candidate_dx) {
                    for (double dy_candidate : candidate_dy) {
                        BoundingBoxf3 candidate_bbox(active_wipe_bbox.min + Vec3d(dx_candidate, dy_candidate, 0.0), active_wipe_bbox.max + Vec3d(dx_candidate, dy_candidate, 0.0));
                        if (!inside_build_volume(candidate_bbox) || !satisfies_all_clearance(candidate_bbox))
                            continue;
                        const double score = std::hypot(dx_candidate, dy_candidate);
                        if (!has_candidate || score < best_score) {
                            has_candidate = true;
                            best_score = score;
                            best_delta = Vec3d(dx_candidate, dy_candidate, 0.0);
                        }
                    }
                }
                if (!has_candidate)
                    return {{"success", false}, {"message", "No collision-free clearance move found for wipe tower within the current plate"}};
                auto_delta += best_delta;
            }
            BoundingBoxf3 moved_bbox(wipe_bbox.min + auto_delta, wipe_bbox.max + auto_delta);
            Vec3d bounds_delta = Vec3d::Zero();
            if (!resolve_bounds_delta(moved_bbox, plate->get_build_volume(), bounds_margin_mm, bounds_delta, auto_error))
                return {{"success", false}, {"message", auto_error}};
            auto_delta += bounds_delta;
        }

        if (nearly_zero(auto_delta.x()) && nearly_zero(auto_delta.y()) && nearly_zero(auto_delta.z()))
            return {{"success", true}, {"message", "Wipe tower already satisfies the requested move constraints"}, {"applied_delta", {0.0, 0.0, 0.0}}};

        plater->take_snapshot("Move Wipe Tower");
        json resp = {{"success", true}, {"applied_delta", {auto_delta.x(), auto_delta.y(), auto_delta.z()}}, {"target_kind", "wipe_tower"}, {"target_plate_index", plate_idx}};
        if (!apply_wipe_tower_delta(plate_idx, auto_delta, resp))
            return {{"success", false}, {"message", "Failed to update wipe tower position"}};
        plater->update();
        resp["message"] = "Moved wipe tower by (" + std::to_string((int)auto_delta.x()) + ", " + std::to_string((int)auto_delta.y()) + ", " + std::to_string((int)auto_delta.z()) + ") mm";
        BOOST_LOG_TRIVIAL(info) << "[SlicerBridge] " << resp["message"].get<std::string>();
        return resp;
    }

    plater->take_snapshot(target_obj_indices.size() > 1 ? "Move Multiple Objects" : "Move Object");

    // When moving to a plate center, mirror Selection::center() exactly:
    //   delta = plate_center - group_bounding_box_center  (XY only)
    // The whole group moves as one unit; relative positions are preserved.
    // We do NOT call add_to_plate here — plate membership is updated automatically
    // via notify_instance_update after the coordinates are applied.
    Vec3d center_delta = Vec3d::Zero();
    if (has_target_plate && !has_relative_offset && !has_auto_mode) {
        PartPlate* target_plate = plate_list.get_plate(target_plate_idx);
        if (!target_plate)
            return {{"success", false}, {"message", "Target plate not found"}};

        // Compute combined bounding box of all target instances at their current positions.
        BoundingBoxf3 group_bbox;
        bool group_bbox_valid = false;
        for (int idx : target_obj_indices) {
            ModelObject* obj = model.objects[idx];
            if (!obj || obj->instances.empty()) continue;
            const BoundingBoxf3 obj_bbox = obj->instance_convex_hull_bounding_box(size_t(0));
            if (obj_bbox.defined) {
                group_bbox.merge(obj_bbox);
                group_bbox_valid = true;
            }
        }

        if (group_bbox_valid) {
            const Vec3d src_center = group_bbox.center();
            const Vec3d tar_center = target_plate->get_center_origin();
            center_delta = Vec3d(tar_center.x() - src_center.x(), tar_center.y() - src_center.y(), 0.0);
        }
    }

    int moved_count = 0;
    int moved_instances = 0;
    int eligible_count = 0;
    Vec3d total_applied_delta = Vec3d::Zero();
    for (int idx : target_obj_indices) {
        ModelObject* obj = model.objects[idx];
        if (!obj || obj->instances.empty())
            continue;
        ++eligible_count;

        bool object_changed = false;

        if (has_target_plate && (has_relative_offset || has_auto_mode)) {
            // Plate reassignment with explicit offset or auto mode.
            for (int ii = 0; ii < (int)obj->instances.size(); ++ii) {
                if (plate_list.add_to_plate(idx, ii, target_plate_idx) == 0) {
                    ++moved_instances;
                    object_changed = true;
                }
            }
        } else if (has_target_plate) {
            // Pure center-to-plate: coordinates will be updated below via center_delta.
            // Count instances; plate membership will be refreshed by notify_instance_update.
            for (int ii = 0; ii < (int)obj->instances.size(); ++ii) {
                ++moved_instances;
                object_changed = true;
            }
        }

        Vec3d object_delta(dx + center_delta.x(), dy + center_delta.y(), dz + center_delta.z());
        if (has_auto_mode) {
            const int plate_idx = resolve_plate_for_object(idx);
            PartPlate* plate = plate_idx >= 0 && plate_idx < plate_count ? plate_list.get_plate(plate_idx) : nullptr;
            if (!plate)
                return {{"success", false}, {"message", "Target plate not found for auto move"}};
            const BoundingBoxf3 moving_bbox = obj->instance_convex_hull_bounding_box(size_t(0));
            std::string auto_error;
            if (auto_fix_bounds && !resolve_bounds_delta(moving_bbox, plate->get_build_volume(), bounds_margin_mm, object_delta, auto_error))
                return {{"success", false}, {"message", auto_error}};
            if (auto_fix_clearance && !resolve_clearance_delta(moving_bbox, idx, plate_idx, plate->get_build_volume(), bounds_margin_mm, object_delta, auto_error)) {
                const std::string code = auto_error == "No collision-free clearance move found within the current plate"
                    ? "NO_COLLISION_FREE_PLACEMENT"
                    : "CLEARANCE_REPAIR_FAILED";
                return {{"success", false}, {"code", code}, {"message", auto_error}};
            }
            if (auto_fix_clearance) {
                BoundingBoxf3 moved_bbox(moving_bbox.min + object_delta, moving_bbox.max + object_delta);
                Vec3d bounds_delta = Vec3d::Zero();
                if (!resolve_bounds_delta(moved_bbox, plate->get_build_volume(), bounds_margin_mm, bounds_delta, auto_error))
                    return {{"success", false}, {"message", auto_error}};
                object_delta += bounds_delta;
            }
        }

        if (!nearly_zero(object_delta.x()) || !nearly_zero(object_delta.y()) || !nearly_zero(object_delta.z())) {
            for (ModelInstance* inst : obj->instances) {
                Vec3d cur_offset = inst->get_offset();
                inst->set_offset(cur_offset + object_delta);
            }
            total_applied_delta += object_delta;
            object_changed = true;
        }

        if (object_changed) {
            obj->ensure_on_bed();
            ++moved_count;
        }
    }

    if (moved_count == 0) {
        if (has_auto_mode && eligible_count > 0)
            return {{"success", true}, {"message", "Object already satisfies the requested move constraints"}, {"applied_delta", {0.0, 0.0, 0.0}}};
        return {{"success", false}, {"message", "Target object has no instance"}};
    }

    for (int idx : target_obj_indices) {
        ModelObject* obj = model.objects[idx];
        if (!obj)
            continue;
        for (int ii = 0; ii < (int)obj->instances.size(); ++ii)
            plate_list.notify_instance_update(idx, ii, false);
    }

    plater->update();
    if (auto* obj_list = wxGetApp().obj_list()) {
        obj_list->reload_all_plates(true);
        for (int idx : target_obj_indices)
            obj_list->update_info_items((size_t)idx);
    }

    std::string msg;
    if (has_target_plate && (has_relative_offset || has_auto_mode)) {
        msg = "Moved " + std::to_string(moved_count) + " objects to plate " + std::to_string(target_plate_idx + 1) +
              " and offset by (" + std::to_string((int)total_applied_delta.x()) + ", " + std::to_string((int)total_applied_delta.y()) + ", " + std::to_string((int)total_applied_delta.z()) + ") mm";
    } else if (has_target_plate) {
        msg = "Moved " + std::to_string(moved_count) + " objects to plate " + std::to_string(target_plate_idx + 1) + " center";
    } else if (moved_count == 1) {
        const int idx = target_obj_indices.front();
        const std::string name = (idx >= 0 && idx < (int)model.objects.size() && model.objects[idx] && !model.objects[idx]->name.empty())
            ? model.objects[idx]->name : std::to_string(idx);
        msg = "Moved " + name + " by (" + std::to_string((int)total_applied_delta.x()) + ", " + std::to_string((int)total_applied_delta.y()) + ", " + std::to_string((int)total_applied_delta.z()) + ") mm";
    } else {
        msg = "Moved " + std::to_string(moved_count) + " objects by (" + std::to_string((int)total_applied_delta.x()) + ", " + std::to_string((int)total_applied_delta.y()) + ", " + std::to_string((int)total_applied_delta.z()) + ") mm";
    }

    BOOST_LOG_TRIVIAL(info) << "[SlicerBridge] " << msg;
    json resp = {
        {"success", true},
        {"message", msg},
        {"applied_delta", {total_applied_delta.x(), total_applied_delta.y(), total_applied_delta.z()}},
    };
    if (has_target_plate) {
        resp["target_plate_index"] = target_plate_idx;
        resp["target_plate_number"] = target_plate_idx + 1;
        resp["moved_instances"] = moved_instances;
    }
    return resp;
}
json SlicerBridge::DoRotateObject(const json& params)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    std::string target_name = params.value("object_name", "");
    int obj_idx = params.value("object_index", -1);
    double rx_deg = params.value("rx_deg", 0.0);
    double ry_deg = params.value("ry_deg", 0.0);
    double rz_deg = params.value("rz_deg", 0.0);

    if (rx_deg == 0.0 && ry_deg == 0.0 && rz_deg == 0.0)
        return {{"success", false}, {"message", "At least one of rx_deg/ry_deg/rz_deg must be non-zero"}};

    Model& model = plater->model();
    std::vector<int> target_obj_indices;
    std::unordered_set<int> seen;
    auto add_target = [&](int idx) {
        if (idx >= 0 && idx < (int)model.objects.size() && seen.insert(idx).second)
            target_obj_indices.push_back(idx);
    };

    // Check if plate scope is explicitly requested
    const bool has_plate_param =
        params.contains("plate_number") || params.contains("plateNumber") ||
        params.contains("plate_no") || params.contains("plateNo") ||
        params.contains("target_plate_number") || params.contains("targetPlateNumber") ||
        params.contains("target_plate") || params.contains("targetPlate") ||
        params.contains("plate_index") || params.contains("plateIndex") ||
        params.contains("plate");

    if (params.contains("object_indices") && params["object_indices"].is_array()) {
        for (const auto& it : params["object_indices"]) {
            if (it.is_number_integer())
                add_target(it.get<int>());
            else if (it.is_string()) {
                try { add_target(std::stoi(it.get<std::string>())); } catch (...) {}
            }
        }
    }

    // When plate_number is specified, ignore object_index from params
    if (obj_idx >= 0 && !has_plate_param)
        add_target(obj_idx);

    if (target_obj_indices.empty() && !target_name.empty()) {
        for (int i = 0; i < (int)model.objects.size(); ++i) {
            if (model.objects[i] && model.objects[i]->name == target_name) {
                add_target(i);
                break;
            }
        }
    }

    // ---- Try current selection (only when no explicit plate scope) ----
    if (target_obj_indices.empty() && !has_plate_param) {
        if (auto* canvas = plater->canvas3D()) {
            const Selection& selection = canvas->get_selection();
            const auto& selected_content = selection.get_content();
            for (const auto& kv : selected_content)
                add_target(kv.first);
        }
    }

    if (target_obj_indices.empty() && model.objects.size() == 1)
        add_target(0);

    if (target_obj_indices.empty()) {
        // Fallback: when no explicit target and no selection, operate on all models
        // on the current plate.
        PartPlateList& plate_list = plater->get_partplate_list();
        const int current_plate = plate_list.get_curr_plate_index();
        const int plate_count = plate_list.get_plate_count();
        if (current_plate >= 0 && current_plate < plate_count) {
            PartPlate* plate = plate_list.get_plate(current_plate);
            if (plate) {
                ModelObjectPtrs objs = plate->get_objects_on_this_plate();
                for (const ModelObject* obj : objs) {
                    for (int oi = 0; oi < (int)model.objects.size(); ++oi) {
                        if (model.objects[oi] == obj) {
                            add_target(oi);
                            break;
                        }
                    }
                }
                // Fallback: try intersect check if still empty
                if (target_obj_indices.empty()) {
                    for (int oi = 0; oi < (int)model.objects.size(); ++oi) {
                        const ModelObject* mobj = model.objects[oi];
                        if (!mobj || mobj->instances.empty()) continue;
                        for (int ii = 0; ii < (int)mobj->instances.size(); ++ii) {
                            if (plate->intersect_instance(oi, ii)) {
                                add_target(oi);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    if (target_obj_indices.empty())
        return {{"success", false}, {"message", "Target object not found"}};

    plater->take_snapshot(target_obj_indices.size() > 1 ? "Rotate Multiple Objects" : "Rotate Object");

    constexpr double kDegToRad = 3.14159265358979323846 / 180.0;
    Vec3d delta(rx_deg * kDegToRad, ry_deg * kDegToRad, rz_deg * kDegToRad);

    int rotated_count = 0;
    for (int idx : target_obj_indices) {
        ModelObject* obj = model.objects[idx];
        if (!obj || obj->instances.empty())
            continue;

        for (ModelInstance* inst : obj->instances)
            inst->set_rotation(inst->get_rotation() + delta);

        obj->ensure_on_bed();
        ++rotated_count;
    }

    if (rotated_count == 0)
        return {{"success", false}, {"message", "Target object has no instance"}};

    PartPlateList& plate_list = plater->get_partplate_list();
    for (int idx : target_obj_indices) {
        ModelObject* obj = model.objects[idx];
        if (!obj)
            continue;
        for (int ii = 0; ii < (int)obj->instances.size(); ++ii)
            plate_list.notify_instance_update(idx, ii, false);
    }

    plater->update();

    std::string msg;
    if (rotated_count == 1) {
        const int idx = target_obj_indices.front();
        const std::string name = (idx >= 0 && idx < (int)model.objects.size() && model.objects[idx] && !model.objects[idx]->name.empty())
            ? model.objects[idx]->name : std::to_string(idx);
        msg = "Rotated " + name + " by (" + std::to_string((int)rx_deg) + ", " + std::to_string((int)ry_deg) + ", " + std::to_string((int)rz_deg) + ") deg";
    } else {
        msg = "Rotated " + std::to_string(rotated_count) + " objects by (" + std::to_string((int)rx_deg) + ", " + std::to_string((int)ry_deg) + ", " + std::to_string((int)rz_deg) + ") deg";
    }

    BOOST_LOG_TRIVIAL(info) << "[SlicerBridge] " << msg;
    return {{"success", true}, {"message", msg}};
}

json SlicerBridge::DoScaleObject(const json& params)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    std::string target_name = params.value("object_name", "");
    int obj_idx = params.value("object_index", -1);
    double sx = params.value("sx", 1.0);
    double sy = params.value("sy", 1.0);
    double sz = params.value("sz", 1.0);

    if (sx <= 0.0 || sy <= 0.0 || sz <= 0.0)
        return {{"success", false}, {"message", "sx/sy/sz must be > 0"}};
    if (sx == 1.0 && sy == 1.0 && sz == 1.0)
        return {{"success", false}, {"message", "At least one of sx/sy/sz must differ from 1"}};

    auto trim = [](std::string s) {
        auto is_ws = [](unsigned char ch) { return std::isspace(ch) != 0; };
        while (!s.empty() && is_ws((unsigned char)s.front())) s.erase(s.begin());
        while (!s.empty() && is_ws((unsigned char)s.back())) s.pop_back();
        return s;
    };

    auto to_lower_ascii = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        return s;
    };

    auto parse_int_relaxed = [&trim](const json& value, int& out) -> bool {
        try {
            if (value.is_number_integer()) {
                out = value.get<int>();
                return true;
            }
            if (value.is_number()) {
                out = (int)std::llround(value.get<double>());
                return true;
            }
            if (value.is_string()) {
                const std::string s = trim(value.get<std::string>());
                if (s.empty())
                    return false;

                try {
                    size_t pos = 0;
                    int v = std::stoi(s, &pos);
                    if (pos == s.size()) {
                        out = v;
                        return true;
                    }
                } catch (...) {
                }

                std::string token;
                bool started = false;
                for (unsigned char ch : s) {
                    if (!started) {
                        if (std::isdigit(ch) || ch == '+' || ch == '-') {
                            token.push_back((char)ch);
                            started = true;
                        }
                    } else {
                        if (std::isdigit(ch))
                            token.push_back((char)ch);
                        else
                            break;
                    }
                }

                if (!token.empty() && token != "+" && token != "-") {
                    out = std::stoi(token);
                    return true;
                }
            }
        } catch (...) {
        }
        return false;
    };

    // ---- Resolve scope ----
    std::string scope = to_lower_ascii(trim(params.value("scope", "")));

    // Check if plate scope is explicitly requested
    const bool has_plate_param =
        params.contains("plate_number") || params.contains("plateNumber") ||
        params.contains("plate_no") || params.contains("plateNo") ||
        params.contains("target_plate_number") || params.contains("targetPlateNumber") ||
        params.contains("target_plate") || params.contains("targetPlate") ||
        params.contains("plate_index") || params.contains("plateIndex") ||
        params.contains("plate");

    // When plate_number is explicitly provided, ignore object_index/object_name
    // so that "盘2缩小50%" operates on all models in plate 2, not just one object
    const bool has_object_param =
        params.contains("object_name") || params.contains("object_index") || params.contains("object_indices");

    const bool has_explicit_object_target = has_object_param && !has_plate_param;

    if (scope.empty()) {
        if (has_explicit_object_target)
            scope = "object";
        else if (has_plate_param)
            scope = "plate";
        else
            scope = "object";  // default: object-level targeting
    }

    if (scope == "all_objects") scope = "all";
    if (scope == "current_plate" || scope == "plate_objects") scope = "plate";
    if (scope == "by_object") scope = "object";

    Model& model = plater->model();
    std::vector<int> target_obj_indices;
    std::unordered_set<int> seen;
    auto add_target = [&](int idx) {
        if (idx >= 0 && idx < (int)model.objects.size() && seen.insert(idx).second)
            target_obj_indices.push_back(idx);
    };

    // ---- Collect targets by explicit object params ----
    if (params.contains("object_indices") && params["object_indices"].is_array()) {
        for (const auto& it : params["object_indices"]) {
            int idx = -1;
            if (parse_int_relaxed(it, idx))
                add_target(idx);
        }
    }

    // When plate_number is specified, ignore object_index from params
    if (obj_idx >= 0 && !has_plate_param)
        add_target(obj_idx);

    if (target_obj_indices.empty() && !target_name.empty()) {
        for (int i = 0; i < (int)model.objects.size(); ++i) {
            if (model.objects[i] && model.objects[i]->name == target_name) {
                add_target(i);
                break;
            }
        }
    }

    // ---- Try current selection (only when no explicit plate scope) ----
    if (target_obj_indices.empty() && scope != "plate") {
        if (auto* canvas = plater->canvas3D()) {
            const Selection& selection = canvas->get_selection();
            const auto& selected_content = selection.get_content();
            for (const auto& kv : selected_content)
                add_target(kv.first);
        }
    }

    // ---- Single-object fallback ----
    if (target_obj_indices.empty() && model.objects.size() == 1)
        add_target(0);

    // ---- Scope-based collection when no explicit target found ----
    // When scope=object but no target found, auto-fallback to plate scope
    // so that "shrink" / "scale" commands work even when no model is selected.
    if (target_obj_indices.empty()) {
        if (scope == "object")
            scope = "plate";  // auto-fallback: try current plate

        if (scope == "all") {
            for (int i = 0; i < (int)model.objects.size(); ++i)
                add_target(i);
        } else if (scope == "plate") {
            PartPlateList& plate_list = plater->get_partplate_list();
            const int plate_count = plate_list.get_plate_count();
            if (plate_count <= 0)
                return {{"success", false}, {"message", "No plate available"}};

            auto parse_key = [&](const char* key, int& out) -> bool {
                return params.contains(key) && parse_int_relaxed(params[key], out);
            };

            auto find_plate_by_public_index = [&](int requested) -> int {
                for (int pi = 0; pi < plate_count; ++pi) {
                    PartPlate* plate = plate_list.get_plate(pi);
                    if (plate && plate->get_index() == requested)
                        return pi;
                }
                return -1;
            };

            auto resolve_plate_index_like = [&](int requested) -> int {
                if (requested >= 0 && requested < plate_count)
                    return requested;
                return find_plate_by_public_index(requested);
            };

            auto resolve_plate_number = [&](int plate_number) -> int {
                if (plate_number > 0 && plate_number <= plate_count)
                    return plate_number - 1;
                return -1;
            };

            int selected_plate_idx = -1;
            int requested_plate = -1;
            bool has_requested_plate = false;

            if (parse_key("plate_number", requested_plate) || parse_key("plateNumber", requested_plate) ||
                parse_key("target_plate_number", requested_plate) || parse_key("targetPlateNumber", requested_plate)) {
                selected_plate_idx = resolve_plate_number(requested_plate);
                has_requested_plate = true;
            } else if (parse_key("plate", requested_plate)) {
                selected_plate_idx = resolve_plate_number(requested_plate);
                if (selected_plate_idx < 0)
                    selected_plate_idx = resolve_plate_index_like(requested_plate);
                has_requested_plate = true;
            } else if (parse_key("plate_index", requested_plate) || parse_key("plateIndex", requested_plate)) {
                selected_plate_idx = resolve_plate_index_like(requested_plate);
                has_requested_plate = true;
            }

            if (!has_requested_plate) {
                const int current_plate = plate_list.get_curr_plate_index();
                selected_plate_idx = resolve_plate_index_like(current_plate);
                if (selected_plate_idx < 0 && current_plate >= 0 && current_plate < plate_count)
                    selected_plate_idx = current_plate;
            }

            if (selected_plate_idx < 0 || selected_plate_idx >= plate_count)
                return {{"success", false}, {"message", "Invalid target plate. Prefer plate_number (1-based UI number)."}};

            PartPlate* target_plate = plate_list.get_plate(selected_plate_idx);
            if (!target_plate)
                return {{"success", false}, {"message", "Target plate not available"}};

            ModelObjectPtrs plate_objs = target_plate->get_objects_on_this_plate();
            for (const ModelObject* pobj : plate_objs) {
                for (int oi = 0; oi < (int)model.objects.size(); ++oi) {
                    if (model.objects[oi] == pobj) {
                        add_target(oi);
                        break;
                    }
                }
            }

            // Fallback: if no objects found via get_objects_on_this_plate(), try intersect check
            if (target_obj_indices.empty()) {
                for (int oi = 0; oi < (int)model.objects.size(); ++oi) {
                    const ModelObject* obj = model.objects[oi];
                    if (!obj || obj->instances.empty()) continue;
                    for (int ii = 0; ii < (int)obj->instances.size(); ++ii) {
                        if (target_plate->intersect_instance(oi, ii)) {
                            add_target(oi);
                            break;
                        }
                    }
                }
            }
        }
    }

    if (target_obj_indices.empty())
        return {{"success", false}, {"message", "Target object not found"}};

    plater->take_snapshot(target_obj_indices.size() > 1 ? "Scale Multiple Objects" : "Scale Object");

    Vec3d scale_mul(sx, sy, sz);
    int scaled_count = 0;
    for (int idx : target_obj_indices) {
        ModelObject* obj = model.objects[idx];
        if (!obj || obj->instances.empty())
            continue;

        for (ModelInstance* inst : obj->instances) {
            Vec3d cur_scale = inst->get_scaling_factor();
            inst->set_scaling_factor(cur_scale.cwiseProduct(scale_mul));
        }

        BoundingBoxf3 bb = obj->instance_bounding_box(0);
        const double min_z = bb.min.z();
        if ((min_z > 1e-6 || min_z < -1e-6) && !obj->instances.empty()) {
            Vec3d off = obj->instances[0]->get_offset();
            off.z() -= min_z;
            obj->instances[0]->set_offset(off);
        }

        obj->ensure_on_bed();
        ++scaled_count;
    }

    if (scaled_count == 0)
        return {{"success", false}, {"message", "Target object has no instance"}};

    PartPlateList& plate_list = plater->get_partplate_list();
    for (int idx : target_obj_indices) {
        ModelObject* obj = model.objects[idx];
        if (!obj)
            continue;
        for (int ii = 0; ii < (int)obj->instances.size(); ++ii)
            plate_list.notify_instance_update(idx, ii, false);
    }

    plater->update();

    std::string msg;
    if (scaled_count == 1) {
        const int idx = target_obj_indices.front();
        const std::string name = (idx >= 0 && idx < (int)model.objects.size() && model.objects[idx] && !model.objects[idx]->name.empty())
            ? model.objects[idx]->name : std::to_string(idx);
        msg = "Scaled " + name + " by factors (" + std::to_string(sx) + ", " + std::to_string(sy) + ", " + std::to_string(sz) + ")";
    } else {
        msg = "Scaled " + std::to_string(scaled_count) + " objects by factors (" + std::to_string(sx) + ", " + std::to_string(sy) + ", " + std::to_string(sz) + ")";
    }

    BOOST_LOG_TRIVIAL(info) << "[SlicerBridge] " << msg;
    json result = {
        {"success", true},
        {"message", msg},
        {"scope", scope},
        {"scaled_count", scaled_count},
        {"object_indices", target_obj_indices}
    };
    return result;
}

json SlicerBridge::DoSelectObjects(const json& params)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    Model& model = plater->model();
    if (model.objects.empty())
        return {{"success", false}, {"message", "No model in scene"}};

    auto* canvas = plater->canvas3D();
    if (!canvas)
        return {{"success", false}, {"message", "Canvas not available"}};

    auto trim = [](std::string s) {
        auto is_ws = [](unsigned char ch) { return std::isspace(ch) != 0; };
        while (!s.empty() && is_ws((unsigned char)s.front())) s.erase(s.begin());
        while (!s.empty() && is_ws((unsigned char)s.back())) s.pop_back();
        return s;
    };

    auto to_lower_ascii = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        return s;
    };

    auto parse_int = [&trim](const json& value, int& out) -> bool {
        try {
            if (value.is_number_integer()) {
                out = value.get<int>();
                return true;
            }
            if (value.is_number()) {
                out = (int)std::llround(value.get<double>());
                return true;
            }
            if (value.is_string()) {
                const std::string s = trim(value.get<std::string>());
                if (s.empty())
                    return false;
                out = std::stoi(s);
                return true;
            }
        } catch (...) {
        }
        return false;
    };

    bool append = false;
    if (params.contains("append")) {
        const json& append_value = params["append"];
        if (append_value.is_boolean()) {
            append = append_value.get<bool>();
        } else if (append_value.is_number_integer()) {
            append = append_value.get<int>() != 0;
        } else if (append_value.is_string()) {
            const std::string v = to_lower_ascii(trim(append_value.get<std::string>()));
            append = (v == "1" || v == "true" || v == "yes" || v == "on");
        }
    }

    std::string scope = to_lower_ascii(trim(params.value("scope", "")));
    if (scope.empty()) {
        if (params.contains("plate_number") || params.contains("plateNumber") ||
            params.contains("plate_no") || params.contains("plateNo") ||
            params.contains("target_plate_number") || params.contains("targetPlateNumber") ||
            params.contains("target_plate") || params.contains("targetPlate") ||
            params.contains("plate_index") || params.contains("plateIndex") ||
            params.contains("plate"))
            scope = "plate";
        else if (params.contains("color") || params.contains("extruder_id"))
            scope = "color";
        else if (params.contains("object_name") || params.contains("object_names") || params.contains("object_index") || params.contains("object_indices"))
            scope = "object";
        else
            scope = "all";
    }

    if (scope == "all_objects") scope = "all";
    if (scope == "current_plate" || scope == "plate_objects") scope = "plate";
    if (scope == "by_color") scope = "color";
    if (scope == "by_object") scope = "object";

    std::vector<int> target_obj_indices;
    std::unordered_set<int> seen;
    int selected_plate_idx = -1;
    auto add_target = [&](int idx) {
        if (idx >= 0 && idx < (int)model.objects.size() && model.objects[idx] && seen.insert(idx).second)
            target_obj_indices.push_back(idx);
    };

    auto collect_by_name = [&](const std::string& input_name) {
        const std::string name = trim(input_name);
        if (name.empty())
            return;

        const size_t before_count = target_obj_indices.size();
        for (int i = 0; i < (int)model.objects.size(); ++i) {
            if (model.objects[i] && model.objects[i]->name == name)
                add_target(i);
        }

        if (target_obj_indices.size() == before_count) {
            const std::string wanted = to_lower_ascii(name);
            for (int i = 0; i < (int)model.objects.size(); ++i) {
                if (model.objects[i] && to_lower_ascii(model.objects[i]->name) == wanted)
                    add_target(i);
            }
        }
    };

    auto collect_from_object_params = [&]() {
        if (params.contains("object_indices")) {
            const json& indices = params["object_indices"];
            if (indices.is_array()) {
                for (const auto& item : indices) {
                    int idx = -1;
                    if (parse_int(item, idx))
                        add_target(idx);
                }
            } else {
                int idx = -1;
                if (parse_int(indices, idx))
                    add_target(idx);
            }
        }

        if (params.contains("object_index")) {
            int idx = -1;
            if (parse_int(params["object_index"], idx))
                add_target(idx);
        }

        if (params.contains("object_name") && params["object_name"].is_string())
            collect_by_name(params["object_name"].get<std::string>());

        if (params.contains("object_names") && params["object_names"].is_array()) {
            for (const auto& item : params["object_names"]) {
                if (item.is_string())
                    collect_by_name(item.get<std::string>());
            }
        }
    };

    if (scope == "all") {
        for (int i = 0; i < (int)model.objects.size(); ++i)
            add_target(i);
    } else if (scope == "object") {
        collect_from_object_params();
        if (target_obj_indices.empty() && model.objects.size() == 1)
            add_target(0);
    } else if (scope == "plate") {
        PartPlateList& plate_list = plater->get_partplate_list();
        const int plate_count = plate_list.get_plate_count();
        if (plate_count <= 0)
            return {{"success", false}, {"message", "No plate available"}};

        auto parse_key = [&](const char* key, int& out) -> bool {
            return params.contains(key) && parse_int(params[key], out);
        };

        auto find_plate_by_public_index = [&](int requested) -> int {
            for (int pi = 0; pi < plate_count; ++pi) {
                PartPlate* plate = plate_list.get_plate(pi);
                if (plate && plate->get_index() == requested)
                    return pi;
            }
            return -1;
        };

        auto resolve_plate_index_like = [&](int requested) -> int {
            // Compatibility-first behavior for LLM outputs:
            // - Positive values are treated as UI plate numbers (0-based).
            // - Zero maps to the first plate.
            // This keeps plate selection consistent with natural-language requests.
            if (requested >= 0 && requested < plate_count)
                return requested;

            return find_plate_by_public_index(requested);
        };

        auto resolve_plate_number = [&](int plate_number) -> int {
            if (plate_number > 0 && plate_number <= plate_count)
                return plate_number - 1;
            return -1;
        };
        bool has_requested_plate = false;
        int requested_plate = -1;
        if (parse_key("plate_number", requested_plate) || parse_key("plateNumber", requested_plate) ||
            parse_key("target_plate_number", requested_plate) || parse_key("targetPlateNumber", requested_plate)) {
            selected_plate_idx = resolve_plate_number(requested_plate);
            has_requested_plate = true;
        } else if (parse_key("plate", requested_plate)) {
            selected_plate_idx = resolve_plate_number(requested_plate);
            if (selected_plate_idx < 0)
                selected_plate_idx = resolve_plate_index_like(requested_plate);
            has_requested_plate = true;
        } else if (parse_key("plate_index", requested_plate) || parse_key("plateIndex", requested_plate)) {
            selected_plate_idx = resolve_plate_index_like(requested_plate);
            has_requested_plate = true;
        }

        if (!has_requested_plate) {
            const int current_plate = plate_list.get_curr_plate_index();
            selected_plate_idx = resolve_plate_index_like(current_plate);
            if (selected_plate_idx < 0 && current_plate >= 0 && current_plate < plate_count)
                selected_plate_idx = current_plate;
        }

        if (selected_plate_idx < 0 || selected_plate_idx >= plate_count)
            return {{"success", false}, {"message", "Invalid target plate. Prefer plate_number (1-based UI number)."}};

        PartPlate* target_plate = plate_list.get_plate(selected_plate_idx);
        if (!target_plate)
            return {{"success", false}, {"message", "Target plate not available"}};

        ModelObjectPtrs plate_objs = target_plate->get_objects_on_this_plate();
        for (const ModelObject* pobj : plate_objs) {
            for (int oi = 0; oi < (int)model.objects.size(); ++oi) {
                if (model.objects[oi] == pobj) {
                    add_target(oi);
                    break;
                }
            }
        }
        // Fallback: if no objects found via get_objects_on_this_plate(), try intersect check
        if (target_obj_indices.empty()) {
            for (int oi = 0; oi < (int)model.objects.size(); ++oi) {
                const ModelObject* obj = model.objects[oi];
                if (!obj || obj->instances.empty()) continue;
                for (int ii = 0; ii < (int)obj->instances.size(); ++ii) {
                    if (target_plate->intersect_instance(oi, ii)) {
                        add_target(oi);
                        break;
                    }
                }
            }
        }
    } else if (scope == "color") {
        const std::vector<std::string> extruder_colors = plater->get_extruder_colors_from_plater_config();

        auto normalize_hex = [](std::string s) -> std::string {
            s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char ch) { return std::isspace(ch) != 0; }), s.end());
            if (!s.empty() && s.front() == '#') s.erase(s.begin());
            if (s.size() == 3) {
                std::string x;
                x.reserve(6);
                x.push_back(s[0]); x.push_back(s[0]);
                x.push_back(s[1]); x.push_back(s[1]);
                x.push_back(s[2]); x.push_back(s[2]);
                s = x;
            }
            if (s.size() != 6) return "";
            for (unsigned char ch : s) {
                if (!std::isxdigit(ch)) return "";
            }
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
            return "#" + s;
        };

        auto hex_to_rgb = [&](const std::string& hex, int& r, int& g, int& b) -> bool {
            std::string n = normalize_hex(hex);
            if (n.empty()) return false;
            try {
                r = std::stoi(n.substr(1, 2), nullptr, 16);
                g = std::stoi(n.substr(3, 2), nullptr, 16);
                b = std::stoi(n.substr(5, 2), nullptr, 16);
                return true;
            } catch (...) {
                return false;
            }
        };

        int target_extruder_id = 0;
        if (params.contains("extruder_id"))
            parse_int(params["extruder_id"], target_extruder_id);

        std::string input_color = trim(params.value("color", ""));
        if (target_extruder_id <= 0 && !input_color.empty()) {
            const std::string color_lower = to_lower_ascii(input_color);
            static const std::unordered_map<std::string, std::string> kColorAliases = {
                {"red", "#ff0000"}, {"green", "#00ff00"}, {"blue", "#0000ff"},
                {"yellow", "#ffff00"}, {"black", "#000000"}, {"white", "#ffffff"},
                {"orange", "#ffa500"}, {"purple", "#800080"}, {"pink", "#ff69b4"},
                {"gray", "#808080"}, {"grey", "#808080"}, {"cyan", "#00ffff"},
                {"teal", "#008080"},
                {"hong", "#ff0000"}, {"lv", "#00ff00"}, {"lan", "#0000ff"},
                {"huang", "#ffff00"}, {"hei", "#000000"}, {"bai", "#ffffff"},
                {"cheng", "#ffa500"}, {"zi", "#800080"}, {"hui", "#808080"},
                {"qing", "#00ffff"},
                {"hongse", "#ff0000"}, {"lvse", "#00ff00"}, {"lanse", "#0000ff"},
                {"huangse", "#ffff00"}, {"heise", "#000000"}, {"baise", "#ffffff"},
                {"chengse", "#ffa500"}, {"zise", "#800080"}, {"huise", "#808080"},
                {"qingse", "#00ffff"}
            };

            std::string wanted_hex;
            auto it_alias = kColorAliases.find(color_lower);
            if (it_alias != kColorAliases.end())
                wanted_hex = it_alias->second;
            else
                wanted_hex = normalize_hex(input_color);

            if (wanted_hex.empty()) {
                bool is_digit_string = !color_lower.empty() &&
                                       std::all_of(color_lower.begin(), color_lower.end(), [](unsigned char c) { return std::isdigit(c) != 0; });
                if (is_digit_string) {
                    try { target_extruder_id = std::stoi(color_lower); } catch (...) {}
                } else {
                    return {{"success", false}, {"message", "Invalid color value: " + input_color}};
                }
            } else {
                if (extruder_colors.empty())
                    return {{"success", false}, {"message", "No filament colors available for color mapping"}};

                int wr = 0, wg = 0, wb = 0;
                if (!hex_to_rgb(wanted_hex, wr, wg, wb))
                    return {{"success", false}, {"message", "Invalid color value: " + input_color}};

                int best_idx = -1;
                int best_dist = std::numeric_limits<int>::max();
                for (int i = 0; i < (int)extruder_colors.size(); ++i) {
                    int cr = 0, cg = 0, cb = 0;
                    if (!hex_to_rgb(extruder_colors[i], cr, cg, cb))
                        continue;
                    const int dr = cr - wr;
                    const int dg = cg - wg;
                    const int db = cb - wb;
                    const int dist = dr * dr + dg * dg + db * db;
                    if (dist < best_dist) {
                        best_dist = dist;
                        best_idx = i;
                    }
                }

                if (best_idx < 0)
                    return {{"success", false}, {"message", "Failed to map color to available filament color"}};

                target_extruder_id = best_idx + 1;
            }
        }

        if (target_extruder_id <= 0)
            return {{"success", false}, {"message", "extruder_id or color is required for scope=color"}};

        if (!extruder_colors.empty() && target_extruder_id > (int)extruder_colors.size()) {
            return {{"success", false},
                    {"message", "extruder_id out of range: " + std::to_string(target_extruder_id) +
                                " (available: 1-" + std::to_string((int)extruder_colors.size()) + ")"}};
        }

        for (int i = 0; i < (int)model.objects.size(); ++i) {
            const ModelObject* obj = model.objects[i];
            if (!obj)
                continue;

            bool has_model_part = false;
            bool matches = false;
            for (const ModelVolume* vol : obj->volumes) {
                if (!vol || !vol->is_model_part())
                    continue;
                has_model_part = true;
                int eid = vol->extruder_id();
                if (eid <= 0) eid = 1;
                if (eid == target_extruder_id) {
                    matches = true;
                    break;
                }
            }

            if (!has_model_part) {
                int obj_eid = 1;
                if (const ConfigOption* obj_opt = obj->config.option("extruder")) {
                    try { obj_eid = std::stoi(obj_opt->serialize()); }
                    catch (...) {}
                    if (obj_eid <= 0) obj_eid = 1;
                }
                matches = (obj_eid == target_extruder_id);
            }

            if (matches)
                add_target(i);
        }
    } else {
        return {{"success", false}, {"message", "Invalid scope. Use all|plate|color|object"}};
    }

    if (target_obj_indices.empty())
        return {{"success", false}, {"message", "No objects matched selection condition"}};

    Selection& selection = canvas->get_selection();

    plater->take_snapshot(append ? "Add Object Selection" : "Select Objects");
    {
        Plater::SuppressSnapshots suppress(plater);
        if (!append)
            selection.remove_all();

        for (int idx : target_obj_indices)
            selection.add_object((unsigned int)idx, false);
    }

    plater->update();
    if (auto* obj_list = wxGetApp().obj_list()) {
        obj_list->reload_all_plates(true);
        for (int idx : target_obj_indices)
            obj_list->update_info_items((size_t)idx);
    }

    json selected_objects = json::array();
    for (int idx : target_obj_indices) {
        json item;
        item["object_index"] = idx;
        item["object_name"] = model.objects[idx] ? model.objects[idx]->name : std::to_string(idx);
        selected_objects.push_back(std::move(item));
    }

    std::string msg;
    if (target_obj_indices.size() == 1) {
        const int idx = target_obj_indices.front();
        const std::string name = (idx >= 0 && idx < (int)model.objects.size() && model.objects[idx] && !model.objects[idx]->name.empty())
            ? model.objects[idx]->name : std::to_string(idx);
        msg = append ? ("Added to selection: " + name) : ("Selected object: " + name);
    } else {
        msg = append
            ? ("Added " + std::to_string((int)target_obj_indices.size()) + " objects to selection")
            : ("Selected " + std::to_string((int)target_obj_indices.size()) + " objects");
    }

    return {
        {"success", true},
        {"message", msg},
        {"scope", scope},
        {"append", append},
        {"target_plate_index", selected_plate_idx},
        {"target_plate_number", selected_plate_idx >= 0 ? selected_plate_idx + 1 : 0},
        {"selected_count", (int)target_obj_indices.size()},
        {"object_indices", target_obj_indices},
        {"selected_objects", selected_objects}
    };
}

json SlicerBridge::DoDeleteModel(const json& params)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    Model& model = plater->model();
    if (model.objects.empty())
        return {{"success", false}, {"message", "No model in scene"}};

    // Support plate_number: delete all models on the specified plate
    bool has_plate_param =
        params.contains("plate_number") || params.contains("plateNumber") ||
        params.contains("plate_no")     || params.contains("plateNo")     ||
        params.contains("plate_index")  || params.contains("plateIndex")  ||
        params.contains("plate");

    if (has_plate_param) {
        PartPlateList& plate_list = plater->get_partplate_list();
        const int plate_count = plate_list.get_plate_count();
        int target_plate_idx = -1;

        auto parse_int_key = [&](const std::string& key, int& out) -> bool {
            if (!params.contains(key)) return false;
            const auto& v = params[key];
            if (v.is_number()) { out = (int)std::llround(v.get<double>()); return true; }
            if (v.is_string()) {
                try { out = std::stoi(v.get<std::string>()); return true; } catch (...) {}
            }
            return false;
        };

        int requested = -1;
        if (parse_int_key("plate_number", requested) || parse_int_key("plateNumber", requested) ||
            parse_int_key("plate_no", requested)     || parse_int_key("plateNo", requested)) {
            // plate_number is 1-based
            if (requested > 0 && requested <= plate_count)
                target_plate_idx = requested - 1;
        } else if (parse_int_key("plate_index", requested) || parse_int_key("plateIndex", requested) ||
                   parse_int_key("plate", requested)) {
            // plate_index is 0-based
            if (requested >= 0 && requested < plate_count)
                target_plate_idx = requested;
            else if (requested > 0 && requested <= plate_count)
                target_plate_idx = requested - 1;  // fallback: treat as 1-based
        }

        if (target_plate_idx < 0 || target_plate_idx >= plate_count)
            return {{"success", false}, {"message", "Invalid plate number"}};

        PartPlate* target_plate = plate_list.get_plate(target_plate_idx);
        if (!target_plate)
            return {{"success", false}, {"message", "Plate not found"}};

        // Collect all object indices on this plate (descending order to safely remove)
        std::vector<int> indices_to_delete;
        for (int i = 0; i < (int)model.objects.size(); ++i) {
            if (plate_list.find_instance(i, 0) == target_plate_idx)
                indices_to_delete.push_back(i);
        }

        if (indices_to_delete.empty())
            return {{"success", true}, {"message", "No models on the specified plate"}};

        plater->take_snapshot("Delete models on plate");
        // Delete in descending order so indices remain valid
        std::sort(indices_to_delete.rbegin(), indices_to_delete.rend());
        for (int idx : indices_to_delete)
            plater->remove((size_t)idx);
        plater->update();

        return {{"success", true}, {"message", "Deleted " + std::to_string(indices_to_delete.size()) + " model(s) on plate " + std::to_string(target_plate_idx + 1)}};
    }

    int obj_idx = -1;
    if (params.contains("object_index")) {
        obj_idx = params.value("object_index", -1);
    } else {
        std::string target_name = params.value("object_name", "");
        if (!target_name.empty()) {
            for (int i = 0; i < (int)model.objects.size(); ++i) {
                if (model.objects[i] && model.objects[i]->name == target_name) {
                    obj_idx = i;
                    break;
                }
            }
        }
    }

    if (obj_idx < 0 && model.objects.size() == 1)
        obj_idx = 0;

    if (obj_idx < 0 || obj_idx >= (int)model.objects.size())
        return {{"success", false}, {"message", "Target object not found"}};

    std::string name = model.objects[obj_idx] ? model.objects[obj_idx]->name : std::to_string(obj_idx);
    plater->take_snapshot("Delete " + name);
    plater->remove((size_t)obj_idx);
    plater->update();

    return {{"success", true}, {"message", "Deleted model: " + name}};
}

json SlicerBridge::DoCloneModel(const json& params)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    Model& model = plater->model();
    if (model.objects.empty())
        return {{"success", false}, {"message", "No model in scene"}};

    auto trim = [](std::string s) {
        auto is_ws = [](unsigned char ch) { return std::isspace(ch) != 0; };
        while (!s.empty() && is_ws((unsigned char)s.front())) s.erase(s.begin());
        while (!s.empty() && is_ws((unsigned char)s.back())) s.pop_back();
        return s;
    };

    auto parse_int_relaxed = [&trim](const json& value, int& out) -> bool {
        try {
            if (value.is_number_integer()) {
                out = value.get<int>();
                return true;
            }
            if (value.is_number()) {
                out = (int)std::llround(value.get<double>());
                return true;
            }
            if (value.is_string()) {
                const std::string s = trim(value.get<std::string>());
                if (s.empty())
                    return false;

                try {
                    size_t pos = 0;
                    int v = std::stoi(s, &pos);
                    if (pos == s.size()) {
                        out = v;
                        return true;
                    }
                } catch (...) {
                }

                std::string token;
                bool started = false;
                for (unsigned char ch : s) {
                    if (!started) {
                        if (std::isdigit(ch) || ch == '+' || ch == '-') {
                            token.push_back((char)ch);
                            started = true;
                        }
                    } else {
                        if (std::isdigit(ch))
                            token.push_back((char)ch);
                        else
                            break;
                    }
                }

                if (!token.empty() && token != "+" && token != "-") {
                    out = std::stoi(token);
                    return true;
                }
            }
        } catch (...) {
        }
        return false;
    };

    int copies = 1;
    auto parse_copies_key = [&](const char* key) -> bool {
        if (!params.contains(key))
            return false;
        return parse_int_relaxed(params[key], copies);
    };

    parse_copies_key("copies") ||
    parse_copies_key("count") ||
    parse_copies_key("copy_count") ||
    parse_copies_key("clone_count") ||
    parse_copies_key("num") ||
    parse_copies_key("number") ||
    parse_copies_key("quantity");

    if (copies <= 0) copies = 1;
    if (copies > 20) copies = 20;

    int obj_idx = -1;
    if (params.contains("object_index")) {
        parse_int_relaxed(params["object_index"], obj_idx);
    } else if (params.contains("object_indices") && params["object_indices"].is_array()) {
        const auto& indices = params["object_indices"];
        if (!indices.empty())
            parse_int_relaxed(indices[0], obj_idx);
    } else {
        std::string target_name = params.value("object_name", "");
        if (!target_name.empty()) {
            for (int i = 0; i < (int)model.objects.size(); ++i) {
                if (model.objects[i] && model.objects[i]->name == target_name) {
                    obj_idx = i;
                    break;
                }
            }
        }
    }

    if (obj_idx < 0)
        obj_idx = plater->get_selected_object_idx();

    if (obj_idx < 0 && model.objects.size() == 1)
        obj_idx = 0;

    if (obj_idx < 0 || obj_idx >= (int)model.objects.size())
        return {{"success", false}, {"message", "Target object not found"}};

    ModelObject* src = model.objects[obj_idx];
    if (!src)
        return {{"success", false}, {"message", "Target object is invalid"}};

    const std::string src_name = src->name.empty() ? std::to_string(obj_idx) : src->name;
    auto* canvas = plater->canvas3D();
    if (!canvas)
        return {{"success", false}, {"message", "Canvas not available"}};

    Selection& selection = canvas->get_selection();
    const size_t before_count = model.objects.size();

    {
        Plater::SuppressSnapshots suppress(plater);
        selection.remove_all();
        selection.add_object((unsigned int)obj_idx, true);
    }

    selection.copy_to_clipboard();
    selection.calculate_clone_preview_offsets(copies);
    plater->clone_selection();
    selection.release_clone_preview_info();
    selection.clear_clone_offsets();

    const size_t after_count = model.objects.size();
    const size_t created_count = (after_count > before_count) ? (after_count - before_count) : 0;
    if (created_count == 0)
        return {{"success", false}, {"message", "Clone failed"}};

    plater->update();
    wxGetApp().obj_list()->reload_all_plates(true);
    for (size_t idx = before_count; idx < after_count; ++idx)
        wxGetApp().obj_list()->update_info_items(idx);

    return {
        {"success", true},
        {"message", "Cloned model: " + src_name + " (" + std::to_string((int)created_count) + " copy/copies)"},
        {"requested_copies", copies},
        {"created_copies", (int)created_count}
    };
}

json SlicerBridge::DoRepairMesh(const json& params)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    auto* obj_list = wxGetApp().obj_list();
    if (!obj_list)
        return {{"success", false}, {"message", "Object list not available"}};

    Model& model = plater->model();
    if (model.objects.empty())
        return {{"success", false}, {"message", "No model in scene"}};

    // Check if a gizmo is open — fix_through_netfabb will silently return if so
    auto* canvas3d = plater->get_view3D_canvas3D();
    if (canvas3d && !canvas3d->get_gizmos_manager().check_gizmos_closed_except(GLGizmosManager::Undefined)) {
        return {{"success", false}, {"message", "Cannot repair while a gizmo tool is active. Please close the gizmo first."}};
    }

    json select_params = params.is_object() ? params : json::object();
    const bool has_explicit_target =
        select_params.contains("object_name") ||
        select_params.contains("object_index") ||
        select_params.contains("object_indices");

    bool should_select = has_explicit_target;
    if (!should_select && plater->get_selection().is_empty() && model.objects.size() == 1) {
        select_params["object_index"] = 0;
        should_select = true;
    }

    // If no explicit target and nothing selected and multiple objects, fail with guidance
    if (!should_select && plater->get_selection().is_empty() && model.objects.size() > 1) {
        return {{"success", false}, {"message", "Multiple objects in scene but none selected. Please select the object to repair first."}};
    }

    json selection_result = json::object();
    if (should_select) {
        if (!select_params.contains("scope"))
            select_params["scope"] = "object";

        selection_result = DoSelectObjects(select_params);
        if (!selection_result.value("success", false))
            return selection_result;
    }

    if (!plater->can_fix_through_netfabb()) {
        return {
            {"success", false},
            {"message", "Selected object has no repairable mesh errors or repair is not available on this platform"},
            {"selection", selection_result}
        };
    }

    obj_list->fix_through_netfabb();

    json result = {
        {"success", true},
        {"message", "Mesh repair finished"}
    };

    if (!selection_result.empty())
        result["selection"] = selection_result;

    return result;
}
json SlicerBridge::DoFillBed(const json& params)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    Model& model = plater->model();
    if (model.objects.empty())
        return {{"success", false}, {"message", "No model in scene"}};

    int obj_idx = -1;
    if (params.contains("object_index")) {
        obj_idx = params.value("object_index", -1);
    } else {
        std::string target_name = params.value("object_name", "");
        if (!target_name.empty()) {
            for (int i = 0; i < (int)model.objects.size(); ++i) {
                if (model.objects[i] && model.objects[i]->name == target_name) {
                    obj_idx = i;
                    break;
                }
            }
        }
    }

    if (obj_idx < 0)
        obj_idx = plater->get_selected_object_idx();
    if (obj_idx < 0 && model.objects.size() == 1)
        obj_idx = 0;

    if (obj_idx < 0 || obj_idx >= (int)model.objects.size())
        return {{"success", false}, {"message", "Target object not found"}};

    auto* canvas = plater->canvas3D();
    if (!canvas)
        return {{"success", false}, {"message", "Canvas not available"}};

    Selection& sel = canvas->get_selection();
    sel.remove_all();
    sel.add_object((unsigned int)obj_idx, true);

    plater->fill_bed_with_instances();
    return {{"success", true}, {"message", "Fill-bed job started"}};
}

json SlicerBridge::DoRenamePlate(const json& params)
{
    BOOST_LOG_TRIVIAL(info) << "[DoRenamePlate] ENTERED with params=" << params.dump();
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    PartPlateList& plate_list = plater->get_partplate_list();
    const int plate_count = plate_list.get_plate_count();
    if (plate_count == 0)
        return {{"success", false}, {"message", "No plates available"}};

    // Determine target plate index (0-based)
    // Priority: plate_index > plate_number > current plate
    int target_idx = -1;
    if (params.contains("plate_index") && params["plate_index"].is_number_integer()) {
        target_idx = params["plate_index"].get<int>();
    } else if (params.contains("plate_number") && params["plate_number"].is_number_integer()) {
        // plate_number is 1-based from UI
        target_idx = params["plate_number"].get<int>() - 1;
    } else if (params.contains("plate") && params["plate"].is_number_integer()) {
        // alias of plate_number
        target_idx = params["plate"].get<int>() - 1;
    } else {
        target_idx = plate_list.get_curr_plate_index();
    }

    if (target_idx < 0 || target_idx >= plate_count)
        return {{"success", false}, {"message", "Invalid plate index: " + std::to_string(target_idx)}};

    std::string new_name;
    if (params.contains("name") && params["name"].is_string())
        new_name = params["name"].get<std::string>();

    if (new_name.empty())
        return {{"success", false}, {"message", "Plate name must not be empty"}};

    // fix:[16411] Validate plate name length - limit to 20 characters
    {
        wxString wx_name = from_u8(new_name);
        if (wx_name.Length() > 20) {
            new_name = wx_name.Left(20).ToUTF8().data();
        }
    }

    PartPlate* plate = plate_list.get_plate(target_idx);
    if (!plate)
        return {{"success", false}, {"message", "Plate not found at index " + std::to_string(target_idx)}};

    std::string old_name = plate->get_plate_name();
    plate->set_plate_name(new_name);

    // Trigger a UI update so the name change is visible
    if (target_idx == plate_list.get_curr_plate_index())
        plater->update();

    // Read back the actual name after truncation
    std::string actual_name = plate->get_plate_name();

    return {
        {"success", true},
        {"message", "\xe7\x9b\x98\xe5\x90\x8d\xe5\xb7\xb2\xe4\xbb\x8e \xe2\x80\x98" + old_name + "\xe2\x80\x99 \xe6\x94\xb9\xe4\xb8\xba \xe2\x80\x98" + actual_name + "\xe2\x80\x99"},  // 盘名已从 'X' 改为 'Y'
        {"plate_index", target_idx},
        {"plate_number", target_idx + 1},
        {"old_name", old_name},
        {"new_name", actual_name}
    };
}

json SlicerBridge::DoAddPlate(const json& params)
{
    BOOST_LOG_TRIVIAL(info) << "[DoAddPlate] ENTERED";
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    // 解析要添加的盘数量（默认 1），一次调用内循环添加，
    // 只返回一个汇总结果，避免上层为每个盘渲染独立结果卡片。
    int requested_count = 1;
    try {
        if (params.contains("count")) {
            if (params["count"].is_number_integer())
                requested_count = params["count"].get<int>();
            else if (params["count"].is_number())
                requested_count = (int)std::llround(params["count"].get<double>());
            else if (params["count"].is_string())
                requested_count = std::stoi(params["count"].get<std::string>());
        }
    } catch (...) {
        requested_count = 1;
    }
    if (requested_count < 1)
        requested_count = 1;

    const int max_plates = (int)PartPlateList::MAX_PLATES_COUNT;

    PartPlateList& plate_list = plater->get_partplate_list();

    int added_count = 0;
    int last_plate_index = -1;
    for (int i = 0; i < requested_count; ++i) {
        if (!plater->can_add_plate()) {
            BOOST_LOG_TRIVIAL(warning) << "[DoAddPlate] Stopped at " << added_count << "/" << requested_count
                                       << ": cannot add more plates (max=" << max_plates << ").";
            break;
        }
        plate_list.create_plate();
        last_plate_index = plate_list.get_plate_count() - 1;
        ++added_count;
    }

    if (added_count == 0)
        return {{"success", false}, {"message", "Cannot add plate: maximum plate count reached or background slicing in progress."}};

    plate_list.select_plate(last_plate_index);
    plater->update();

    BOOST_LOG_TRIVIAL(info) << "[DoAddPlate] Added " << added_count << " plate(s). New count="
                            << plate_list.get_plate_count();

    const bool limit_reached = added_count < requested_count;

    // 文案交由 UI 卡片按 i18n 渲染（依据下列结构化字段）。
    // 这里仅提供一个中性的英文兜底 message，避免在客户端硬编码多语言文案。
    std::string message = (added_count == 1)
        ? "Plate added"
        : ("Added " + std::to_string(added_count) + " plates");
    if (limit_reached)
        message += " (reached the maximum of " + std::to_string(max_plates) + " plates)";

    return {
        {"success", true},
        {"message", message},
        {"result_kind", "add_plate"},
        {"added_count", added_count},
        {"requested_count", requested_count},
        {"max_plates", max_plates},
        {"limit_reached", limit_reached},
        {"plate_index", last_plate_index},
        {"plate_number", last_plate_index + 1},
        {"total_plates", plate_list.get_plate_count()}
    };
}

json SlicerBridge::DoDeletePlate(const json& params)
{
    BOOST_LOG_TRIVIAL(info) << "[DoDeletePlate] ENTERED";
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    if (!plater->can_delete_plate())
        return {{"success", false}, {"message", "Cannot delete plate: only one plate remaining."}};

    PartPlateList& plate_list = plater->get_partplate_list();
    const int plate_count = plate_list.get_plate_count();

    // Determine target plate index (0-based)
    int target_idx = -1;
    if (params.contains("plate_index") && params["plate_index"].is_number_integer()) {
        target_idx = params["plate_index"].get<int>();
    } else if (params.contains("plate_number") && params["plate_number"].is_number_integer()) {
        target_idx = params["plate_number"].get<int>() - 1;
    } else {
        target_idx = plate_list.get_curr_plate_index();
    }

    if (target_idx < 0 || target_idx >= plate_count)
        return {{"success", false}, {"message", "Invalid plate index: " + std::to_string(target_idx)}};

    PartPlate* plate = plate_list.get_plate(target_idx);
    const std::string deleted_name = plate ? plate->get_plate_name() : "";

    const int ret = plater->delete_plate(target_idx);
    if (ret != 0)
        return {{"success", false}, {"message", "Failed to delete plate at index " + std::to_string(target_idx)}};

    BOOST_LOG_TRIVIAL(info) << "[DoDeletePlate] Plate deleted. Remaining count=" << plate_list.get_plate_count();

    return {
        {"success", true},
        {"message", "\xe7\x9b\x98\xe5\xb7\xb2\xe5\x88\xa0\xe9\x99\xa4"},  // 盘已删除
        {"deleted_plate_name", deleted_name},
        {"remaining_plates", plate_list.get_plate_count()}
    };
}

json SlicerBridge::DoTogglePreviewLiteMode(const json& params)
{
    BOOST_LOG_TRIVIAL(info) << "[DoTogglePreviewLiteMode] ENTERED";
    auto* app_config = wxGetApp().app_config;
    if (!app_config)
        return {{"success", false}, {"message", "AppConfig not available"}};

    // "精简模式" refers to gcode_preview_lite_mode (G-code preview simplified line type filter)
    bool current_enabled = app_config->get_bool("gcode_preview_lite_mode");
    bool target_enabled;

    if (params.contains("enabled") && params["enabled"].is_boolean()) {
        target_enabled = params["enabled"].get<bool>();
    } else {
        // Toggle current state
        target_enabled = !current_enabled;
    }

    if (target_enabled == current_enabled) {
        std::string mode_label = current_enabled
            ? "\xe7\xb2\xbe\xe7\xae\x80\xe6\xa8\xa1\xe5\xbc\x8f"   // 精简模式
            : "\xe6\xa0\x87\xe5\x87\x86\xe6\xa8\xa1\xe5\xbc\x8f";   // 标准模式
        return {
            {"success", true},
            {"message", "\xe5\xbd\x93\xe5\x89\x8d\xe5\xb7\xb2\xe6\x98\xaf" + mode_label},  // 当前已是...模式
            {"lite_mode", current_enabled}
        };
    }

    // Apply the change
    app_config->set("gcode_preview_lite_mode", target_enabled ? "true" : "false");
    app_config->save();

    // Refresh the GCode preview if currently in preview mode
    auto* plater = wxGetApp().plater();
    if (plater) {
        plater->refresh_print();
    }

    std::string result_label = target_enabled
        ? "\xe7\xb2\xbe\xe7\xae\x80\xe6\xa8\xa1\xe5\xbc\x8f\xe5\xb7\xb2\xe5\xbc\x80\xe5\x90\xaf"   // 精简模式已开启
        : "\xe7\xb2\xbe\xe7\xae\x80\xe6\xa8\xa1\xe5\xbc\x8f\xe5\xb7\xb2\xe5\x85\xb3\xe9\x97\xad";   // 精简模式已关闭

    BOOST_LOG_TRIVIAL(info) << "[DoTogglePreviewLiteMode] gcode_preview_lite_mode set to " << (target_enabled ? "true" : "false");

    return {
        {"success", true},
        {"message", result_label},
        {"lite_mode", target_enabled}
    };
}

json SlicerBridge::DoSimplifyModel(const json& params)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {{"message", "Plater not available"}}};

    Model& model = plater->model();
    if (model.objects.empty())
        return {{"success", false}, {{"message", "No model loaded to simplify"}}};

    // ---- Resolve object index ----
    int obj_idx = -1;
    if (params.contains("object_index")) {
        if (params["object_index"].is_number_integer())
            obj_idx = params["object_index"].get<int>();
        else if (params["object_index"].is_number())
            obj_idx = (int)std::llround(params["object_index"].get<double>());
    }

    if (obj_idx < 0 && params.contains("object_name")) {
        std::string target_name = params["object_name"].get<std::string>();
        for (int i = 0; i < (int)model.objects.size(); ++i) {
            if (model.objects[i] && model.objects[i]->name == target_name) {
                obj_idx = i;
                break;
            }
        }
    }

    if (obj_idx < 0)
        obj_idx = plater->get_selected_object_idx();

    if (obj_idx < 0 && model.objects.size() == 1)
        obj_idx = 0;

    if (obj_idx < 0 || obj_idx >= (int)model.objects.size())
        return {{"success", false}, {{"message", "Target object not found"}}};

    ModelObject* object = model.objects[obj_idx];
    if (!object || object->volumes.empty())
        return {{"success", false}, {{"message", "Target object has no volumes"}}};

    ModelVolume* volume = object->volumes.front();
    if (!volume)
        return {{"success", false}, {{"message", "Target volume is invalid"}}};

    const indexed_triangle_set& its = volume->mesh().its;
    size_t current_triangles = its.indices.size();
    if (current_triangles == 0)
        return {{"success", false}, {{"message", "Target model has no triangles"}}};

    // ---- Read ratio parameter ----
    double ratio = 0.5;  // default: keep 60% of triangles
    if (params.contains("ratio")) {
        if (params["ratio"].is_number())
            ratio = params["ratio"].get<double>();
    }

    // Clamp ratio to valid range
    if (ratio < 0.05) ratio = 0.05;
    if (ratio > 0.95) ratio = 0.95;

    uint32_t target_triangle_count = static_cast<uint32_t>(current_triangles * ratio);
    if (target_triangle_count < 100)
        target_triangle_count = 100;  // minimum triangles to keep

    BOOST_LOG_TRIVIAL(info) << "[SlicerBridge] DoSimplifyModel: object=" << object->name
                            << " current_triangles=" << current_triangles
                            << " ratio=" << ratio
                            << " target_triangles=" << target_triangle_count;

    // Take a snapshot for undo
    plater->take_snapshot(GUI::format("Simplify %1%", volume->name));
    plater->clear_before_change_mesh(obj_idx, true);

    // Perform quadric edge collapse simplification
    auto its_copy = std::make_unique<indexed_triangle_set>(its);
    float max_error = std::numeric_limits<float>::max();
    its_quadric_edge_collapse(*its_copy, target_triangle_count, &max_error);

    size_t result_triangles = its_copy->indices.size();

    // Update the mesh
    volume->set_mesh(std::move(*its_copy));
    volume->calculate_convex_hull();
    volume->invalidate_convex_hull_2d();
    volume->set_new_unique_id();
    volume->get_object()->invalidate_bounding_box();
    volume->get_object()->ensure_on_bed();

    // Fix hollowing, sla support points, modifiers, etc.
    plater->changed_mesh(obj_idx);
    // Fix warning icon in object list
    wxGetApp().obj_list()->update_item_error_icon(obj_idx, -1);

    BOOST_LOG_TRIVIAL(info) << "[SlicerBridge] DoSimplifyModel: simplified "
                            << current_triangles << " -> " << result_triangles << " triangles";

    return {
        {"success", true},
        {"message", "Model simplified from " + std::to_string(current_triangles) + " to " + std::to_string(result_triangles) + " triangles (ratio=" + std::to_string(ratio) + ")"}
    };
}

json SlicerBridge::DoSplitModel(const json& params)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    Model& model = plater->model();
    if (model.objects.empty())
        return {{"success", false}, {"message", "No model in scene"}};

    // ---- Resolve object index (same pattern as DoCloneModel) ----
    int obj_idx = -1;
    if (params.contains("object_index")) {
        if (params["object_index"].is_number_integer())
            obj_idx = params["object_index"].get<int>();
        else if (params["object_index"].is_number())
            obj_idx = (int)std::llround(params["object_index"].get<double>());
    }

    if (obj_idx < 0 && params.contains("object_name")) {
        std::string target_name = params["object_name"].get<std::string>();
        for (int i = 0; i < (int)model.objects.size(); ++i) {
            if (model.objects[i] && model.objects[i]->name == target_name) {
                obj_idx = i;
                break;
            }
        }
    }

    if (obj_idx < 0)
        obj_idx = plater->get_selected_object_idx();

    if (obj_idx < 0 && model.objects.size() == 1)
        obj_idx = 0;

    if (obj_idx < 0 || obj_idx >= (int)model.objects.size())
        return {{"success", false}, {"message", "Target object not found"}};

    ModelObject* object = model.objects[obj_idx];
    if (!object || object->instances.empty())
        return {{"success", false}, {"message", "Target object is invalid or has no instances"}};

    // ---- Determine cut Z ----
    const int instance_idx = 0;
    const BoundingBoxf3 bbox = object->instance_bounding_box(*object->instances[instance_idx], false);
    double min_z = bbox.min.z();
    double max_z = bbox.max.z();

    if (max_z - min_z < 0.2)
        return {{"success", false}, {"message", "Model is too thin to split (height: " + std::to_string(max_z - min_z) + " mm)"}};

    double cut_z = (min_z + max_z) / 2.0;  // default: middle
    if (params.contains("z")) {
        if (params["z"].is_number())
            cut_z = params["z"].get<double>();
        else if (params["z"].is_string()) {
            try { cut_z = std::stod(params["z"].get<std::string>()); }
            catch (...) {}
        }
        // Clamp to valid range
        if (cut_z < min_z + 0.1) cut_z = min_z + 0.1;
        if (cut_z > max_z - 0.1) cut_z = max_z - 0.1;
    }

    // ---- Perform cut ----
    const Vec3d instance_offset = object->instances[instance_idx]->get_offset();
    ModelObjectCutAttributes attributes = ModelObjectCutAttribute::KeepUpper |
                                           ModelObjectCutAttribute::KeepLower;
                                           // Match native GUI cut defaults: separate objects, upper drops to bed

    const std::string obj_name = object->name.empty() ? std::to_string(obj_idx) : object->name;

    Cut cut(object, instance_idx, Geometry::translation_transform(cut_z * Vec3d::UnitZ() - instance_offset), attributes);
    const ModelObjectPtrs& new_objects = cut.perform_with_plane();

    if (new_objects.empty())
        return {{"success", false}, {"message", "Split produced no objects"}};

    // Apply the cut result to the model (undo support)
    {
        Plater::TakeSnapshot snapshot(plater, "Split Model");
        plater->apply_cut_object_to_model((size_t)obj_idx, new_objects);
    }

    return {
        {"success", true},
        {"message", "Model '" + obj_name + "' split into " + std::to_string((int)new_objects.size()) + " parts at Z=" + std::to_string((int)(cut_z * 10) / 10.0) + " mm"}
    };
}

json SlicerBridge::DoAddTestModel(const json& params)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    std::string type_name = params.value("type_name", "");
    if (type_name.empty())
        type_name = params.value("name", "");
    if (type_name.empty()) {
        return {
            {"success", false},
            {"message", "Missing model type name. Supported: Cube, Sphere, Cylinder, Cone, Truncated Cone, Torus, Pyramid, Prism, Disc, Block20XY, 3DBenchy, Complex, Overhang, Square columns Z axis, Square prism Z axis"}
        };
    }

    // Map of file-based test model names to their filenames
    static const std::unordered_map<std::string, std::string> file_based_models = {
        {"Block20XY", "Block20XY.stl"},
        {"3DBenchy", "3DBenchy.stl"},
        {"Complex", "ksr_fdmtest_v4.stl"},
        {"Overhang", "Overhang.stl"},
        {"Square columns Z axis", "Square columns Z axis.stl"},
        {"Square prism Z axis", "Square prism Z axis.stl"}
    };

    // Set of procedural model names
    static const std::unordered_set<std::string> procedural_models = {
        "Cube", "Sphere", "Cylinder", "Cone", "Truncated Cone", "Torus", "Pyramid", "Prism", "Disc"
    };

    auto it_file = file_based_models.find(type_name);
    if (it_file != file_based_models.end()) {
        // File-based test model: load from resources
        auto file_path = boost::filesystem::path(Slic3r::resources_dir()) / "creality_models" / it_file->second;
        if (!boost::filesystem::exists(file_path) || !boost::filesystem::is_regular_file(file_path)) {
            return {
                {"success", false},
                {"message", "Test model file not found: " + it_file->second},
                {"path", file_path.string()}
            };
        }

        wxArrayString paths;
        paths.Add(from_u8(file_path.string()));
        plater->load_files(paths);
        plater->update();
        return {{"success", true}, {"message", "Test model '" + type_name + "' added"}, {"type_name", type_name}};
    }

    if (procedural_models.find(type_name) == procedural_models.end()) {
        return {
            {"success", false},
            {"message", "Unknown test model type: " + type_name + ". Supported: Cube, Sphere, Cylinder, Cone, Truncated Cone, Torus, Pyramid, Prism, Disc, Block20XY, 3DBenchy, Complex, Overhang, Square columns Z axis, Square prism Z axis"}
        };
    }

    // Procedural model: generate mesh and load
    double side = 20.0;
    if (auto* canvas = plater->canvas3D()) {
        side = canvas->get_size_proportional_to_max_bed_size(0.1);
        if (side > 100.0) side = 20.0;
    }

    TriangleMesh mesh;
    if (type_name == "Cube")
        mesh = TriangleMesh(its_make_cube(side, side, side));
    else if (type_name == "Cylinder")
        mesh = TriangleMesh(its_make_cylinder(0.5 * side, side));
    else if (type_name == "Sphere")
        mesh = TriangleMesh(its_make_sphere(0.5 * side, PI / 90));
    else if (type_name == "Cone")
        mesh = TriangleMesh(its_make_cone(0.5 * side, side));
    else if (type_name == "Disc")
        mesh = TriangleMesh(its_make_cylinder(5.0f, 0.3f));
    else if (type_name == "Torus")
        mesh.ReadSTLFile((Slic3r::resources_dir() + "/creality_models/torus.stl").c_str(), true, nullptr);
    else if (type_name == "Prism")
        mesh = TriangleMesh(its_make_cylinder(20.0f, 30.0f, 2.0 * PI / 3.0));
    else if (type_name == "Truncated Cone")
        mesh = TriangleMesh(its_make_frustum(20.0f, 30.0f));
    else if (type_name == "Pyramid")
        mesh = TriangleMesh(its_make_cone(20.0f, 30.0f, 0.50 * PI));

    if (mesh.facets_count() == 0) {
        return {{"success", false}, {"message", "Failed to create mesh for: " + type_name}};
    }

    // Add mesh to model (same pattern as load_shape_object / load_mesh_object)
    {
        Plater::TakeSnapshot snapshot(plater, "Add Test Model");

        Model& model = plater->model();
        ModelObject* new_object = model.add_object();
        new_object->name = type_name;
        new_object->add_instance();

        ModelVolume* new_volume = new_object->add_volume(mesh);
        new_object->sort_volumes(true);
        new_volume->name = type_name;
        new_object->config.set_key_value("extruder", new ConfigOptionInt(1));
        new_object->invalidate_bounding_box();

        auto bb = mesh.bounding_box();
        new_object->translate(-bb.center());

        Slic3r::save_object_mesh(*new_object);
        plater->arrange_loaded_object_to_new_position(new_object->instances[0]);
        new_object->ensure_on_bed();

        Geometry::Transformation t = new_object->instances[0]->get_transformation();
        new_object->instances[0]->set_assemble_transformation(t);
    }

    plater->update();
    {
        const size_t obj_idx = plater->model().objects.size() - 1;
        wxGetApp().obj_list()->paste_objects_into_list({obj_idx});
    }

    return {{"success", true}, {"message", "Test model '" + type_name + "' added"}, {"type_name", type_name}};
}

} // namespace Bridge
} // namespace GUI
} // namespace Slic3r



