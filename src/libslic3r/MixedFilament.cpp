#include "MixedFilament.hpp"
#include "filament_mixer.h"

#include <algorithm>
#include <boost/log/trivial.hpp>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <sstream>
#include <iomanip>
#include <numeric>
#include <unordered_map>
#include <unordered_set>



namespace Slic3r {

static uint64_t canonical_pair_key(unsigned int a, unsigned int b)
{
    const unsigned int lo = std::min(a, b);
    const unsigned int hi = std::max(a, b);
    return (uint64_t(lo) << 32) | uint64_t(hi);
}

// ---------------------------------------------------------------------------
// Gradient curve helpers
// ---------------------------------------------------------------------------
//
// Internal CSV tokenization for the gradient curve string uses ',' as the
// field separator and ';' as the anchor separator. The mixed-row CSV in
// MixedFilamentManager also uses ',' as the field separator, so curve strings
// embedded into a mixed row have to be encoded first to avoid corrupting the
// outer tokenization. The encoding replaces the two conflict characters
// with rare ASCII punctuation that is otherwise unused in this file:
//
//   ',' in the curve payload -> '<'   (so the outer getline(',') stops here)
//   ';' in the curve payload -> '~'   (no need to re-parse anchors, but
//                                       callers must decode before
//                                       parse_gradient_curve).
//
// This is internal to MixedFilament; the curve string is decoded back to its
// canonical form (',', ';') before being handed to parse_gradient_curve.

static std::string encode_gradient_curve_for_row(const std::string &curve_canonical)
{
    std::string out;
    out.reserve(curve_canonical.size());
    for (const char c : curve_canonical) {
        if (c == ',')      out.push_back('<');
        else if (c == ';') out.push_back('~');
        else               out.push_back(c);
    }
    return out;
}

static std::string decode_gradient_curve_for_row(const std::string &curve_encoded)
{
    std::string out;
    out.reserve(curve_encoded.size());
    for (const char c : curve_encoded) {
        if (c == '<')      out.push_back(',');
        else if (c == '~') out.push_back(';');
        else               out.push_back(c);
    }
    return out;
}

// Default Fritsch-Carlson PCHIP tangents for a sorted-by-x anchor list. m
// has size n matching the anchor count; for n == 1 the tangent is 0; for
// n == 2 both endpoint tangents equal the single secant (degenerates to
// linear).
std::vector<double> compute_pchip_default_tangents(const std::vector<GradientAnchor> &pts)
{
    const size_t n = pts.size();
    std::vector<double> m(n, 0.0);
    if (n < 2) return m;

    std::vector<double> d(n - 1);
    for (size_t i = 0; i + 1 < n; ++i) {
        const double h = std::max(1e-12, pts[i + 1].x - pts[i].x);
        d[i] = (pts[i + 1].y - pts[i].y) / h;
    }

    m[0]     = d[0];
    m[n - 1] = d[n - 2];
    for (size_t i = 1; i + 1 < n; ++i)
        m[i] = 0.5 * (d[i - 1] + d[i]);

    // Fritsch-Carlson monotonic guard: kill flats then rescale steep tangents
    // so the resulting cubic never overshoots [min, max] of the surrounding
    // anchors.
    for (size_t i = 0; i + 1 < n; ++i) {
        if (d[i] == 0.0) {
            m[i]     = 0.0;
            m[i + 1] = 0.0;
            continue;
        }
        const double a = m[i]     / d[i];
        const double b = m[i + 1] / d[i];
        const double s = a * a + b * b;
        if (s > 9.0) {
            const double tau = 3.0 / std::sqrt(s);
            m[i]     = tau * a * d[i];
            m[i + 1] = tau * b * d[i];
        }
    }
    return m;
}

GradientCurve parse_gradient_curve(const std::string &s)
{
    GradientCurve curve;
    if (s.empty())
        return curve;

    auto split_commas = [](const std::string &seg) {
        std::vector<std::string> out;
        size_t start = 0;
        while (true) {
            const size_t comma = seg.find(',', start);
            if (comma == std::string::npos) {
                out.emplace_back(seg.substr(start));
                return out;
            }
            out.emplace_back(seg.substr(start, comma - start));
            start = comma + 1;
        }
    };

    auto parse_tangent_token = [](const std::string &t) -> double {
        const std::string trimmed = t; // already whitespace-trimmed by split path
        if (trimmed.empty())
            return std::numeric_limits<double>::quiet_NaN();
        try {
            const double v = std::stod(trimmed);
            return v;
        } catch (...) {
            return std::numeric_limits<double>::quiet_NaN();
        }
    };

    std::istringstream ss(s);
    std::string segment;
    while (std::getline(ss, segment, ';')) {
        if (segment.empty())
            continue;
        const auto fields = split_commas(segment);
        // 2-field legacy form -> (x, y), tangents stay NaN.
        // 4-field form -> (x, y, m_in, m_out), empty / unparseable token -> NaN.
        if (fields.size() != 2 && fields.size() != 4) {
            BOOST_LOG_TRIVIAL(warning) << "parse_gradient_curve: ignoring malformed segment \""
                                       << segment << "\" (expected 2 or 4 comma-separated fields, got "
                                       << fields.size() << ")";
            continue;
        }
        try {
            double x = std::stod(fields[0]);
            double y = std::stod(fields[1]);
            x = std::max(0.0, std::min(1.0, x));
            y = std::max(kGradientMinRatio, std::min(kGradientMaxRatio, y));
            GradientAnchor a;
            a.x = x;
            a.y = y;
            if (fields.size() == 4) {
                a.m_in  = parse_tangent_token(fields[2]);
                a.m_out = parse_tangent_token(fields[3]);
            }
            curve.points.push_back(a);
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(warning) << "parse_gradient_curve: ignoring unparseable segment \""
                                       << segment << "\": " << e.what();
        }
    }

    if (curve.points.size() < 2) {
        if (!curve.points.empty())
            BOOST_LOG_TRIVIAL(warning) << "parse_gradient_curve: only "
                << curve.points.size() << " valid point(s), need at least 2; discarding";
        curve.points.clear();
        return curve;
    }

    std::sort(curve.points.begin(), curve.points.end(),
              [](const GradientAnchor &a, const GradientAnchor &b) {
                  return a.x < b.x;
              });
    return curve;
}

std::string serialize_gradient_curve(const GradientCurve &c)
{
    if (c.points.empty())
        return std::string{};

    std::string out;
    char buf[128];
    for (size_t i = 0; i < c.points.size(); ++i) {
        if (i > 0) out += ';';
        const auto &a = c.points[i];
        const bool has_in  = std::isfinite(a.m_in);
        const bool has_out = std::isfinite(a.m_out);
        if (has_in || has_out) {
            // Emit empty tokens for NaN slots so a future 4-field parser
            // would still split four fields; the new parser interprets empty
            // tokens as "use PCHIP default".
            char in_buf[32]  = {0};
            char out_buf[32] = {0};
            if (has_in)  std::snprintf(in_buf,  sizeof(in_buf),  "%.4f", a.m_in);
            if (has_out) std::snprintf(out_buf, sizeof(out_buf), "%.4f", a.m_out);
            std::snprintf(buf, sizeof(buf), "%.4f,%.4f,%s,%s",
                          a.x, a.y, in_buf, out_buf);
        } else {
            // 4-field form is only emitted when at least one tangent is
            // finite; the 2-field form is emitted otherwise so the JSON
            // payload stays minimal and remains readable by older clients
            // that only know (x, y) pairs.
            std::snprintf(buf, sizeof(buf), "%.4f,%.4f", a.x, a.y);
        }
        out += buf;
    }
    return out;
}

double sample_gradient_curve(const GradientCurve &c, double t)
{
    const auto &pts = c.points;
    if (pts.size() < 2)
        return 0.5;
    if (t <= pts.front().x)
        return pts.front().y;
    if (t >= pts.back().x)
        return pts.back().y;

    // PCHIP defaults are computed for every call; control point counts are
    // typically tiny (< 16) so the allocation cost is negligible compared to
    // any actual rendering or G-code work that drives the sampler.
    const std::vector<double> m_def = compute_pchip_default_tangents(pts);
    const size_t n = pts.size();

    // Linear scan to locate the interval [pts[i].x, pts[i+1].x] containing
    // t. Cheap and avoids the upper_bound boilerplate; n is small.
    for (size_t i = 1; i < n; ++i) {
        const double x0 = pts[i - 1].x;
        const double x1 = pts[i].x;
        if (t > x1) continue;

        const double y0 = pts[i - 1].y;
        const double y1 = pts[i].y;
        const double h  = std::max(1e-12, x1 - x0);
        const double m_left  = std::isfinite(pts[i - 1].m_out) ? pts[i - 1].m_out : m_def[i - 1];
        const double m_right = std::isfinite(pts[i].m_in)      ? pts[i].m_in      : m_def[i];

        const double u   = (t - x0) / h;
        const double u2  = u * u;
        const double u3  = u2 * u;
        const double h00 =  2.0 * u3 - 3.0 * u2 + 1.0;
        const double h10 =        u3 - 2.0 * u2 + u;
        const double h01 = -2.0 * u3 + 3.0 * u2;
        const double h11 =        u3 -       u2;
        double y = h00 * y0 + h10 * h * m_left
                 + h01 * y1 + h11 * h * m_right;
        // Defensive clamp in case tangent overrides on legacy curves push
        // the single-segment Hermite slightly outside the anchor band.
        if (y < kGradientMinRatio) y = kGradientMinRatio;
        if (y > kGradientMaxRatio) y = kGradientMaxRatio;
        return y;
    }
    return pts.back().y;
}

// ---------------------------------------------------------------------------
// Colour helpers (internal)
// ---------------------------------------------------------------------------

struct RGB {
    int r = 0, g = 0, b = 0;
};

struct RGBf {
    float r = 0.f, g = 0.f, b = 0.f;
};

[[maybe_unused]] static float clamp01(float v)
{
    return std::max(0.f, std::min(1.f, v));
}

[[maybe_unused]] static RGBf to_rgbf(const RGB &c)
{
    return {
        clamp01(static_cast<float>(c.r) / 255.f),
        clamp01(static_cast<float>(c.g) / 255.f),
        clamp01(static_cast<float>(c.b) / 255.f)
    };
}

[[maybe_unused]] static RGB to_rgb8(const RGBf &c)
{
    auto to_u8 = [](float v) -> int {
        return std::clamp(static_cast<int>(std::round(clamp01(v) * 255.f)), 0, 255);
    };
    return { to_u8(c.r), to_u8(c.g), to_u8(c.b) };
}


// Convert RGB to an artist-pigment style RYB space.
// This is an approximation, but it gives expected pair mixes:
// Red + Blue -> Purple, Blue + Yellow -> Green, Red + Yellow -> Orange.

// Legacy RYB conversion helpers kept for reference.
// Active code paths use FilamentMixer.
[[maybe_unused]] static RGBf rgb_to_ryb(RGBf in)
{
    float r = clamp01(in.r);
    float g = clamp01(in.g);
    float b = clamp01(in.b);

    const float white = std::min({ r, g, b });
    r -= white;
    g -= white;
    b -= white;

    const float max_g = std::max({ r, g, b });

    float y = std::min(r, g);
    r -= y;
    g -= y;

    if (b > 0.f && g > 0.f) {
        b *= 0.5f;
        g *= 0.5f;
    }

    y += g;
    b += g;

    const float max_y = std::max({ r, y, b });
    if (max_y > 1e-6f) {
        const float n = max_g / max_y;
        r *= n;
        y *= n;
        b *= n;
    }

    r += white;
    y += white;
    b += white;
    return { clamp01(r), clamp01(y), clamp01(b) };
}

[[maybe_unused]] static RGBf ryb_to_rgb(RGBf in)
{
    float r = clamp01(in.r);
    float y = clamp01(in.g);
    float b = clamp01(in.b);

    const float white = std::min({ r, y, b });
    r -= white;
    y -= white;
    b -= white;

    const float max_y = std::max({ r, y, b });

    float g = std::min(y, b);
    y -= g;
    b -= g;

    if (b > 0.f && g > 0.f) {
        b *= 2.f;
        g *= 2.f;
    }

    r += y;
    g += y;

    const float max_g = std::max({ r, g, b });
    if (max_g > 1e-6f) {
        const float n = max_y / max_g;
        r *= n;
        g *= n;
        b *= n;
    }

    r += white;
    g += white;
    b += white;
    return { clamp01(r), clamp01(g), clamp01(b) };
}

// Parse "#RRGGBB" to RGB.  Returns black on failure.
static RGB parse_hex_color(const std::string &hex)
{
    RGB c;
    if (hex.size() >= 7 && hex[0] == '#') {
        try {
            c.r = std::stoi(hex.substr(1, 2), nullptr, 16);
            c.g = std::stoi(hex.substr(3, 2), nullptr, 16);
            c.b = std::stoi(hex.substr(5, 2), nullptr, 16);
        } catch (...) {
            c = {};
        }
    }
    return c;
}

static std::string rgb_to_hex(const RGB &c)
{
    char buf[8];
    std::snprintf(buf, sizeof(buf), "#%02X%02X%02X", c.r, c.g, c.b);
    return std::string(buf);
}

[[maybe_unused]] static std::string blend_color_ryb_legacy(const RGB &rgb_a,
                                                           const RGB &rgb_b,
                                                           int        ratio_a,
                                                           int        ratio_b)
{
    const int safe_a = std::max(0, ratio_a);
    const int safe_b = std::max(0, ratio_b);
    const float total = static_cast<float>(safe_a + safe_b);
    const float wa    = (total > 0.f) ? static_cast<float>(safe_a) / total : 0.5f;
    const float wb    = 1.f - wa;

    const RGBf color_a = to_rgbf(rgb_a);
    const RGBf color_b = to_rgbf(rgb_b);
    const RGBf ryb_a = rgb_to_ryb(color_a);
    const RGBf ryb_b = rgb_to_ryb(color_b);

    RGBf ryb_out;
    ryb_out.r = wa * ryb_a.r + wb * ryb_b.r;
    ryb_out.g = wa * ryb_a.g + wb * ryb_b.g;
    ryb_out.b = wa * ryb_a.b + wb * ryb_b.b;

    RGBf rgb_out = ryb_to_rgb(ryb_out);
    const float v_out = std::max({ rgb_out.r, rgb_out.g, rgb_out.b });
    const float v_tgt = wa * std::max({ color_a.r, color_a.g, color_a.b }) +
                        wb * std::max({ color_b.r, color_b.g, color_b.b });
    if (v_out > 1e-6f && v_tgt > 0.f) {
        const float scale = v_tgt / v_out;
        rgb_out.r = clamp01(rgb_out.r * scale);
        rgb_out.g = clamp01(rgb_out.g * scale);
        rgb_out.b = clamp01(rgb_out.b * scale);
    }

    return rgb_to_hex(to_rgb8(rgb_out));
}

static int clamp_int(int v, int lo, int hi)
{
    return std::max(lo, std::min(hi, v));
}

static int safe_ratio_from_height(float h, float unit)
{
    if (unit <= 1e-6f)
        return 1;
    return std::max(0, int(std::lround(h / unit)));
}

static void compute_gradient_heights(const MixedFilament &mf, float lower_bound, float upper_bound, float &h_a, float &h_b)
{
    const int   mix_b = clamp_int(mf.mix_b_percent, 0, 100);
    const float pct_b = float(mix_b) / 100.f;
    const float pct_a = 1.f - pct_b;
    const float lo    = std::max(0.01f, lower_bound);
    const float hi    = std::max(lo, upper_bound);

    h_a = lo + pct_a * (hi - lo);
    h_b = lo + pct_b * (hi - lo);
}

static void normalize_ratio_pair(int &a, int &b)
{
    a = std::max(0, a);
    b = std::max(0, b);
    if (a == 0 && b == 0) {
        a = 1;
        return;
    }
    if (a > 0 && b > 0) {
        const int g = std::gcd(a, b);
        if (g > 1) {
            a /= g;
            b /= g;
        }
    }
}

static void compute_gradient_ratios(MixedFilament &mf, int gradient_mode, float lower_bound, float upper_bound)
{
    if (gradient_mode == 1) {
        // Height-weighted mode:
        // map blend to [lower, upper], then convert relative heights to an integer cadence.
        float h_a = 0.f;
        float h_b = 0.f;
        compute_gradient_heights(mf, lower_bound, upper_bound, h_a, h_b);
        // Use lower-bound as quantization unit so this mode differs clearly from layer-cycle mode.
        const float unit = std::max(0.01f, std::min(h_a, h_b));
        mf.ratio_a = std::max(1, safe_ratio_from_height(h_a, unit));
        mf.ratio_b = std::max(1, safe_ratio_from_height(h_b, unit));
    } else {
        // Layer-cycle mode:
        // derive a gradual integer cadence directly from the blend ratio
        // by fixing the minority side to one layer and scaling the majority.
        const int mix_b = clamp_int(mf.mix_b_percent, 0, 100);
        if (mix_b <= 0) {
            mf.ratio_a = 1;
            mf.ratio_b = 0;
        } else if (mix_b >= 100) {
            mf.ratio_a = 0;
            mf.ratio_b = 1;
        } else {
            const int pct_b = mix_b;
            const int pct_a = 100 - pct_b;
            const bool b_is_major = pct_b >= pct_a;
            const int major_pct = b_is_major ? pct_b : pct_a;
            const int minor_pct = b_is_major ? pct_a : pct_b;
            const int major_layers = std::max(1, int(std::lround(double(major_pct) / double(std::max(1, minor_pct)))));
            mf.ratio_a = b_is_major ? 1 : major_layers;
            mf.ratio_b = b_is_major ? major_layers : 1;
        }
    }

    normalize_ratio_pair(mf.ratio_a, mf.ratio_b);
}

static int safe_mod(int x, int m)
{
    if (m <= 0)
        return 0;
    int r = x % m;
    return (r < 0) ? (r + m) : r;
}

static int dithering_phase_step(int cycle)
{
    if (cycle <= 1)
        return 0;
    int step = cycle / 2 + 1;
    while (std::gcd(step, cycle) != 1)
        ++step;
    return step % cycle;
}

static bool use_component_b_advanced_dither(int layer_index, int ratio_a, int ratio_b)
{
    ratio_a = std::max(0, ratio_a);
    ratio_b = std::max(0, ratio_b);

    const int cycle = ratio_a + ratio_b;
    if (cycle <= 0 || ratio_b <= 0)
        return false;
    if (ratio_a <= 0)
        return true;

    // Base ordered pattern: as evenly distributed as possible for ratio_b/cycle.
    const int pos = safe_mod(layer_index, cycle);
    const int cycle_idx = (layer_index - pos) / cycle;

    // Rotate each cycle to avoid visible long-period vertical striping.
    const int phase = safe_mod(cycle_idx * dithering_phase_step(cycle), cycle);
    const int p = safe_mod(pos + phase, cycle);

    const int b_before = (p * ratio_b) / cycle;
    const int b_after  = ((p + 1) * ratio_b) / cycle;
    return b_after > b_before;
}

static bool parse_row_definition(const std::string &row,
                                 unsigned int      &a,
                                 unsigned int      &b,
                                 uint64_t          &stable_id,
                                 bool              &enabled,
                                 bool              &custom,
                                 bool              &origin_auto,
                                 int               &mix_b_percent,
                                 bool              &pointillism_all_filaments,
                                 std::string       &gradient_component_ids,
                                 std::string       &gradient_component_weights,
                                 std::string       &manual_pattern,
                                 int               &distribution_mode,
                                 bool              &deleted,
                                 std::string       &gradient_curve_str,
                                 bool              &per_part_gradient,
                                 bool              &gradient_enabled,
                                 int               &gradient_direction)
{
    auto trim_copy = [](const std::string &s) {
        size_t lo = 0;
        size_t hi = s.size();
        while (lo < hi && std::isspace(static_cast<unsigned char>(s[lo])))
            ++lo;
        while (hi > lo && std::isspace(static_cast<unsigned char>(s[hi - 1])))
            --hi;
        return s.substr(lo, hi - lo);
    };

    auto parse_int_token = [&trim_copy](const std::string &tok, int &out) {
        const std::string t = trim_copy(tok);
        if (t.empty())
            return false;
        try {
            size_t consumed = 0;
            int v = std::stoi(t, &consumed);
            if (consumed != t.size())
                return false;
            out = v;
            return true;
        } catch (...) {
            return false;
        }
    };

    auto parse_uint64_token = [&trim_copy](const std::string &tok, uint64_t &out) {
        const std::string t = trim_copy(tok);
        if (t.empty())
            return false;
        try {
            size_t consumed = 0;
            const unsigned long long v = std::stoull(t, &consumed);
            if (consumed != t.size())
                return false;
            out = uint64_t(v);
            return true;
        } catch (...) {
            return false;
        }
    };

    std::vector<std::string> tokens;
    std::stringstream ss(row);
    std::string token;
    while (std::getline(ss, token, ','))
        tokens.emplace_back(trim_copy(token));

    if (tokens.size() < 4)
        return false;

    int values[5] = { 0, 0, 1, 1, 50 };
    if (tokens.size() == 4) {
        // Legacy: a,b,enabled,mix
        if (!parse_int_token(tokens[0], values[0]) ||
            !parse_int_token(tokens[1], values[1]) ||
            !parse_int_token(tokens[2], values[2]) ||
            !parse_int_token(tokens[3], values[4]))
            return false;
    } else {
        // Current: a,b,enabled,custom,mix[,pointillism_all[,pattern]]
        for (size_t i = 0; i < 5; ++i)
            if (!parse_int_token(tokens[i], values[i]))
                return false;
    }

    if (values[0] < 0 || values[1] < 0)
        return false;

    a = unsigned(values[0]);
    b = unsigned(values[1]);
    stable_id = 0;
    enabled = (values[2] != 0);
    custom = (tokens.size() == 4) ? true : (values[3] != 0);
    origin_auto = !custom;
    mix_b_percent = clamp_int(values[4], 0, 100);
    pointillism_all_filaments = false;
    gradient_component_ids.clear();
    gradient_component_weights.clear();
    manual_pattern.clear();
    distribution_mode = int(MixedFilament::Simple);
    deleted = false;
    gradient_curve_str.clear();
    per_part_gradient = false;
    gradient_enabled = false;
    gradient_direction = 0;

    size_t token_idx = 5;
    if (tokens.size() >= 6) {
        // Backward compatibility:
        // - old: token[5] is pointillism flag ("0"/"1")
        // - old: token[5] is pattern ("12", "1212", ...)
        // - new: token[5] may be metadata token ("g..." / "m...")
        const std::string &legacy = tokens[5];
        if (legacy == "0" || legacy == "1") {
            pointillism_all_filaments = (legacy == "1");
            token_idx = 6;
        } else if (legacy.empty() || legacy[0] == 'g' || legacy[0] == 'G' || legacy[0] == 'm' || legacy[0] == 'M') {
            token_idx = 5;
        } else {
            manual_pattern = legacy;
            token_idx = 6;
        }
    }

    std::vector<std::string> pattern_tokens;
    pattern_tokens.reserve(tokens.size() > token_idx ? tokens.size() - token_idx : 1);
    if (!manual_pattern.empty())
        pattern_tokens.push_back(manual_pattern);
    for (size_t i = token_idx; i < tokens.size(); ++i) {
        const std::string &tok = tokens[i];
        if (tok.empty())
            continue;
        if (tok[0] == 'g' || tok[0] == 'G') {
            gradient_component_ids = tok.substr(1);
            continue;
        }
        if (tok[0] == 'w' || tok[0] == 'W') {
            gradient_component_weights = tok.substr(1);
            continue;
        }
        if (tok[0] == 'm' || tok[0] == 'M') {
            int parsed_mode = distribution_mode;
            if (parse_int_token(tok.substr(1), parsed_mode))
                distribution_mode = clamp_int(parsed_mode, int(MixedFilament::LayerCycle), int(MixedFilament::Simple));
            continue;
        }
        if (tok[0] == 'd' || tok[0] == 'D') {
            int parsed_deleted = deleted ? 1 : 0;
            if (parse_int_token(tok.substr(1), parsed_deleted))
                deleted = parsed_deleted != 0;
            continue;
        }
        if (tok[0] == 'o' || tok[0] == 'O') {
            int parsed_origin_auto = origin_auto ? 1 : 0;
            if (parse_int_token(tok.substr(1), parsed_origin_auto))
                origin_auto = parsed_origin_auto != 0;
            continue;
        }
        if (tok[0] == 'u' || tok[0] == 'U') {
            uint64_t parsed_stable_id = stable_id;
            if (parse_uint64_token(tok.substr(1), parsed_stable_id))
                stable_id = parsed_stable_id;
            continue;
        }
        if (tok[0] == 'c' || tok[0] == 'C') {
            // The gradient curve string contains commas (between fields) and
            // semicolons (between anchors). Both are token separators above,
            // so the curve has to be re-joined from trailing tokens starting
            // at the first 'c'/'C' tag. We just store the raw payload here
            // and let the caller re-serialize via serialize_gradient_curve
            // when needed. The convention is: empty payload -> no custom
            // curve (caller falls back to linear gradient range).
            gradient_curve_str = tok.substr(1);
            continue;
        }
        if (tok[0] == 'p' || tok[0] == 'P') {
            int parsed_ppg = per_part_gradient ? 1 : 0;
            if (parse_int_token(tok.substr(1), parsed_ppg))
                per_part_gradient = parsed_ppg != 0;
            continue;
        }
        if (tok[0] == 'e' || tok[0] == 'E') {
            int parsed_ge = gradient_enabled ? 1 : 0;
            if (parse_int_token(tok.substr(1), parsed_ge))
                gradient_enabled = parsed_ge != 0;
            continue;
        }
        if (tok[0] == 'i' || tok[0] == 'I') {
            int parsed_dir = 0;
            if (parse_int_token(tok.substr(1), parsed_dir))
                gradient_direction = clamp_int(parsed_dir, 0, 1);
            continue;
        }
        pattern_tokens.push_back(tok);
    }

    if (!pattern_tokens.empty()) {
        std::ostringstream joined_pattern;
        for (size_t i = 0; i < pattern_tokens.size(); ++i) {
            if (i != 0)
                joined_pattern << ',';
            joined_pattern << pattern_tokens[i];
        }
        manual_pattern = joined_pattern.str();
    }

    // Compatibility for early same-layer prototype rows.
    if (distribution_mode == int(MixedFilament::LayerCycle) && pointillism_all_filaments)
        distribution_mode = int(MixedFilament::SameLayerPointillisme);
    return true;
}

static bool is_pattern_separator(char c)
{
    return std::isspace(static_cast<unsigned char>(c)) || c == '/' || c == '-' || c == '_' || c == '|' || c == ':' || c == ';' || c == ',';
}

static bool decode_pattern_step(char c, char &out)
{
    if (c >= '1' && c <= '9') {
        out = c;
        return true;
    }
    switch (std::tolower(static_cast<unsigned char>(c))) {
    case 'a':
        out = '1';
        return true;
    case 'b':
        out = '2';
        return true;
    default:
        return false;
    }
}

static std::vector<std::string> split_manual_pattern_groups(const std::string &pattern)
{
    std::vector<std::string> groups;
    if (pattern.empty())
        return groups;

    std::string current;
    for (const char c : pattern) {
        if (c == ',') {
            if (!current.empty()) {
                groups.emplace_back(std::move(current));
                current.clear();
            }
            continue;
        }
        current.push_back(c);
    }
    if (!current.empty())
        groups.emplace_back(std::move(current));
    return groups;
}

static std::string flatten_manual_pattern_groups(const std::string &pattern)
{
    std::string flattened;
    flattened.reserve(pattern.size());
    for (const char c : pattern)
        if (c != ',')
            flattened.push_back(c);
    return flattened;
}

static unsigned int physical_filament_from_pattern_step(char token, const MixedFilament &mf, size_t num_physical)
{
    if (token == '1')
        return mf.component_a;
    if (token == '2')
        return mf.component_b;
    if (token >= '3' && token <= '9') {
        const unsigned int direct = unsigned(token - '0');
        if (direct >= 1 && direct <= num_physical)
            return direct;
    }
    return 0;
}

static int mix_percent_from_normalized_pattern(const std::string &pattern)
{
    const std::vector<std::string> groups = split_manual_pattern_groups(pattern);
    if (groups.empty())
        return 50;

    // For grouped patterns, blend preview is the average of each perimeter
    // group's own cadence. This keeps simple outer/inner patterns like
    // "12,21" at 50/50 and "11111112,11121111" at 12.5%.
    double blend_b = 0.0;
    for (const std::string &group : groups) {
        if (group.empty())
            continue;
        const int count_b = int(std::count(group.begin(), group.end(), '2'));
        blend_b += double(count_b) / double(group.size());
    }
    return clamp_int(int(std::lround(100.0 * blend_b / double(groups.size()))), 0, 100);
}

static std::string normalize_gradient_component_ids(const std::string &components)
{
    // Support both formats:
    // 1. New '|'-separated format: "1|2|11|12" (supports multi-digit IDs)
    // 2. Old compact format: "123" (single-char IDs 1-9, no separator)
    if (components.find('|') != std::string::npos) {
        // Deduplicate valid IDs, but preserve every zero placeholder and its weight slot.
        std::string normalized;
        std::vector<unsigned int> seen;
        std::string tok;
        for (char c : components) {
            if (c >= '0' && c <= '9') {
                tok.push_back(c);
            } else if (c == '|') {
                if (!tok.empty()) {
                    try {
                        unsigned int id = std::stoi(tok);
                        if (id == 0 || std::find(seen.begin(), seen.end(), id) == seen.end()) {
                            seen.push_back(id);
                            if (!normalized.empty()) normalized.push_back('|');
                            normalized += std::to_string(id);
                        }
                    } catch (...) {}
                    tok.clear();
                }
            }
        }
        if (!tok.empty()) {
            try {
                unsigned int id = std::stoi(tok);
                if (id == 0 || std::find(seen.begin(), seen.end(), id) == seen.end()) {
                    if (!normalized.empty()) normalized.push_back('|');
                    normalized += std::to_string(id);
                }
            } catch (...) {}
        }
        return normalized;
    }

    // Old compact format: single-char IDs '1'-'9'
    std::string normalized;
    normalized.reserve(components.size());
    bool seen[10] = { false };
    for (const char c : components) {
        // Zero is a positional placeholder for a removed component. Preserve
        // every zero so gradient weights stay aligned with their component.
        if (c == '0') {
            normalized.push_back(c);
            continue;
        }
        if (c < '1' || c > '9')
            continue;
        const int idx = c - '0';
        if (seen[idx])
            continue;
        seen[idx] = true;
        normalized.push_back(c);
    }
    return normalized;
}

// Returns true if the gradient_component_ids string contains a zero placeholder
// (indicating a missing component). Handles both formats:
// - Old compact: '0' character (e.g., "102" has a missing component at position 2)
// - New '|'-separated: "0" token (e.g., "1|0|11" has a missing component)
static bool gradient_component_ids_has_missing(const std::string &components)
{
    if (components.empty()) return false;
    if (components.find('|') != std::string::npos) {
        // New format: check for "0" token
        std::string tok;
        for (char c : components) {
            if (c >= '0' && c <= '9') {
                tok.push_back(c);
            } else if (c == '|') {
                if (tok == "0") return true;
                tok.clear();
            }
        }
        return tok == "0";
    }
    // Old format: check for '0' character
    return components.find('0') != std::string::npos;
}

// Count the number of component IDs in a normalized gradient_component_ids string.
// Handles both formats:
// - Old compact: "123" -> 3 IDs
// - New '|'-separated: "11|12|13" -> 3 IDs
static size_t count_gradient_component_ids(const std::string &components)
{
    if (components.empty()) return 0;
    if (components.find('|') != std::string::npos) {
        // New format: count separators + 1
        size_t count = 1;
        for (char c : components) {
            if (c == '|') ++count;
        }
        return count;
    }
    // Old format: each character is one ID
    return components.size();
}

static std::vector<unsigned int> decode_gradient_component_ids(const std::string &components, size_t num_physical)
{
    std::vector<unsigned int> ids;
    if (components.empty() || num_physical == 0)
        return ids;

    // Support both formats:
    // 1. New '|'-separated format: "1|2|11|12" (supports multi-digit IDs)
    // 2. Old compact format: "123" (single-char IDs 1-9, no separator)
    if (components.find('|') != std::string::npos) {
        // New '|'-separated format
        std::vector<unsigned int> seen;
        std::string tok;
        for (char c : components) {
            if (c >= '0' && c <= '9') {
                tok.push_back(c);
            } else if (c == '|') {
                if (!tok.empty()) {
                    try {
                        unsigned int id = std::stoi(tok);
                        if (id >= 1 && id <= num_physical &&
                            std::find(seen.begin(), seen.end(), id) == seen.end()) {
                            seen.push_back(id);
                            ids.emplace_back(id);
                        }
                    } catch (...) {}
                    tok.clear();
                }
            }
        }
        if (!tok.empty()) {
            try {
                unsigned int id = std::stoi(tok);
                if (id >= 1 && id <= num_physical &&
                    std::find(seen.begin(), seen.end(), id) == seen.end()) {
                    ids.emplace_back(id);
                }
            } catch (...) {}
        }
        return ids;
    }

    // Old compact format: single-char IDs '1'-'9'
    bool seen[10] = { false };
    ids.reserve(components.size());
    for (const char c : components) {
        if (c < '1' || c > '9')
            continue;
        const unsigned int id = unsigned(c - '0');
        if (id > num_physical || seen[id])
            continue;
        seen[id] = true;
        ids.emplace_back(id);
    }
    return ids;
}

static std::vector<int> parse_gradient_weight_tokens(const std::string &weights)
{
    std::vector<int> out;
    std::string token;
    for (const char c : weights) {
        if (c >= '0' && c <= '9') {
            token.push_back(c);
            continue;
        }
        if (!token.empty()) {
            out.emplace_back(std::max(0, std::atoi(token.c_str())));
            token.clear();
        }
    }
    if (!token.empty())
        out.emplace_back(std::max(0, std::atoi(token.c_str())));
    return out;
}

static std::vector<int> normalize_weight_vector_to_percent(const std::vector<int> &weights)
{
    std::vector<int> out(weights.size(), 0);
    if (weights.empty())
        return out;
    int sum = 0;
    for (const int w : weights)
        sum += std::max(0, w);
    if (sum <= 0)
        return out;

    std::vector<double> remainders(weights.size(), 0.);
    int assigned = 0;
    for (size_t i = 0; i < weights.size(); ++i) {
        const double exact = 100.0 * double(std::max(0, weights[i])) / double(sum);
        out[i] = int(std::floor(exact));
        remainders[i] = exact - double(out[i]);
        assigned += out[i];
    }
    int missing = std::max(0, 100 - assigned);
    while (missing > 0) {
        size_t best_idx = 0;
        double best_rem = -1.0;
        for (size_t i = 0; i < remainders.size(); ++i) {
            if (weights[i] <= 0)
                continue;
            if (remainders[i] > best_rem) {
                best_rem = remainders[i];
                best_idx = i;
            }
        }
        ++out[best_idx];
        remainders[best_idx] = 0.0;
        --missing;
    }
    return out;
}

static std::string normalize_gradient_component_weights(const std::string &weights, size_t expected_components)
{
    if (expected_components == 0)
        return std::string();
    std::vector<int> parsed = parse_gradient_weight_tokens(weights);
    if (parsed.size() != expected_components)
        return std::string();
    std::vector<int> normalized = normalize_weight_vector_to_percent(parsed);
    int sum = 0;
    for (const int v : normalized)
        sum += v;
    if (sum <= 0)
        return std::string();

    std::ostringstream ss;
    for (size_t i = 0; i < normalized.size(); ++i) {
        if (i > 0)
            ss << '/';
        ss << normalized[i];
    }
    return ss.str();
}

static std::vector<int> decode_gradient_component_weights(const std::string &weights, size_t expected_components)
{
    if (expected_components == 0)
        return {};
    std::vector<int> parsed = parse_gradient_weight_tokens(weights);
    if (parsed.size() != expected_components)
        return {};
    std::vector<int> normalized = normalize_weight_vector_to_percent(parsed);
    int sum = 0;
    for (const int v : normalized)
        sum += v;
    return (sum > 0) ? normalized : std::vector<int>();
}

static std::vector<unsigned int> build_weighted_gradient_sequence(const std::vector<unsigned int> &ids,
                                                                  const std::vector<int>          &weights)
{
    if (ids.empty())
        return {};

    std::vector<unsigned int> filtered_ids;
    std::vector<int>          counts;
    filtered_ids.reserve(ids.size());
    counts.reserve(ids.size());
    for (size_t i = 0; i < ids.size(); ++i) {
        const int w = (i < weights.size()) ? std::max(0, weights[i]) : 0;
        if (w <= 0)
            continue;
        filtered_ids.emplace_back(ids[i]);
        counts.emplace_back(w);
    }
    if (filtered_ids.empty()) {
        filtered_ids = ids;
        counts.assign(ids.size(), 1);
    }

    int g = 0;
    for (const int c : counts)
        g = std::gcd(g, std::max(1, c));
    if (g > 1) {
        for (int &c : counts)
            c = std::max(1, c / g);
    }

    int cycle = std::accumulate(counts.begin(), counts.end(), 0);
    constexpr int k_max_cycle = 48;
    if (cycle > k_max_cycle) {
        const double scale = double(k_max_cycle) / double(cycle);
        for (int &c : counts)
            c = std::max(1, int(std::round(double(c) * scale)));
        cycle = std::accumulate(counts.begin(), counts.end(), 0);
        while (cycle > k_max_cycle) {
            auto it = std::max_element(counts.begin(), counts.end());
            if (it == counts.end() || *it <= 1)
                break;
            --(*it);
            --cycle;
        }
    }
    if (cycle <= 0)
        return {};

    std::vector<unsigned int> sequence;
    sequence.reserve(size_t(cycle));
    std::vector<int> emitted(counts.size(), 0);
    for (int pos = 0; pos < cycle; ++pos) {
        size_t best_idx = 0;
        double best_score = -1e9;
        for (size_t i = 0; i < counts.size(); ++i) {
            const double target = double((pos + 1) * counts[i]) / double(cycle);
            const double score = target - double(emitted[i]);
            if (score > best_score) {
                best_score = score;
                best_idx = i;
            }
        }
        ++emitted[best_idx];
        sequence.emplace_back(filtered_ids[best_idx]);
    }
    return sequence;
}

// ---------------------------------------------------------------------------
// MixedFilamentManager
// ---------------------------------------------------------------------------

uint64_t MixedFilamentManager::allocate_stable_id()
{
    const uint64_t stable_id = std::max<uint64_t>(1, m_next_stable_id);
    m_next_stable_id = stable_id + 1;
    return stable_id;
}

uint64_t MixedFilamentManager::normalize_stable_id(uint64_t stable_id)
{
    if (stable_id == 0)
        return allocate_stable_id();
    if (stable_id >= m_next_stable_id)
        m_next_stable_id = stable_id + 1;
    return stable_id;
}

std::vector<unsigned int> MixedFilament::referenced_component_ids(size_t num_physical) const
{
    std::vector<unsigned int> ids;
    if (!gradient_component_ids.empty()) {
        ids = decode_gradient_component_ids(gradient_component_ids, num_physical);
        // decode_gradient_component_ids already drops 0 / out-of-range IDs.
    }
    if (ids.size() < 2) {
        ids.clear();
        if (component_a >= 1 && component_a <= num_physical)
            ids.push_back(component_a);
        if (component_b >= 1 && component_b <= num_physical &&
            std::find(ids.begin(), ids.end(), component_b) == ids.end())
            ids.push_back(component_b);
    }
    return ids;
}

bool MixedFilament::has_type_mismatch(const std::vector<std::string> &physical_types) const
{
    if (physical_types.empty())
        return false;

    const std::vector<unsigned int> ids = referenced_component_ids(physical_types.size());
    std::string ref_type;
    for (unsigned int c : ids) {
        if (c < 1 || c > physical_types.size())
            continue;
        const std::string &ft = physical_types[c - 1];
        if (ft.empty())
            continue;
        if (ref_type.empty())
            ref_type = ft;
        else if (ft != ref_type)
            return true;
    }
    return false;
}

void MixedFilamentManager::auto_generate(const std::vector<std::string> &filament_colours)
{
    // Keep a copy of the old list so we can preserve user-modified ratios and
    // enabled flags, custom rows and incomplete rows awaiting repair.
    std::vector<MixedFilament> old = std::move(m_mixed);
    m_mixed.clear();

    const size_t n = filament_colours.size();

    std::vector<MixedFilament> custom_rows;
    std::vector<MixedFilament> incomplete_rows;
    custom_rows.reserve(old.size());
    incomplete_rows.reserve(old.size());
    std::unordered_map<uint64_t, const MixedFilament *> old_auto_rows;
    old_auto_rows.reserve(old.size());
    for (const MixedFilament &prev : old) {
        const bool incomplete = prev.component_a == 0 || prev.component_b == 0 ||
                                prev.component_a > n || prev.component_b > n ||
                                prev.component_a == prev.component_b ||
                                gradient_component_ids_has_missing(prev.gradient_component_ids);
        if (incomplete) {
            MixedFilament retained = prev;
            retained.stable_id = normalize_stable_id(retained.stable_id);
            retained.enabled = false;
            incomplete_rows.push_back(std::move(retained));
            continue;
        }
        if (!prev.custom) {
            old_auto_rows.emplace(canonical_pair_key(prev.component_a, prev.component_b), &prev);
            continue;
        }
        MixedFilament custom = prev;
        custom.stable_id = normalize_stable_id(custom.stable_id);
        custom_rows.push_back(std::move(custom));
    }

    if (n >= 2) {
        // CRITICAL: Handle both addition and deletion of physical filaments correctly.
        // Existing pairs preserve state; only genuinely new pairs are enabled.
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                const auto key = canonical_pair_key(static_cast<unsigned int>(i + 1), static_cast<unsigned int>(j + 1));
                const auto it_prev = old_auto_rows.find(key);

                MixedFilament mf;
                mf.component_a = static_cast<unsigned int>(i + 1);
                mf.component_b = static_cast<unsigned int>(j + 1);
                mf.ratio_a = 1;
                mf.ratio_b = 1;
                mf.mix_b_percent = 50;
                mf.custom = false;
                mf.origin_auto = true;

                if (it_prev != old_auto_rows.end()) {
                    const MixedFilament &prev = *it_prev->second;
                    mf.enabled = prev.enabled;
                    mf.deleted = prev.deleted;
                    mf.stable_id = prev.stable_id;
                    if (mf.deleted)
                        mf.enabled = false;
                } else {
                    mf.enabled = true;
                    mf.deleted = false;
                    mf.stable_id = 0;
                }

                mf.stable_id = normalize_stable_id(mf.stable_id);
                m_mixed.push_back(mf);
            }
        }
    }

    for (MixedFilament &mf : custom_rows)
        m_mixed.push_back(std::move(mf));
    for (MixedFilament &mf : incomplete_rows)
        m_mixed.push_back(std::move(mf));

    refresh_display_colors(filament_colours);
}

