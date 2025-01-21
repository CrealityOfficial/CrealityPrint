#include "Filament.hpp"

#include "libslic3r/Utils.hpp"
#include <boost/filesystem/path.hpp>
#include <boost/format.hpp>
#include <boost/log/trivial.hpp>
#include <boost/regex.hpp>
#include <boost/nowide/fstream.hpp>
#include <nlohmann/json.hpp>

using namespace nlohmann;
namespace fs = boost::filesystem;

namespace creality
{
    Slic3r::FilamentTempType Filament::get_filament_temp_type(const std::string& filament_type)
    {
        const static std::string HighTempFilamentStr = "high_temp_filament";
        const static std::string LowTempFilamentStr = "low_temp_filament";
        const static std::string HighLowCompatibleFilamentStr = "high_low_compatible_filament";
        static std::unordered_map<std::string, std::unordered_set<std::string>> filament_temp_type_map;

        if (filament_temp_type_map.empty()) {
            fs::path file_path = fs::path(Slic3r::resources_dir()) / "info" / "filament_info.json";
            std::ifstream in(file_path.string());
            json j;
            try{
                j = json::parse(in);
                in.close();
                auto&&high_temp_filament_arr =j[HighTempFilamentStr].get < std::vector<std::string>>();
                filament_temp_type_map[HighTempFilamentStr] = std::unordered_set<std::string>(high_temp_filament_arr.begin(), high_temp_filament_arr.end());
                auto&& low_temp_filament_arr = j[LowTempFilamentStr].get < std::vector<std::string>>();
                filament_temp_type_map[LowTempFilamentStr] = std::unordered_set<std::string>(low_temp_filament_arr.begin(), low_temp_filament_arr.end());
                auto&& high_low_compatible_filament_arr = j[HighLowCompatibleFilamentStr].get < std::vector<std::string>>();
                filament_temp_type_map[HighLowCompatibleFilamentStr] = std::unordered_set<std::string>(high_low_compatible_filament_arr.begin(), high_low_compatible_filament_arr.end());
            }
            catch (const json::parse_error& err){
                in.close();
                BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << file_path.string() << " got a nlohmann::detail::parse_error, reason = " << err.what();
                filament_temp_type_map[HighTempFilamentStr] = {"ABS","ASA","PC","PA","PA-CF","PA-GF","PA6-CF","PET-CF","PPS","PPS-CF","PPA-GF","PPA-CF","ABS-Aero","ABS-GF"};
                filament_temp_type_map[LowTempFilamentStr] = {"PLA","TPU","PLA-CF","PLA-AERO","PVA","BVOH"};
                filament_temp_type_map[HighLowCompatibleFilamentStr] = { "HIPS","PETG","PCTG","PE","PP","EVA","PE-CF","PP-CF","PP-GF","PHA"};
            }
        }

        if (filament_temp_type_map[HighLowCompatibleFilamentStr].find(filament_type) != filament_temp_type_map[HighLowCompatibleFilamentStr].end())
            return Slic3r::HighLowCompatible;
        if (filament_temp_type_map[HighTempFilamentStr].find(filament_type) != filament_temp_type_map[HighTempFilamentStr].end())
            return Slic3r::HighTemp;
        if (filament_temp_type_map[LowTempFilamentStr].find(filament_type) != filament_temp_type_map[LowTempFilamentStr].end())
            return Slic3r::LowTemp;
        return Slic3r::Undefine;
    }

    int Filament::get_hrc_by_nozzle_type(const Slic3r::NozzleType&type)
    {
        static std::map<std::string, int>nozzle_type_to_hrc;
        if (nozzle_type_to_hrc.empty()) {
            fs::path file_path = fs::path(Slic3r::resources_dir()) / "info" / "nozzle_info.json";
            boost::nowide::ifstream in(file_path.string());
            //std::ifstream in(file_path.string());
            json j;
            try {
                j = json::parse(in);
                in.close();
                for (const auto& elem : j["nozzle_hrc"].items())
                    nozzle_type_to_hrc[elem.key()] = elem.value();
            }
            catch (const json::parse_error& err) {
                in.close();
                BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": parse " << file_path.string() << " got a nlohmann::detail::parse_error, reason = " << err.what();
                nozzle_type_to_hrc = {
                    {"hardened_steel",55},
                    {"stainless_steel",20},
                    {"brass",2},
                    {"undefine",0}
                };
            }
        }
        auto iter = nozzle_type_to_hrc.find(Slic3r::NozzleTypeEumnToStr[type]);
        if (iter != nozzle_type_to_hrc.end())
            return iter->second;
        //0 represents undefine
        return 0;
    }

    bool Filament::check_multi_filaments_compatibility(const std::vector<std::string>& filament_types)
    {
        bool has_high_temperature_filament = false;
        bool has_low_temperature_filament = false;

        for (const auto& type : filament_types) {
            if (get_filament_temp_type(type) == Slic3r::FilamentTempType::HighTemp)
                has_high_temperature_filament = true;
            else if (get_filament_temp_type(type) == Slic3r::FilamentTempType::LowTemp)
                has_low_temperature_filament = true;
        }

        if (has_high_temperature_filament && has_low_temperature_filament)
            return false;

        return true;
    }

    bool Filament::is_filaments_compatible(const std::vector<int>& filament_types)
    {
        bool has_high_temperature_filament = false;
        bool has_low_temperature_filament = false;

        for (const auto& type : filament_types) {
            if (type == Slic3r::FilamentTempType::HighTemp)
                has_high_temperature_filament = true;
            else if (type == Slic3r::FilamentTempType::LowTemp)
                has_low_temperature_filament = true;
        }

        if (has_high_temperature_filament && has_low_temperature_filament)
            return false;

        return true;
    }

    int Filament::get_compatible_filament_type(const std::set<int>& filament_types)
    {
        bool has_high_temperature_filament = false;
        bool has_low_temperature_filament = false;

        for (const auto& type : filament_types) {
            if (type == Slic3r::FilamentTempType::HighTemp)
                has_high_temperature_filament = true;
            else if (type == Slic3r::FilamentTempType::LowTemp)
                has_low_temperature_filament = true;
        }

        if (has_high_temperature_filament && has_low_temperature_filament)
            return Slic3r::HighLowCompatible;
        else if (has_high_temperature_filament)
            return Slic3r::HighTemp;
        else if (has_low_temperature_filament)
            return Slic3r::LowTemp;
        return Slic3r::HighLowCompatible;
    }
}