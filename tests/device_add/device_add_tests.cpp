#include "libslic3r/DeviceAddUtils.hpp"
#include <iostream>
#include <stdexcept>

using nlohmann::json;
using Slic3r::DeviceAddUtils::resolve_printer_selections;

static void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

int main()
{
    try {
        const std::string all = "0.2;0.4;0.6;0.8";
        const json profiles = json::array({
            {{"model", "Creality K2 Plus"}, {"vendor", "Other"}, {"nozzle_diameter", "0.4"}},
            {{"model", "Creality K2 Plus"}, {"vendor", "Creality"}, {"nozzle_diameter", all}},
            {{"model", "Creality K1"}, {"vendor", "Creality"}, {"nozzle_diameter", all}},
            {{"model", "Creality K1 Max"}, {"vendor", "Creality"}, {"nozzle_diameter", all}},
            {{"model", "SPARKX i7"}, {"vendor", "Creality"}, {"nozzle_diameter", all}}
        });
        const json bundled = {{"printerList", json::array({
            {{"printerIntName", "F008"}, {"name", "K2 Plus"}, {"nozzleDiameter", {"0.8"}}},
            {{"printerIntName", "F008"}, {"name", "K2 Plus"}, {"nozzleDiameter", {"0.4"}}},
            {{"printerIntName", "CR-K1"}, {"name", "K1"}},
            {{"printerIntName", "CR-K1 Max"}, {"name", "K1 Max"}},
            {{"printerIntName", "FNEW"}, {"name", "Obsolete name"}}
        })}};
        auto resolve = [&](json selection, const json& system = json()) {
            json selections = json::array({selection});
            resolve_printer_selections(selections, profiles, system, bundled);
            return selections[0];
        };
        for (const auto& empty : {json::array(), json(""), json()}) {
            auto selected = resolve({{"model", "F008"}, {"vendor", ""}, {"nozzle_diameter", empty}});
            require(selected["model"] == "Creality K2 Plus", "scan model must resolve");
            require(selected["nozzle_diameter"] == all, "scan/bind must enable every supported diameter");
        }
        require(resolve({{"model", "F008"}})["nozzle_diameter"] == all, "missing diameters must enable all");
        require(resolve({{"model", "Creality K2 Plus"}, {"vendor", "Creality"}})["nozzle_diameter"] == all,
                "named model must select all from the matching vendor");
        for (const auto& explicit_value : {json("0.6;0.8"), json::array({"0.6", "0.8"})}) {
            require(resolve({{"model", "F008"}, {"nozzle_diameter", explicit_value}})["nozzle_diameter"] == "0.6;0.8",
                    "explicit scan selection must survive");
            require(resolve({{"model", "Creality K2 Plus"}, {"vendor", "Creality"}, {"nozzle_diameter", explicit_value}})["nozzle_diameter"] == "0.6;0.8",
                    "explicit manual selection must survive");
        }
        const json updated = {{"printerList", json::array({
            {{"printerIntName", "FNEW"}, {"name", "SPARKX i7"}, {"nozzleDiameter", {"0.4", "0.4"}}},
            {{"printerIntName", "ONLY_UPDATED"}, {"name", "K2 Plus"}}
        })}};
        auto selected = resolve({{"model", "FNEW"}}, updated);
        require(selected["model"] == "SPARKX i7", "updated catalogue must take precedence and preserve SPARKX name");
        require(selected["nozzle_diameter"] == all, "physical extruder count must not limit supported variants");
        require(resolve({{"model", "ONLY_UPDATED"}}, updated)["nozzle_diameter"] == all, "updated-only model must resolve");
        require(resolve({{"model", "F008"}}, updated)["nozzle_diameter"] == all, "missing updated entry must fall back to bundled catalogue");
        require(resolve({{"model", "K1"}})["nozzle_diameter"] == all, "K1 alias must resolve");
        require(resolve({{"model", "K1 Max"}})["nozzle_diameter"] == all, "K1 Max alias must resolve");
        require(!resolve({{"model", "UNKNOWN"}}).contains("nozzle_diameter"), "unknown model must not invent a diameter");
        json unmatched = json::array({{{"model", "UNKNOWN"}, {"vendor", "Creality"}}});
        resolve_printer_selections(unmatched, profiles, json(), json());
        require(!unmatched[0].contains("nozzle_diameter"), "missing catalogues must be safe");
        std::cout << "Device-add nozzle regression checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