void MixedFilamentManager::remove_physical_filament(unsigned int deleted_filament_id, unsigned int num_physicals)
{
    if (deleted_filament_id == 0 || deleted_filament_id > num_physicals || m_mixed.empty())
        return;

    for (MixedFilament &mf : m_mixed) {
        bool missing_component = false;

        // Preserve every gradient slot so its weight remains aligned. The
        // removed physical filament becomes a zero placeholder; later IDs are
        // shifted down to follow the physical-filament renumbering.
        if (!mf.gradient_component_ids.empty()) {
            if (mf.gradient_component_ids.find('|') != std::string::npos) {
                // New '|'-separated format: parse tokens, adjust IDs, re-encode
                std::string adjusted;
                std::string tok;
                bool first = true;
                for (char c : mf.gradient_component_ids) {
                    if (c >= '0' && c <= '9') {
                        tok.push_back(c);
                    } else if (c == '|') {
                        if (!tok.empty()) {
                            try {
                                unsigned int id = std::stoi(tok);
                                if (id == deleted_filament_id)
                                    id = 0;
                                else if (id > deleted_filament_id)
                                    --id;
                                if (id == 0)
                                    missing_component = true;
                                if (!first) adjusted.push_back('|');
                                adjusted += std::to_string(id);
                                first = false;
                            } catch (...) {}
                            tok.clear();
                        }
                    }
                }
                if (!tok.empty()) {
                    try {
                        unsigned int id = std::stoi(tok);
                        if (id == deleted_filament_id)
                            id = 0;
                        else if (id > deleted_filament_id)
                            --id;
                        if (id == 0)
                            missing_component = true;
                        if (!first) adjusted.push_back('|');
                        adjusted += std::to_string(id);
                    } catch (...) {}
                }
                mf.gradient_component_ids = normalize_gradient_component_ids(adjusted);
            } else {
                // Old compact format: single-char IDs '0'-'9'
                std::string adjusted;
                adjusted.reserve(mf.gradient_component_ids.size());
                for (const char c : mf.gradient_component_ids) {
                    if (c < '0' || c > '9')
                        continue;
                    unsigned int id = unsigned(c - '0');
                    if (id == deleted_filament_id)
                        id = 0;
                    else if (id > deleted_filament_id)
                        --id;
                    if (id == 0)
                        missing_component = true;
                    adjusted.push_back(char('0' + id));
                }
                mf.gradient_component_ids = normalize_gradient_component_ids(adjusted);
            }
        }

        auto adjust_component = [deleted_filament_id, &missing_component](unsigned int &component) {
            if (component == deleted_filament_id)
                component = 0;
            else if (component > deleted_filament_id)
                --component;
            if (component == 0)
                missing_component = true;
        };
        adjust_component(mf.component_a);
        adjust_component(mf.component_b);

        // Keep the row and its virtual filament slot, but mark it unavailable
        // until every missing component has been selected again.
        if (missing_component || mf.component_a == mf.component_b)
            mf.enabled = false;
    }
}

