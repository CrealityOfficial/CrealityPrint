#include "MaterialListManager.hpp"

#include "Preset.hpp"
#include "Utils.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>

#include <boost/log/trivial.hpp>
#include <boost/nowide/fstream.hpp>
#include <nlohmann/json.hpp>

namespace Slic3r {

static std::string trim_copy(std::string value)
{
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

MaterialListManager& MaterialListManager::instance()
{
    static MaterialListManager manager;
    return manager;
}

boost::filesystem::path MaterialListManager::material_list_path() const
{
    return (boost::filesystem::path(Slic3r::data_dir()) / "system" / "Creality" / "materialList.json").make_preferred();
}

std::string MaterialListManager::base_name_from_preset_name(std::string material_name) const
{
    material_name = Preset::remove_suffix_modified(std::move(material_name));
    const std::string::size_type printer_suffix_pos = material_name.find(" @");
    const std::string::size_type at_pos = printer_suffix_pos != std::string::npos ? printer_suffix_pos : material_name.find('@');
    if (at_pos != std::string::npos)
        material_name.erase(at_pos);
    return trim_copy(std::move(material_name));
}

void MaterialListManager::refresh_alias_cache_locked()
{
    const boost::filesystem::path path = material_list_path();

    if (m_alias_cache_initialized && m_alias_cache_path == path)
        return;

    m_alias_cache_initialized = true;

    std::time_t write_time = 0;
    if (boost::filesystem::exists(path))
        write_time = boost::filesystem::last_write_time(path);

    if (m_alias_cache_path == path && m_alias_cache_last_write_time == write_time)
        return;

    m_alias_cache_path = path;
    m_alias_cache_last_write_time = write_time;
    m_aliases.clear();

    if (write_time == 0)
        return;

    try {
        std::ifstream stream(path.string());
        nlohmann::json root = nlohmann::json::parse(stream, nullptr, true);
        const auto& materials = root.contains("materials") ? root["materials"] : root;
        if (!materials.is_array())
            return;

        for (const auto& material : materials) {
            if (!material.is_object())
                continue;
            const std::string name = material.value("name", std::string());
            const std::string alias = trim_copy(material.value("alias", std::string()));
            if (!name.empty() && !alias.empty())
                m_aliases[name] = alias;
        }
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ": failed to load material aliases from " << path.string()
                                   << ": " << e.what();
        m_aliases.clear();
    }
}

std::string MaterialListManager::display_name_from_preset_name(const std::string& preset_name)
{
    std::lock_guard<std::mutex> lock(m_alias_cache_mutex);
    refresh_alias_cache_locked();

    const std::string key = base_name_from_preset_name(preset_name);
    auto it = m_aliases.find(key);
    return it != m_aliases.end() && !it->second.empty() ? it->second : std::string();
}

std::string MaterialListManager::display_name_with_material_alias(const std::string& preset_name, bool keep_printer_suffix)
{
    std::string display_name = display_name_from_preset_name(preset_name);
    if (display_name.empty())
        return preset_name;

    const std::string clean_preset_name = Preset::remove_suffix_modified(preset_name);
    if (keep_printer_suffix && display_name != clean_preset_name) {
        const std::string::size_type printer_suffix_pos = clean_preset_name.find(" @");
        const std::string::size_type at_pos = printer_suffix_pos != std::string::npos ? printer_suffix_pos : clean_preset_name.find('@');
        if (at_pos != std::string::npos)
            display_name += clean_preset_name.substr(at_pos);
    }

    return clean_preset_name != preset_name && preset_name.rfind(Preset::suffix_modified(), 0) == 0 ?
        Preset::suffix_modified() + display_name :
        display_name;
}

std::string MaterialListManager::display_name_with_material_alias(const Preset& preset, bool keep_printer_suffix)
{
    const bool is_user_base_filament = preset.is_user() && preset.inherits().empty() &&
        (preset.file.find("filament/base") != std::string::npos || preset.file.find("filament\\base") != std::string::npos);
    if (is_user_base_filament) {
        std::string display_name = Preset::remove_suffix_modified(preset.label(true));
        const std::string::size_type printer_suffix_pos = display_name.find(" @");
        const std::string::size_type at_pos = printer_suffix_pos != std::string::npos ? printer_suffix_pos : display_name.find('@');
        if (at_pos != std::string::npos)
            display_name = display_name.substr(0, at_pos);
        return preset.is_dirty ? Preset::suffix_modified() + display_name : display_name;
    }

    if (preset.is_user() || !preset.is_system)
        return preset.label(true);

    std::string display_name = display_name_from_preset_name(preset.name);
    if (display_name.empty())
        display_name = preset.alias.empty() ? preset.name : preset.alias;

    const std::string preset_name = Preset::remove_suffix_modified(preset.name);
    if (keep_printer_suffix && display_name != preset_name) {
        const std::string::size_type printer_suffix_pos = preset_name.find(" @");
        const std::string::size_type at_pos = printer_suffix_pos != std::string::npos ? printer_suffix_pos : preset_name.find('@');
        if (at_pos != std::string::npos)
            display_name += preset_name.substr(at_pos);
    }

    return (preset.is_dirty ? Preset::suffix_modified() : std::string()) + display_name;
}

bool MaterialListManager::material_alias_matches_preset_name(const std::string& preset_name, const std::string& alias)
{
    const std::string display_name = display_name_from_preset_name(preset_name);
    return !display_name.empty() && display_name == base_name_from_preset_name(alias);
}

bool MaterialListManager::save_official_material_list(const nlohmann::json& materials)
{
    if (!materials.is_array() || materials.empty())
        return false;

    try {
        nlohmann::json out;
        out["materials"] = materials;

        const boost::filesystem::path path = material_list_path();
        boost::filesystem::create_directories(path.parent_path());

        boost::nowide::ofstream stream;
        stream.open(path.string(), std::ios::out | std::ios::trunc);
        if (!stream.is_open())
            return false;
        stream << std::setw(4) << out << std::endl;
        {
            std::lock_guard<std::mutex> lock(m_alias_cache_mutex);
            m_alias_cache_last_write_time = 0;
            m_alias_cache_initialized = false;
            m_aliases.clear();
        }
        return true;
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ": failed to save material list: " << e.what();
    } catch (...) {
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << ": failed to save material list";
    }
    return false;
}

} // namespace Slic3r
