#ifndef slic3r_DeviceAddUtils_hpp_
#define slic3r_DeviceAddUtils_hpp_

#include <string>
#include <nlohmann/json.hpp>

namespace Slic3r { namespace DeviceAddUtils {

inline std::string normalize_nozzles(const nlohmann::json& value)
{
    if (value.is_string())
        return value.get<std::string>();
    if (!value.is_array())
        return {};
    std::string result;
    for (const auto& nozzle : value) {
        if (!nozzle.is_string() || nozzle.get<std::string>().empty())
            continue;
        if (!result.empty())
            result += ";";
        result += nozzle.get<std::string>();
    }
    return result;
}

inline bool nozzle_list_contains(const std::string& list, const std::string& nozzle)
{
    size_t begin = 0;
    while (begin <= list.size()) {
        const size_t end = list.find(';', begin);
        if (list.substr(begin, end == std::string::npos ? end : end - begin) == nozzle)
            return true;
        if (end == std::string::npos)
            break;
        begin = end + 1;
    }
    return false;
}

// Saving the device-add selection rebuilds the enabled vendor map. Keep the
// active system printer selectable even when its parameter package appeared
// after the device-add page took its profile snapshot.
inline bool ensure_active_nozzle_selected(nlohmann::json& profile_models, const std::string& vendor,
                                          const std::string& model, const std::string& nozzle)
{
    if (!profile_models.is_array() || vendor.empty() || model.empty() || nozzle.empty())
        return false;
    for (auto& profile : profile_models) {
        if (!profile.is_object() || profile.value("vendor", std::string()) != vendor ||
            profile.value("model", std::string()) != model)
            continue;

        const std::string supported = normalize_nozzles(profile.value("nozzle_diameter", nlohmann::json()));
        if (!nozzle_list_contains(supported, nozzle))
            return false;
        std::string selected = normalize_nozzles(profile.value("nozzle_selected", nlohmann::json()));
        if (nozzle_list_contains(selected, nozzle))
            return false;
        if (!selected.empty())
            selected += ';';
        profile["nozzle_selected"] = selected + nozzle;
        return true;
    }
    return false;
}

// Device-add computes the installed filament set from the selected printers.
// Merge the active system preset into that selection before the computation so
// a newly downloaded nozzle variant keeps both its printer and its materials.
inline bool ensure_printer_nozzle_selected(nlohmann::json& selections, const nlohmann::json& profile_models,
                                           const std::string& vendor, const std::string& model,
                                           const std::string& nozzle)
{
    if ((!selections.is_array() && !selections.is_object()) || !profile_models.is_array() ||
        vendor.empty() || model.empty() || nozzle.empty())
        return false;

    bool supported = false;
    for (const auto& profile : profile_models) {
        if (profile.is_object() && profile.value("vendor", std::string()) == vendor &&
            profile.value("model", std::string()) == model &&
            nozzle_list_contains(normalize_nozzles(profile.value("nozzle_diameter", nlohmann::json())), nozzle)) {
            supported = true;
            break;
        }
    }
    if (!supported)
        return false;

    for (auto& selected : selections) {
        if (!selected.is_object() || selected.value("vendor", std::string()) != vendor ||
            selected.value("model", std::string()) != model)
            continue;
        std::string nozzles = normalize_nozzles(selected.value("nozzle_diameter", nlohmann::json()));
        if (nozzle_list_contains(nozzles, nozzle))
            return false;
        if (!nozzles.empty())
            nozzles += ';';
        selected["nozzle_diameter"] = nozzles + nozzle;
        return true;
    }

    nlohmann::json selected = {{"vendor", vendor}, {"model", model}, {"nozzle_diameter", nozzle}};
    if (selections.is_array())
        selections.push_back(std::move(selected));
    else
        selections["active:" + vendor + ":" + model] = std::move(selected);
    return true;
}

inline std::string catalog_model_name(const nlohmann::json& catalog, const std::string& internal_name)
{
    const auto printers = catalog.find("printerList");
    if (printers == catalog.end() || !printers->is_array())
        return {};
    for (const auto& printer : *printers) {
        if (!printer.is_object() || printer.value("printerIntName", std::string()) != internal_name)
            continue;
        std::string name = printer.value("name", std::string());
        if (name.empty())
            continue;
        if (name.find("Creality") == std::string::npos && name.find("SPARKX") == std::string::npos)
            name = "Creality " + name;
        return name;
    }
    return {};
}

// Empty nozzle selections from scan/bind flows mean all supported variants.
// The machine model defines selectable diameters; a package's nozzleDiameter
// describes installed physical nozzles and must not narrow this selection.
inline void resolve_printer_selections(nlohmann::json& selections, const nlohmann::json& profile_models,
                                       const nlohmann::json& system_catalog, const nlohmann::json& bundled_catalog)
{
    if (!profile_models.is_array())
        return;
    for (auto& selected : selections) {
        if (!selected.is_object() || !selected.contains("model") || !selected["model"].is_string())
            continue;

        std::string model = selected["model"].get<std::string>();
        std::string vendor = selected.value("vendor", std::string());
        if (vendor.empty()) {
            if (model == "K1 Max") model = "CR-K1 Max";
            if (model == "K1") model = "CR-K1";
            std::string name = catalog_model_name(system_catalog, model);
            if (name.empty())
                name = catalog_model_name(bundled_catalog, model);
            if (name.empty())
                continue;
            model = name;
            vendor = "Creality";
            selected["model"] = model;
            selected["vendor"] = vendor;
        }

        for (const auto& profile : profile_models) {
            if (!profile.is_object() || profile.value("model", std::string()) != model ||
                profile.value("vendor", std::string()) != vendor)
                continue;
            const auto nozzles = selected.find("nozzle_diameter");
            const std::string explicit_nozzles = nozzles == selected.end() ? std::string() : normalize_nozzles(*nozzles);
            selected["nozzle_diameter"] = explicit_nozzles.empty()
                ? normalize_nozzles(profile.value("nozzle_diameter", nlohmann::json())) : explicit_nozzles;
            break;
        }
    }
}

}} // namespace Slic3r::DeviceAddUtils
#endif