void MixedFilamentManager::add_custom_filament(unsigned int component_a,
                                               unsigned int component_b,
                                               int          mix_b_percent,
                                               const std::vector<std::string> &filament_colours)
{
    const size_t n = filament_colours.size();
    if (n < 2)
        return;

    component_a = std::max<unsigned int>(1, std::min<unsigned int>(component_a, unsigned(n)));
    component_b = std::max<unsigned int>(1, std::min<unsigned int>(component_b, unsigned(n)));
    if (component_a == component_b) {
        component_b = (component_a == 1) ? 2 : 1;
    }

    MixedFilament mf;
    mf.component_a = component_a;
    mf.component_b = component_b;
    mf.stable_id = allocate_stable_id();
    mf.mix_b_percent = clamp_int(mix_b_percent, 0, 100);
    mf.ratio_a = 1;
    mf.ratio_b = 1;
    mf.manual_pattern.clear();
    mf.gradient_component_ids.clear();
    mf.gradient_component_weights.clear();
    mf.pointillism_all_filaments = false;
    mf.distribution_mode = int(MixedFilament::Simple);
    mf.enabled = true;
    mf.deleted = false;
    mf.custom = true;
    mf.origin_auto = false;
    m_mixed.push_back(std::move(mf));
    refresh_display_colors(filament_colours);
}

