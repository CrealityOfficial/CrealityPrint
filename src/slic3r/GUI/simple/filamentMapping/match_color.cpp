#include "match_color.hpp"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <limits>

namespace ColorMatch {

static constexpr double kPI = 3.1415926535897932384626433832795;

// --- Color space helpers (sRGB D65 -> XYZ -> CIE Lab) ---

static inline double linear(double v)
{
    return (v > 0.04045) ? std::pow((v + 0.055) / 1.055, 2.4) : (v / 12.92);
}

static inline double f(double t)
{
    return (t > 0.008856) ? std::pow(t, 1.0 / 3.0) : (7.787 * t + 16.0 / 116.0);
}

static inline void rgbToXyz(double r, double g, double b, double &x, double &y, double &z)
{
    const double rLin = linear(r);
    const double gLin = linear(g);
    const double bLin = linear(b);

    // sRGB D65
    const double refX = 0.95047;
    const double refY = 1.0;
    const double refZ = 1.08883;

    x = rLin * 0.412453 + gLin * 0.35758  + bLin * 0.180423;
    y = rLin * 0.212671 + gLin * 0.71516  + bLin * 0.072169;
    z = rLin * 0.019334 + gLin * 0.119193 + bLin * 0.950227;

    x /= refX; y /= refY; z /= refZ;
}

static inline void xyzToLab(double x, double y, double z, double &L, double &a, double &b)
{
    const double fx = f(x);
    const double fy = f(y);
    const double fz = f(z);
    L = 116.0 * fy - 16.0;
    a = 500.0 * (fx - fy);
    b = 200.0 * (fy - fz);
}

static inline double deltaE2000(double L1, double a1, double b1,
                                double L2, double a2, double b2,
                                double KL = 1.0, double KC = 1.0, double KH = 1.0)
{
    const double aLxMean = (L1 + L2) * 0.5;
    const double aLxMean50_2 = std::pow(aLxMean - 50.0, 2.0);
    const double aDeltaLx = L2 - L1;
    const double aS_L = 1.0 + (0.015 * aLxMean50_2) / std::sqrt(20.0 + aLxMean50_2);
    const double aDL = aDeltaLx / (aS_L * KL);

    const double aC1 = std::sqrt(a1 * a1 + b1 * b1);
    const double aC2 = std::sqrt(a2 * a2 + b2 * b2);
    const double aCMean = 0.5 * (aC1 + aC2);

    const double aCMeanPow7 = std::pow(aCMean, 7.0);
    const double a25Pow7 = std::pow(25.0, 7.0);
    const double aG = 0.5 * (1.0 - std::sqrt(aCMeanPow7 / (aCMeanPow7 + a25Pow7)));

    const double a2x = a2 * (1.0 + aG);
    const double a1x = a1 * (1.0 + aG);
    const double aC2x = std::sqrt(a2x * a2x + b2 * b2);
    const double aC1x = std::sqrt(a1x * a1x + b1 * b1);

    const double aCxMean = 0.5 * (aC2x + aC1x);
    const double aS_C = 1.0 + 0.045 * aCxMean;
    const double aDeltaCx = aC2x - aC1x;
    const double aDC = aDeltaCx / (aS_C * KC);

    const double Eps = 1e-4;
    double ah1x = aC1x > Eps ? std::atan2(b1, a1x) * (180.0 / kPI) : 270.0;
    double ah2x = aC2x > Eps ? std::atan2(b2, a2x) * (180.0 / kPI) : 270.0;
    if (ah1x < 0.0) ah1x += 360.0;
    if (ah2x < 0.0) ah2x += 360.0;

    double aHxMean = 0.5 * (ah1x + ah2x);
    double aDeltahx = ah2x - ah1x;
    if (std::abs(aDeltahx) > 180.0) {
        aHxMean += aHxMean < 180.0 ? 180.0 : -180.0;
        aDeltahx += ah1x >= ah2x ? 360.0 : -360.0;
    }

    const double aDeltaHx = 2.0 * std::sqrt(aC1x * aC2x) * std::sin((0.5 * aDeltahx * kPI) / 180.0);
    const double aT = 1.0
        - 0.17 * std::cos(((aHxMean - 30.0) * kPI) / 180.0)
        + 0.24 * std::cos((2.0 * aHxMean * kPI) / 180.0)
        + 0.32 * std::cos(((3.0 * aHxMean + 6.0) * kPI) / 180.0)
        - 0.20 * std::cos(((4.0 * aHxMean - 63.0) * kPI) / 180.0);
    const double aS_H = 1.0 + 0.015 * aCxMean * aT;
    const double aDH = aDeltaHx / (aS_H * KH);

    const double aDeltaTheta = 30.0 * std::exp(-std::pow((aHxMean - 275.0) / 25.0, 2.0));
    const double aCxMeanPow7b = std::pow(aCxMean, 7.0);
    const double aR_C = 2.0 * std::sqrt(aCxMeanPow7b / (aCxMeanPow7b + a25Pow7));
    const double aR_T = -aR_C * std::sin((2.0 * aDeltaTheta * kPI) / 180.0);

    return std::sqrt(aDL * aDL + aDC * aDC + aDH * aDH + aR_T * aDC * aDH);
}

static inline int hex2(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    return 0;
}

static inline void parseHexColor(const std::string& hex, int &r, int &g, int &b)
{
    // Expect "#RRGGBB"; be tolerant of missing '#'
    size_t i = 0;
    if (!hex.empty() && hex[0] == '#') i = 1;
    if (hex.size() - i < 6) { r = g = b = 0; return; }
    r = (hex2(hex[i]) << 4) | hex2(hex[i+1]);
    g = (hex2(hex[i+2]) << 4) | hex2(hex[i+3]);
    b = (hex2(hex[i+4]) << 4) | hex2(hex[i+5]);
}

static inline double calculate_color_distance_impl(const std::string& hex1, const std::string& hex2)
{
    int r1, g1, b1; parseHexColor(hex1, r1, g1, b1);
    int r2, g2, b2; parseHexColor(hex2, r2, g2, b2);

    double x1, y1, z1; rgbToXyz(r1/255.0, g1/255.0, b1/255.0, x1, y1, z1);
    double x2, y2, z2; rgbToXyz(r2/255.0, g2/255.0, b2/255.0, x2, y2, z2);
    double L1, a1, b_1; xyzToLab(x1, y1, z1, L1, a1, b_1);
    double L2, a2, b_2; xyzToLab(x2, y2, z2, L2, a2, b_2);

    // JS weights: deltaE2000(lab1, lab2, 0.8, 0.6, 0.8)
    return deltaE2000(L1, a1, b_1, L2, a2, b_2, 0.8, 0.6, 0.8);
}

double calculatePerceptualColorDistance(const std::string& hex1, const std::string& hex2)
{
    return calculate_color_distance_impl(hex1, hex2);
}

static std::string to_lower_copy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

static std::string extract_material_family(const std::string& value)
{
    const std::string normalized = to_lower_copy(value);
    static const char* kFamilies[] = {
        "pla", "petg", "abs", "asa", "tpu", "pa", "nylon", "pc", "hips", "pva", "support"
    };

    for (const char* family : kFamilies) {
        if (normalized.find(family) != std::string::npos)
            return family;
    }

    return normalized;
}

static bool material_type_matches(const std::string& lhs, const std::string& rhs)
{
    if (lhs.empty() || rhs.empty())
        return true;
    if (lhs == rhs)
        return true;

    const std::string left = to_lower_copy(lhs);
    const std::string right = to_lower_copy(rhs);
    if (left == right)
        return true;
    if (left.find(right) != std::string::npos || right.find(left) != std::string::npos)
        return true;

    return extract_material_family(left) == extract_material_family(right);
}
// --- Matching logic ---

struct MatchColorRet {
    int         index = -1;        // index within deviceColors
    std::string color = "#ffffff";
    int         materialId = 0;
    int         boxId = -1;
    std::string filamentType;      // empty if not matched
    int         cId = -1;
    int         matchStatusCode = 1; // 0 matched
    std::string msg = "Filament type not matched";
    double      distance = std::numeric_limits<double>::infinity();
    int         RFIDState = 1;
    int         percent = 100;
    double      remaining_length = 0.0;
};

static MatchColorRet getMatchColor(const std::string& color,
                                   const std::string& filamentType,
                                   const std::vector<DeviceBoxColorInfo>& deviceBoxColors)
{
    MatchColorRet ret; // defaults to not matched
    for (size_t i = 0; i < deviceBoxColors.size(); ++i) {
        const auto& box = deviceBoxColors[i];
        if (material_type_matches(box.filamentType, filamentType)) {
            const double d = calculatePerceptualColorDistance(color, box.color);
            if (d < ret.distance) {
                ret.index = static_cast<int>(i);
                ret.color = box.color;
                ret.materialId = box.materialId;
                ret.boxId = box.boxId;
                ret.filamentType = box.filamentType;
                ret.cId = box.cId;
                ret.matchStatusCode = 0;
                ret.msg = "ok";
                ret.distance = d;
                ret.RFIDState = box.RFIDState;
                ret.percent = box.percent;
                ret.remaining_length = box.remaining_length;
            }
        }
    }
    return ret;
}

std::vector<MatchResult> getColorMatchInfo(const Device& device, const std::vector<ModelColor>& modelColors)
{
    // Copy model colors (JS does deep copy and sorts by filamentType desc)
    std::vector<ModelColor> plateColors = modelColors;
    std::sort(plateColors.begin(), plateColors.end(), [](const ModelColor& a, const ModelColor& b){
        return a.filamentType > b.filamentType; // descending, matches JS comparator
    });

    // Filter device colors (CFS=0, External=1, CFS-mini/external=2)
    std::vector<DeviceBoxColorInfo> deviceColors;
    deviceColors.reserve(device.boxsInfo.boxColorInfo.size());
    for (const auto& c : device.boxsInfo.boxColorInfo) {
        if (c.boxType == 0 || c.boxType == 1 || c.boxType == 2) 
            deviceColors.push_back(c);
    }

    // In External/CFS-mini mode, deviceColors often contains a single slot that should be reusable:
    // as long as the filament type matches, allow all plateColors to match that same device color.
    const bool reusable_single_device_color =
        (deviceColors.size() == 1) && (deviceColors[0].boxType == 1 || deviceColors[0].boxType == 2);

    std::vector<MatchResult> matched;
    matched.reserve(plateColors.size());

    while (!plateColors.empty() && !deviceColors.empty()) {
        int bestIndex = -1; // index in plateColors
        MatchColorRet bestMatch;
        ModelColor bestExtruderInfo;

        for (int i = 0; i < static_cast<int>(plateColors.size()); ++i) {
            const auto& extr = plateColors[i];
            auto m = getMatchColor(extr.extruderColor, extr.filamentType, deviceColors);
            if ((!bestMatch.filamentType.size() && m.filamentType.size()) ||
                (bestMatch.filamentType.size() && m.filamentType == bestMatch.filamentType && m.distance < bestMatch.distance))
            {
                bestMatch = m;
                bestExtruderInfo = extr;
                bestIndex = i;
            }
        }

        if (bestMatch.filamentType.size()) {
            MatchResult out;
            out.extruderId = bestExtruderInfo.extruderId;
            out.extruderColor = bestExtruderInfo.extruderColor;
            out.filamentLength = bestExtruderInfo.filamentLength;
            out.matchColor = bestMatch.color;
            out.matchFilamentType = bestMatch.filamentType;
            out.materialId = bestMatch.materialId;
            out.boxId = bestMatch.boxId;
            out.cId = bestMatch.cId;
            out.matchStatusCode = bestMatch.matchStatusCode;
            out.msg = bestMatch.msg;
            out.RFIDState = bestMatch.RFIDState;
            out.percent = bestMatch.percent;
            out.remaining_length = bestMatch.remaining_length;
            out.distance = bestMatch.distance;
            matched.push_back(out);

            // erase matched entries as in JS
            plateColors.erase(plateColors.begin() + bestIndex);
            if (!reusable_single_device_color) {
                if (bestMatch.index >= 0 && bestMatch.index < static_cast<int>(deviceColors.size()))
                    deviceColors.erase(deviceColors.begin() + bestMatch.index);
            }
        } else {
            break;
        }
    }

    // Remaining plate colors are unmatched
    for (const auto& extr : plateColors) {
        MatchResult out;
        out.extruderId = extr.extruderId;
        out.extruderColor = extr.extruderColor;
        out.filamentLength = extr.filamentLength;
        // defaults already indicate unmatched (#ffffff, empty type, etc.)
        matched.push_back(out);
    }

    return matched;
}

} // namespace ColorMatch
