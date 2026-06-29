#pragma once

#include <string>
#include <vector>

namespace ColorMatch {

struct DeviceBoxColorInfo {
    // From device.boxsInfo.boxColorInfo
    int         boxType = 0;           // 0 = CFS, 1 = External, 2 = CFS-mini/external (matches JS filter)
    std::string color;                 // "#RRGGBB"
    std::string filamentType;          // e.g. "PLA", "PETG"
    int         materialId = 0;
    int         boxId = -1;            // CFS box id
    int         cId = -1;              // cloud id (optional)
    int         RFIDState = 1;         // 1: non-RFID, 2: RFID
    int         percent = 100;         // remaining percent
    double      remaining_length = 0;  // remaining length
};

struct Device {
    struct BoxsInfo {
        std::vector<DeviceBoxColorInfo> boxColorInfo;
    } boxsInfo;
};

struct ModelColor {
    int         extruderId = 0;
    std::string extruderColor;   // "#RRGGBB"
    std::string filamentType;    // desired type
    double      filamentLength = 0; // optional, passed through
};

struct MatchResult {
    int         extruderId = 0;
    std::string extruderColor;   // input model color
    double      filamentLength = 0;

    std::string matchColor = "#ffffff";
    std::string matchFilamentType; // empty if type not matched
    int         materialId = 0;
    int         boxId = -1;
    int         cId = -1;
    int         matchStatusCode = 1; // 0 matched, 1 not matched
    std::string msg = "No matching color found";
    int         RFIDState = 1;
    int         percent = 100;
    double      remaining_length = 0;
    double      distance = 0.0; // deltaE2000 when matched
};

// Compute perceptual color distance using the same Lab + DeltaE2000 pipeline
// as the legacy send-time color matching path.
double calculatePerceptualColorDistance(const std::string& hex1, const std::string& hex2);

// Compute matching results replicating matchColor.js behavior
std::vector<MatchResult> getColorMatchInfo(const Device& device, const std::vector<ModelColor>& modelColors);

} // namespace ColorMatch