void MixedFilamentManager::clear_custom_entries()
{
    // Incomplete auto rows are also reloaded from the serialized definition;
    // remove them here to avoid duplicating the same stable row on reload.
    m_mixed.erase(std::remove_if(m_mixed.begin(), m_mixed.end(), [](const MixedFilament &mf) {
        return mf.custom || mf.component_a == 0 || mf.component_b == 0 ||
               gradient_component_ids_has_missing(mf.gradient_component_ids);
    }), m_mixed.end());
}

std::string MixedFilamentManager::normalize_manual_pattern(const std::string &pattern)
{
    std::string normalized;
    normalized.reserve(pattern.size());
    bool current_group_has_steps = false;
    for (char c : pattern) {
        char step = '\0';
        if (decode_pattern_step(c, step)) {
            normalized.push_back(step);
            current_group_has_steps = true;
            continue;
        }
        if (c == ',') {
            if (!current_group_has_steps)
                return std::string();
            normalized.push_back(',');
            current_group_has_steps = false;
            continue;
        }
        if (is_pattern_separator(c))
            continue;
        // Unknown token => invalid pattern.
        return std::string();
    }
    if (!normalized.empty() && normalized.back() == ',')
        return std::string();
    return normalized;
}

int MixedFilamentManager::mix_percent_from_manual_pattern(const std::string &pattern)
{
    return mix_percent_from_normalized_pattern(normalize_manual_pattern(pattern));
}

void MixedFilamentManager::apply_gradient_settings(int   gradient_mode,
                                                   float lower_bound,
                                                   float upper_bound,
                                                   bool  advanced_dithering)
{
    m_gradient_mode      = (gradient_mode != 0) ? 1 : 0;
    m_height_lower_bound = std::max(0.01f, lower_bound);
    m_height_upper_bound = std::max(m_height_lower_bound, upper_bound);
    m_advanced_dithering = advanced_dithering;

    for (MixedFilament &mf : m_mixed) {
        if (!mf.custom) {
            mf.ratio_a = 1;
            mf.ratio_b = 1;
            continue;
        }
        compute_gradient_ratios(mf, m_gradient_mode, m_height_lower_bound, m_height_upper_bound);
    }
}

std::string MixedFilamentManager::serialize_custom_entries()
{
    std::ostringstream ss;
    bool first = true;
    for (MixedFilament &mf : m_mixed) {
        if (!first)
            ss << ';';
        first = false;
        mf.stable_id = normalize_stable_id(mf.stable_id);
        const std::string normalized_ids = normalize_gradient_component_ids(mf.gradient_component_ids);
        const size_t num_ids = count_gradient_component_ids(normalized_ids);
        const std::string normalized_weights = normalize_gradient_component_weights(mf.gradient_component_weights, num_ids);
        ss << mf.component_a << ','
           << mf.component_b << ','
           << (mf.enabled ? 1 : 0) << ','
           << (mf.custom ? 1 : 0) << ','
           << clamp_int(mf.mix_b_percent, 0, 100) << ','
           << (mf.pointillism_all_filaments ? 1 : 0) << ','
           << 'g' << normalized_ids << ','
           << 'w' << normalized_weights << ','
           << 'm' << clamp_int(mf.distribution_mode, int(MixedFilament::LayerCycle), int(MixedFilament::Simple)) << ','
           << 'd' << (mf.deleted ? 1 : 0) << ','
           << 'o' << (mf.origin_auto ? 1 : 0) << ','
           << 'u' << mf.stable_id << ','
           << 'c' << encode_gradient_curve_for_row(serialize_gradient_curve(mf.gradient_curve)) << ','
           << 'p' << (mf.per_part_gradient ? 1 : 0) << ','
           << 'e' << (mf.gradient_enabled ? 1 : 0) << ','
           << 'i' << clamp_int(mf.gradient_direction, 0, 1);
        const std::string normalized_pattern = normalize_manual_pattern(mf.manual_pattern);
        if (!normalized_pattern.empty())
            ss << ',' << normalized_pattern;
    }
    return ss.str();
}

void MixedFilamentManager::load_custom_entries(const std::string &serialized, const std::vector<std::string> &filament_colours)
{
    const size_t n = filament_colours.size();
    if (serialized.empty()) {
        BOOST_LOG_TRIVIAL(debug) << "MixedFilamentManager::load_custom_entries skipped"
                                 << ", serialized_empty=1"
                                 << ", physical_count=" << n;
        return;
    }

    size_t parsed_rows   = 0;
    size_t loaded_rows   = 0;
    size_t updated_auto  = 0;
    size_t appended_auto = 0;
    size_t skipped_rows  = 0;

    std::vector<const MixedFilament *> auto_rows_in_order;
    auto_rows_in_order.reserve(m_mixed.size());
    std::unordered_map<uint64_t, const MixedFilament *> auto_rows_by_pair;
    auto_rows_by_pair.reserve(m_mixed.size());
    for (const MixedFilament &mf : m_mixed) {
        const bool complete_pair = mf.component_a != 0 && mf.component_b != 0 &&
                                   mf.component_a <= n && mf.component_b <= n &&
                                   mf.component_a != mf.component_b &&
                                   !gradient_component_ids_has_missing(mf.gradient_component_ids);
        if (!mf.custom && complete_pair) {
            auto_rows_in_order.push_back(&mf);
            auto_rows_by_pair.emplace(canonical_pair_key(mf.component_a, mf.component_b), &mf);
        }
    }

    std::vector<MixedFilament> rebuilt;
    rebuilt.reserve(m_mixed.size() + 8);
    std::unordered_set<uint64_t> consumed_auto_pairs;
    consumed_auto_pairs.reserve(auto_rows_by_pair.size());
    std::unordered_set<uint64_t> used_stable_ids;
    used_stable_ids.reserve(m_mixed.size() + 8);
    auto dedupe_stable_id = [this, &used_stable_ids](uint64_t stable_id) {
        stable_id = normalize_stable_id(stable_id);
        if (used_stable_ids.insert(stable_id).second)
            return stable_id;
        uint64_t replacement = allocate_stable_id();
        used_stable_ids.insert(replacement);
        return replacement;
    };

    std::stringstream all(serialized);
    std::string row;
    int row_num = 0;
    while (std::getline(all, row, ';')) {
        row_num++;
        if (row.empty())
            continue;
        ++parsed_rows;
        unsigned int a = 0;
        unsigned int b = 0;
        uint64_t stable_id = 0;
        bool enabled = true;
        bool custom = true;
        bool origin_auto = false;
        int mix = 50;
        bool pointillism_all_filaments = false;
        std::string gradient_component_ids;
        std::string gradient_component_weights;
        std::string manual_pattern;
        int distribution_mode = int(MixedFilament::Simple);
        bool deleted = false;
        std::string gradient_curve_str;
        bool per_part_gradient = false;
        bool gradient_enabled = false;
        int  gradient_direction = 0;
        if (!parse_row_definition(row, a, b, stable_id, enabled, custom, origin_auto, mix, pointillism_all_filaments,
                                  gradient_component_ids, gradient_component_weights, manual_pattern, distribution_mode, deleted,
                                  gradient_curve_str, per_part_gradient, gradient_enabled, gradient_direction)) {
            ++skipped_rows;
            BOOST_LOG_TRIVIAL(warning) << "MixedFilamentManager::load_custom_entries invalid row format: " << row;
            continue;
        }
        const std::string normalized_gradient_ids = normalize_gradient_component_ids(gradient_component_ids);
        const bool missing_component = a == 0 || b == 0 || gradient_component_ids_has_missing(normalized_gradient_ids);
        if ((a != 0 && a > n) || (b != 0 && b > n) || (a != 0 && b != 0 && a == b)) {
            ++skipped_rows;
            continue;
        }

        // Incomplete rows cannot be matched to a canonical auto pair, but they
        // must survive reload so the user can repair the missing component.
        if (missing_component) {
            MixedFilament mf;
            mf.component_a = a;
            mf.component_b = b;
            mf.stable_id = dedupe_stable_id(stable_id);
            mf.mix_b_percent = mix;
            mf.ratio_a = 1;
            mf.ratio_b = 1;
            mf.pointillism_all_filaments = pointillism_all_filaments;
            mf.gradient_component_ids = normalized_gradient_ids;
            mf.gradient_component_weights =
                normalize_gradient_component_weights(gradient_component_weights, count_gradient_component_ids(normalized_gradient_ids));
            mf.manual_pattern = normalize_manual_pattern(manual_pattern);
            mf.distribution_mode = clamp_int(distribution_mode, int(MixedFilament::LayerCycle), int(MixedFilament::Simple));
            mf.enabled = false;
            mf.deleted = deleted;
            mf.custom = custom;
            mf.origin_auto = origin_auto;
            rebuilt.push_back(std::move(mf));
            ++loaded_rows;
            continue;
        }

        if (!custom) {
            const uint64_t key = canonical_pair_key(a, b);
            if (consumed_auto_pairs.count(key) != 0) {
                ++skipped_rows;
                continue;
            }

            auto it_auto = auto_rows_by_pair.find(key);
            if (it_auto == auto_rows_by_pair.end()) {
                ++skipped_rows;
                continue;
            }

            MixedFilament mf = *it_auto->second;
            mf.component_a = std::min(a, b);
            mf.component_b = std::max(a, b);
            mf.stable_id = dedupe_stable_id(stable_id != 0 ? stable_id : mf.stable_id);
            mf.enabled = enabled;
            mf.pointillism_all_filaments = pointillism_all_filaments;
            mf.gradient_component_ids = normalize_gradient_component_ids(gradient_component_ids);
            mf.gradient_component_weights =
                normalize_gradient_component_weights(gradient_component_weights, count_gradient_component_ids(mf.gradient_component_ids));
            mf.manual_pattern = normalize_manual_pattern(manual_pattern);
            mf.distribution_mode = clamp_int(distribution_mode, int(MixedFilament::LayerCycle), int(MixedFilament::Simple));
            mf.mix_b_percent = mf.manual_pattern.empty() ? mix : mix_percent_from_normalized_pattern(mf.manual_pattern);
            mf.deleted = deleted;
            if (mf.deleted)
                mf.enabled = false;
            mf.custom = false;
            mf.origin_auto = true;
            mf.gradient_curve = parse_gradient_curve(decode_gradient_curve_for_row(gradient_curve_str));
            mf.per_part_gradient = per_part_gradient;
            mf.gradient_enabled = gradient_enabled;

            rebuilt.push_back(std::move(mf));
            consumed_auto_pairs.insert(key);
            ++updated_auto;
            continue;
        }

        MixedFilament mf;
        mf.component_a = a;
        mf.component_b = b;
        mf.stable_id = dedupe_stable_id(stable_id);
        mf.mix_b_percent = mix;
        mf.ratio_a = 1;
        mf.ratio_b = 1;
        mf.pointillism_all_filaments = pointillism_all_filaments;
        mf.gradient_component_ids = normalize_gradient_component_ids(gradient_component_ids);
        mf.gradient_component_weights =
            normalize_gradient_component_weights(gradient_component_weights, count_gradient_component_ids(mf.gradient_component_ids));
        mf.manual_pattern = normalize_manual_pattern(manual_pattern);
        mf.distribution_mode = clamp_int(distribution_mode, int(MixedFilament::LayerCycle), int(MixedFilament::Simple));
        if (!mf.manual_pattern.empty())
            mf.mix_b_percent = mix_percent_from_normalized_pattern(mf.manual_pattern);
        mf.enabled = enabled;
        mf.deleted = deleted;
        if (mf.deleted)
            mf.enabled = false;
        mf.custom = custom;
        mf.origin_auto = origin_auto;
        mf.gradient_curve = parse_gradient_curve(decode_gradient_curve_for_row(gradient_curve_str));
        mf.per_part_gradient = per_part_gradient;
        mf.gradient_enabled = gradient_enabled;
        rebuilt.push_back(std::move(mf));
        ++loaded_rows;
    }

    // Keep any newly generated auto rows that were not present in serialized
    // definitions and append them at the end to preserve existing virtual IDs.
    for (const MixedFilament *auto_mf_ptr : auto_rows_in_order) {
        if (auto_mf_ptr == nullptr)
            continue;
        const uint64_t key = canonical_pair_key(auto_mf_ptr->component_a, auto_mf_ptr->component_b);
        if (consumed_auto_pairs.count(key) != 0)
            continue;
        MixedFilament mf = *auto_mf_ptr;
        const unsigned int lo = std::min(mf.component_a, mf.component_b);
        const unsigned int hi = std::max(mf.component_a, mf.component_b);
        mf.component_a = lo;
        mf.component_b = hi;
        mf.stable_id = dedupe_stable_id(mf.stable_id);
        mf.custom = false;
        mf.origin_auto = true;
        rebuilt.push_back(std::move(mf));
        ++appended_auto;
    }

    m_mixed = std::move(rebuilt);
    refresh_display_colors(filament_colours);
    BOOST_LOG_TRIVIAL(info) << "MixedFilamentManager::load_custom_entries"
                            << ", physical_count=" << n
                            << ", parsed_rows=" << parsed_rows
                            << ", loaded_rows=" << loaded_rows
                            << ", updated_auto_rows=" << updated_auto
                            << ", appended_auto_rows=" << appended_auto
                            << ", skipped_rows=" << skipped_rows
                            << ", mixed_total=" << m_mixed.size();
}

bool MixedFilamentManager::is_definitions_change_append_only(const std::string &old_serialized,
                                                             const std::string &new_serialized)
{
    if (old_serialized == new_serialized)
        return true; // No change at all.

    // The geometry-affecting signature of one enabled (non-deleted) mixed row,
    // in the order it contributes a virtual filament ID. Two definition strings
    // produce identical slice geometry when their enabled-row signature lists
    // are equal, and the new list is allowed to have extra rows appended at the
    // end (those appended virtual filaments are not painted on the model yet).
    auto enabled_signatures = [](const std::string &serialized, bool &parse_ok) {
        std::vector<std::string> signatures;
        parse_ok = true;
        std::stringstream all(serialized);
        std::string row;
        while (std::getline(all, row, ';')) {
            if (row.empty())
                continue;
            unsigned int a = 0, b = 0;
            uint64_t     stable_id = 0;
            bool         enabled = true, custom = true, origin_auto = false;
            int          mix = 50;
            bool         pointillism_all_filaments = false;
            std::string  gradient_component_ids, gradient_component_weights, manual_pattern;
            int          distribution_mode = int(MixedFilament::Simple);
            bool         deleted = false;
            std::string  gradient_curve_str;
            bool         per_part_gradient = false;
            bool         gradient_enabled = false;
            int          gradient_direction = 0;
            if (!parse_row_definition(row, a, b, stable_id, enabled, custom, origin_auto, mix,
                                      pointillism_all_filaments, gradient_component_ids,
                                      gradient_component_weights, manual_pattern, distribution_mode, deleted,
                                      gradient_curve_str, per_part_gradient, gradient_enabled, gradient_direction)) {
                parse_ok = false;
                return signatures;
            }
            // Disabled/deleted rows do not get a virtual filament ID and do not
            // affect slicing, so they are ignored for the comparison.
            if (!enabled || deleted)
                continue;
            // Build a signature from exactly the fields that influence resolved
            // geometry (component IDs, blend ratio, pattern, gradient layout and
            // distribution mode, the per-part gradient flag, and the gradient-
            // enabled toggle). The custom curve string is also part of the
            // signature because changing the curve shape changes the per-layer
            // color blend, and toggling gradient_enabled flips the slicer
            // between fixed-ratio and gradient mode. stable_id, custom/origin_auto
            // flags and colors are intentionally excluded.
            const std::string normalized_ids = normalize_gradient_component_ids(gradient_component_ids);
            const std::string normalized_curve = serialize_gradient_curve(parse_gradient_curve(decode_gradient_curve_for_row(gradient_curve_str)));
            std::ostringstream sig;
            sig << a << '/' << b << '/' << clamp_int(mix, 0, 100) << '/'
                << clamp_int(distribution_mode, int(MixedFilament::LayerCycle), int(MixedFilament::Simple)) << '/'
                << 'g' << normalized_ids << '/'
                << 'w' << normalize_gradient_component_weights(gradient_component_weights, count_gradient_component_ids(normalized_ids)) << '/'
                << 'm' << normalize_manual_pattern(manual_pattern) << '/'
                << 'c' << normalized_curve << '/'
                << 'p' << (per_part_gradient ? '1' : '0') << '/'
                << 'e' << (gradient_enabled ? '1' : '0');
            signatures.emplace_back(sig.str());
        }
        return signatures;
    };

    bool old_ok = true, new_ok = true;
    const std::vector<std::string> old_sigs = enabled_signatures(old_serialized, old_ok);
    const std::vector<std::string> new_sigs = enabled_signatures(new_serialized, new_ok);
    if (!old_ok || !new_ok)
        return false; // Cannot prove safety -> treat as a geometry change.

    // Append-only requires every previously enabled row to remain unchanged in
    // the same position; the new list may only grow at the end.
    if (new_sigs.size() < old_sigs.size())
        return false;
    for (size_t i = 0; i < old_sigs.size(); ++i) {
        if (old_sigs[i] != new_sigs[i])
            return false;
    }
    return true;
}


unsigned int MixedFilamentManager::resolve(unsigned int filament_id,
                                           size_t       num_physical,
                                           int          layer_index,
                                           float        layer_print_z,
                                           float        layer_height,
                                           bool         force_height_weighted) const
{
    const int mixed_idx = mixed_index_from_filament_id(filament_id, num_physical);
    if (mixed_idx < 0)
        return filament_id;

    const MixedFilament &mf = m_mixed[size_t(mixed_idx)];
    if (!mf.is_available(num_physical))
        throw SlicingError(L("The mixed filament references a filament that no longer exists. Please reselect the original filament. Slicing will be available once the configuration is complete."));

    // Manual pattern takes precedence when provided. Pattern uses repeating
    // steps: '1' => component_a, '2' => component_b, '3'..'9' => direct
    // physical filament IDs.
    if (!mf.manual_pattern.empty()) {
        const std::string flattened_pattern = flatten_manual_pattern_groups(mf.manual_pattern);
        if (!flattened_pattern.empty()) {
            const int pos = safe_mod(layer_index, int(flattened_pattern.size()));
            const unsigned int resolved = physical_filament_from_pattern_step(flattened_pattern[size_t(pos)], mf, num_physical);
            if (resolved >= 1 && resolved <= num_physical)
                return resolved;
        }
        return mf.component_a;
    }

    const bool use_simple_mode = mf.distribution_mode == int(MixedFilament::Simple);
    const std::vector<unsigned int> gradient_ids = decode_gradient_component_ids(mf.gradient_component_ids, num_physical);
    if (!use_simple_mode && gradient_ids.size() >= 3) {
        const std::vector<int> gradient_weights =
            decode_gradient_component_weights(mf.gradient_component_weights, gradient_ids.size());
        const std::vector<unsigned int> gradient_sequence = build_weighted_gradient_sequence(
            gradient_ids, gradient_weights.empty() ? std::vector<int>(gradient_ids.size(), 1) : gradient_weights);
        if (!gradient_sequence.empty()) {
            const size_t pos = size_t(safe_mod(layer_index, int(gradient_sequence.size())));
            return gradient_sequence[pos];
        }
    }

    // Height-weighted cadence can be forced by the local-Z planner. The
    // regular gradient height mode keeps historical behavior (custom rows).
    // Simple distribution mode always uses the integer layer-index cadence,
    // regardless of the global gradient mode setting.
    //
    // Per-row gradient_enabled (persisted from the MixedFilamentDialog
    // "Gradient Effect" toggle) also activates height-weighted mode for
    // that specific row, independent of the global m_gradient_mode setting.
    // This is what lets a user enable gradient on a single mixed row without
    // flipping the global "Height-weighted cadence" print setting.
    const bool use_height_weighted = !use_simple_mode &&
        (force_height_weighted || mf.gradient_enabled || (m_gradient_mode == 1 && mf.custom));

    if (use_height_weighted) {
        float h_a = 0.f;
        float h_b = 0.f;
        compute_gradient_heights(mf, m_height_lower_bound, m_height_upper_bound, h_a, h_b);
        const float cycle_h = std::max(0.01f, h_a + h_b);

        // Custom gradient curve (Photoshop-style) overrides the linear
        // [lower_bound, upper_bound] ramp. Sample the curve at the current
        // run-progress t in [0,1] -> ratio of component_a; the ratio of
        // component_b is (1 - r1). Empty curve -> fall through to the
        // existing linear blend derived from mix_b_percent.
        if (!mf.gradient_curve.empty()) {
            const float z_anchor = (layer_height > 1e-6f)
                ? std::max(0.f, layer_print_z - 0.5f * layer_height)
                : std::max(0.f, layer_print_z);
            float phase = std::fmod(z_anchor, cycle_h);
            if (phase < 0.f)
                phase += cycle_h;
            const double t_curve = double(phase) / double(cycle_h);
            const double r1 = std::clamp(sample_gradient_curve(mf.gradient_curve, t_curve),
                                         kGradientMinRatio, kGradientMaxRatio);
            const double r2 = 1.0 - r1;
            // Distribute cycle_h between A and B proportionally to the
            // sampled ratios, then quantize each side so the layer count
            // stays >= 1. This mirrors the integer-cadence path below.
            const double total = r1 + r2;
            const double h_a_eff = (total > 0.0) ? double(cycle_h) * r1 / total : 0.5 * double(cycle_h);
            const double h_b_eff = double(cycle_h) - h_a_eff;
            const double min_h = std::max(0.01, std::min(h_a_eff, h_b_eff));
            const int ratio_a_h = std::max(1, int(std::lround(h_a_eff / min_h)));
            const int ratio_b_h = std::max(1, int(std::lround(h_b_eff / min_h)));
            const int cycle_i = ratio_a_h + ratio_b_h;
            if (cycle_i > 0) {
                const int pos = ((layer_index % cycle_i) + cycle_i) % cycle_i;
                return (pos < ratio_a_h) ? mf.component_a : mf.component_b;
            }
            // cycle_i collapsed -> fall back to the Z-phase model with the
            // adjusted h_a_eff boundary.
            return (phase < float(h_a_eff)) ? mf.component_a : mf.component_b;
        }

        // When the layer height is comparable to or exceeds the cadence cycle,
        // the Z-phase approach degenerates (all layers land at the same phase).
        // Fall back to an integer layer cadence so the requested ratio remains
        // stable (for example, 50:50 alternates A/B instead of producing long
        // runs from an unrelated fixed-height wave).
        if (layer_height >= cycle_h - 1e-4f) {
            const float min_height = std::max(0.01f, std::min(h_a, h_b));
            const int ratio_a_h = std::max(1, int(std::lround(h_a / min_height)));
            const int ratio_b_h = std::max(1, int(std::lround(h_b / min_height)));
            const int cycle_i = ratio_a_h + ratio_b_h;
            if (cycle_i > 0) {
                const int pos = ((layer_index % cycle_i) + cycle_i) % cycle_i;
                return (pos < ratio_a_h) ? mf.component_a : mf.component_b;
            }
        }

        const float z_anchor = (layer_height > 1e-6f)
            ? std::max(0.f, layer_print_z - 0.5f * layer_height)
            : std::max(0.f, layer_print_z);
        float phase = std::fmod(z_anchor, cycle_h);
        if (phase < 0.f)
            phase += cycle_h;
        return (phase < h_a) ? mf.component_a : mf.component_b;
    }

    const int cycle = mf.ratio_a + mf.ratio_b;
    if (cycle <= 0)
        return mf.component_a;

    if (m_gradient_mode == 0 && m_advanced_dithering && mf.custom)
        return use_component_b_advanced_dither(layer_index, mf.ratio_a, mf.ratio_b) ? mf.component_b : mf.component_a;

    const int pos = ((layer_index % cycle) + cycle) % cycle; // safe modulo for negatives
    return (pos < mf.ratio_a) ? mf.component_a : mf.component_b;
}

unsigned int MixedFilamentManager::resolve_perimeter(unsigned int filament_id,
                                                     size_t       num_physical,
                                                     int          layer_index,
                                                     int          perimeter_index,
                                                     float        layer_print_z,
                                                     float        layer_height,
                                                     bool         force_height_weighted) const
{
    const int mixed_idx = mixed_index_from_filament_id(filament_id, num_physical);
    if (mixed_idx < 0)
        return filament_id;

    const MixedFilament &mf = m_mixed[size_t(mixed_idx)];
    if (!mf.is_available(num_physical))
        throw SlicingError(L("The mixed filament references a filament that no longer exists. Please reselect the original filament. Slicing will be available once the configuration is complete."));

    if (!mf.manual_pattern.empty()) {
        const std::vector<std::string> pattern_groups = split_manual_pattern_groups(mf.manual_pattern);
        if (!pattern_groups.empty()) {
            const size_t group_idx = size_t(std::max(0, perimeter_index));
            const std::string &group = pattern_groups[std::min(group_idx, pattern_groups.size() - 1)];
            if (!group.empty()) {
                const int pos = safe_mod(layer_index, int(group.size()));
                const unsigned int resolved = physical_filament_from_pattern_step(group[size_t(pos)], mf, num_physical);
                if (resolved >= 1 && resolved <= num_physical)
                    return resolved;
            }
        }
    }

    return resolve(filament_id, num_physical, layer_index, layer_print_z, layer_height, force_height_weighted);
}

std::vector<unsigned int> MixedFilamentManager::ordered_perimeter_extruders(unsigned int filament_id,
                                                                            size_t       num_physical,
                                                                            int          layer_index,
                                                                            float        layer_print_z,
                                                                            float        layer_height,
                                                                            bool         force_height_weighted) const
{
    std::vector<unsigned int> ordered;

    const int mixed_idx = mixed_index_from_filament_id(filament_id, num_physical);
    if (mixed_idx < 0) {
        ordered.emplace_back(filament_id);
        return ordered;
    }

    const MixedFilament &mf = m_mixed[size_t(mixed_idx)];
    if (!mf.is_available(num_physical))
        return ordered;

    if (!mf.manual_pattern.empty()) {
        const std::vector<std::string> pattern_groups = split_manual_pattern_groups(mf.manual_pattern);
        if (!pattern_groups.empty()) {
            ordered.reserve(pattern_groups.size());
            for (size_t group_idx = 0; group_idx < pattern_groups.size(); ++group_idx) {
                const unsigned int resolved = resolve_perimeter(filament_id,
                                                                num_physical,
                                                                layer_index,
                                                                int(group_idx),
                                                                layer_print_z,
                                                                layer_height,
                                                                force_height_weighted);
                if (resolved < 1 || resolved > num_physical)
                    continue;
                if (std::find(ordered.begin(), ordered.end(), resolved) == ordered.end())
                    ordered.emplace_back(resolved);
            }
            if (!ordered.empty())
                return ordered;
        }
    }

    ordered.emplace_back(resolve(filament_id, num_physical, layer_index, layer_print_z, layer_height, force_height_weighted));
    return ordered;
}

int MixedFilamentManager::mixed_index_from_filament_id(unsigned int filament_id, size_t num_physical) const
{
    if (filament_id <= num_physical)
        return -1;

    const size_t virtual_idx = size_t(filament_id - num_physical - 1);
    size_t visible_seen = 0;
    for (size_t i = 0; i < m_mixed.size(); ++i) {
        if (!m_mixed[i].occupies_virtual_slot())
            continue;
        if (visible_seen == virtual_idx)
            return int(i);
        ++visible_seen;
    }
    return -1;
}

const MixedFilament *MixedFilamentManager::mixed_filament_from_id(unsigned int filament_id, size_t num_physical) const
{
    const int idx = mixed_index_from_filament_id(filament_id, num_physical);
    return idx >= 0 ? &m_mixed[size_t(idx)] : nullptr;
}

ExpandedFilamentUsage MixedFilamentManager::expand_filament_usage(
    const std::vector<unsigned int> &filament_ids,
    size_t                           num_physical) const
{
    ExpandedFilamentUsage usage;
    std::unordered_set<unsigned int> physical_ids;
    std::unordered_set<unsigned int> invalid_ids;

    for (const unsigned int filament_id : filament_ids) {
        if (filament_id >= 1 && filament_id <= num_physical) {
            physical_ids.insert(filament_id);
            continue;
        }

        const MixedFilament *mixed = mixed_filament_from_id(filament_id, num_physical);
        if (mixed == nullptr) {
            invalid_ids.insert(filament_id);
            continue;
        }
        usage.has_mixed_filament = true;

        bool valid_row = true;
        bool added_component = false;
        std::unordered_set<unsigned int> row_physical_ids;
        const auto append_mixed_component = [&](unsigned int component_id) {
            if (component_id < 1 || component_id > num_physical) {
                valid_row = false;
                return;
            }
            row_physical_ids.insert(component_id);
            added_component = true;
        };

        if (!mixed->manual_pattern.empty()) {
            const std::string pattern = flatten_manual_pattern_groups(mixed->manual_pattern);
            if (pattern.empty()) {
                valid_row = false;
            } else {
                for (const char token : pattern)
                    append_mixed_component(physical_filament_from_pattern_step(token, *mixed, num_physical));
            }
        } else {
            // Keep the main branch's multi-digit component format when expanding
            // mixed filaments for nozzle mapping and compatibility checks.
            const std::vector<unsigned int> gradient_ids = decode_gradient_component_ids(
                mixed->gradient_component_ids, size_t(-1));
            if (gradient_component_ids_has_missing(mixed->gradient_component_ids))
                valid_row = false;

            const bool uses_multi_component_gradient =
                mixed->distribution_mode != int(MixedFilament::Simple) && gradient_ids.size() >= 3;
            if (uses_multi_component_gradient) {
                const std::vector<int> weights =
                    decode_gradient_component_weights(mixed->gradient_component_weights, gradient_ids.size());
                for (size_t component_idx = 0; component_idx < gradient_ids.size(); ++component_idx) {
                    if (!weights.empty() && weights[component_idx] <= 0)
                        continue;
                    append_mixed_component(gradient_ids[component_idx]);
                }
            } else {
                if (mixed->ratio_a > 0)
                    append_mixed_component(mixed->component_a);
                if (mixed->ratio_b > 0)
                    append_mixed_component(mixed->component_b);
            }
        }

        if (!valid_row || !added_component) {
            invalid_ids.insert(filament_id);
        } else {
            physical_ids.insert(row_physical_ids.begin(), row_physical_ids.end());
        }
    }

    usage.physical_filament_ids.assign(physical_ids.begin(), physical_ids.end());
    usage.invalid_filament_ids.assign(invalid_ids.begin(), invalid_ids.end());
    std::sort(usage.physical_filament_ids.begin(), usage.physical_filament_ids.end());
    std::sort(usage.invalid_filament_ids.begin(), usage.invalid_filament_ids.end());
    return usage;
}

// Blend N colours using weighted pairwise FilamentMixer blending.
std::string MixedFilamentManager::blend_color_multi(
    const std::vector<std::pair<std::string, int>> &color_percents)
{
    if (color_percents.empty())
        return "#000000";
    if (color_percents.size() == 1)
        return color_percents.front().first;

    struct WeightedColor {
        RGB color;
        int pct;
    };
    std::vector<WeightedColor> colors;
    colors.reserve(color_percents.size());

    int total_pct = 0;
    for (const auto &[hex, pct] : color_percents) {
        if (pct <= 0)
            continue;
        colors.push_back({parse_hex_color(hex), pct});
        total_pct += pct;
    }
    if (colors.empty() || total_pct <= 0)
        return "#000000";

    unsigned char r = static_cast<unsigned char>(colors.front().color.r);
    unsigned char g = static_cast<unsigned char>(colors.front().color.g);
    unsigned char b = static_cast<unsigned char>(colors.front().color.b);
    int accumulated_pct = colors.front().pct;

    for (size_t i = 1; i < colors.size(); ++i) {
        const auto &next = colors[i];
        const int new_total = accumulated_pct + next.pct;
        if (new_total <= 0)
            continue;
        const float t = static_cast<float>(next.pct) / static_cast<float>(new_total);
        filament_mixer_lerp(
            r, g, b,
            static_cast<unsigned char>(next.color.r),
            static_cast<unsigned char>(next.color.g),
            static_cast<unsigned char>(next.color.b),
            t, &r, &g, &b);
        accumulated_pct = new_total;
    }

    return rgb_to_hex({int(r), int(g), int(b)});
}

std::string MixedFilamentManager::blend_color(const std::string &color_a,
                                              const std::string &color_b,
                                              int ratio_a, int ratio_b)
{
    const int safe_a = std::max(0, ratio_a);
    const int safe_b = std::max(0, ratio_b);
    const int total  = safe_a + safe_b;
    const float t    = (total > 0) ? (static_cast<float>(safe_b) / static_cast<float>(total)) : 0.5f;

    const RGB rgb_a = parse_hex_color(color_a);
    const RGB rgb_b = parse_hex_color(color_b);

    unsigned char out_r = static_cast<unsigned char>(rgb_a.r);
    unsigned char out_g = static_cast<unsigned char>(rgb_a.g);
    unsigned char out_b = static_cast<unsigned char>(rgb_a.b);
    filament_mixer_lerp(static_cast<unsigned char>(rgb_a.r),
                        static_cast<unsigned char>(rgb_a.g),
                        static_cast<unsigned char>(rgb_a.b),
                        static_cast<unsigned char>(rgb_b.r),
                        static_cast<unsigned char>(rgb_b.g),
                        static_cast<unsigned char>(rgb_b.b),
                        t, &out_r, &out_g, &out_b);

    return rgb_to_hex({int(out_r), int(out_g), int(out_b)});
}

void MixedFilamentManager::refresh_display_colors(const std::vector<std::string> &filament_colours)
{
    for (MixedFilament &mf : m_mixed) {
        const std::string normalized_pattern = normalize_manual_pattern(mf.manual_pattern);
        if (!normalized_pattern.empty()) {
            const std::string flattened_pattern = flatten_manual_pattern_groups(normalized_pattern);
            if (!flattened_pattern.empty()) {
                std::vector<int> counts(filament_colours.size(), 0);
                for (const char token : flattened_pattern) {
                    const unsigned int resolved = physical_filament_from_pattern_step(token, mf, filament_colours.size());
                    if (resolved >= 1 && resolved <= filament_colours.size())
                        ++counts[resolved - 1];
                }

                std::vector<std::pair<std::string, int>> color_percents;
                color_percents.reserve(filament_colours.size());
                for (size_t i = 0; i < counts.size(); ++i) {
                    const int wi = std::max(0, counts[i]);
                    if (wi == 0)
                        continue;
                    color_percents.emplace_back(filament_colours[i], wi);
                }

                mf.mix_b_percent = mix_percent_from_normalized_pattern(normalized_pattern);
                mf.display_color = color_percents.empty() ? "#26A69A" : blend_color_multi(color_percents);
                continue;
            }
        }

        const std::vector<unsigned int> gradient_ids = decode_gradient_component_ids(mf.gradient_component_ids, filament_colours.size());
        if (mf.distribution_mode != int(MixedFilament::Simple) && gradient_ids.size() >= 3) {
            const std::vector<int> gradient_weights =
                decode_gradient_component_weights(mf.gradient_component_weights, gradient_ids.size());
            const std::vector<unsigned int> gradient_sequence =
                build_weighted_gradient_sequence(gradient_ids,
                    gradient_weights.empty() ? std::vector<int>(gradient_ids.size(), 1) : gradient_weights);
            if (gradient_sequence.empty()) {
                mf.display_color = "#26A69A";
                continue;
            }

            std::vector<int> counts(gradient_ids.size(), 0);
            for (const unsigned int id : gradient_sequence) {
                auto it = std::find(gradient_ids.begin(), gradient_ids.end(), id);
                if (it != gradient_ids.end())
                    ++counts[size_t(it - gradient_ids.begin())];
            }
            std::vector<std::pair<std::string, int>> color_percents;
            color_percents.reserve(gradient_ids.size());
            for (size_t i = 0; i < gradient_ids.size(); ++i) {
                const int wi = std::max(0, counts[i]);
                if (wi == 0)
                    continue;
                color_percents.emplace_back(filament_colours[gradient_ids[i] - 1], wi);
            }
            mf.display_color = blend_color_multi(color_percents);
            continue;
        }
        if (mf.component_a == 0 || mf.component_b == 0 ||
            mf.component_a > filament_colours.size() || mf.component_b > filament_colours.size()) {
            mf.display_color = "#26A69A";
            continue;
        }
        const int ratio_a = std::max(0, 100 - clamp_int(mf.mix_b_percent, 0, 100));
        const int ratio_b = clamp_int(mf.mix_b_percent, 0, 100);
        mf.display_color = blend_color(
            filament_colours[mf.component_a - 1],
            filament_colours[mf.component_b - 1],
            ratio_a, ratio_b);
    }
}

size_t MixedFilamentManager::enabled_count() const
{
    size_t count = 0;
    for (const auto &mf : m_mixed)
        if (mf.enabled && !mf.deleted && mf.component_a != 0 && mf.component_b != 0 &&
            mf.component_a != mf.component_b && !gradient_component_ids_has_missing(mf.gradient_component_ids))
            ++count;
    return count;
}

size_t MixedFilamentManager::virtual_count() const
{
    return size_t(std::count_if(m_mixed.begin(), m_mixed.end(), [](const MixedFilament &mf) {
        return mf.occupies_virtual_slot();
    }));
}

std::vector<std::string> MixedFilamentManager::display_colors() const
{
    std::vector<std::string> colors;
    colors.reserve(virtual_count());
    for (const auto &mf : m_mixed)
        if (mf.occupies_virtual_slot())
            colors.push_back(mf.display_color);
    return colors;
}

} // namespace Slic3r
