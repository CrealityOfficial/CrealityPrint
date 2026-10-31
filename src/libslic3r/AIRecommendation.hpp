#ifndef slic3r_AIRecommendation_hpp_
#define slic3r_AIRecommendation_hpp_

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include "Config.hpp"
#include <string>
#include <vector>

namespace Slic3r {

// Runtime attribution only. Serialized by ModelObject for undo, never as a print option.
struct AIRecommendationValue {
    std::string value;
    int type = 0;
    std::string recommend_id;
    uint64_t apply_seq = 0;
    template<class Archive> void serialize(Archive& ar) { ar(value, type, recommend_id, apply_seq); }
};

struct AIRecommendationBaseline {
    std::map<std::string, AIRecommendationValue> values;
    std::vector<size_t> volume_ids;
    bool incomplete = false;
    void accept(const std::string& key, const AIRecommendationValue& value, const std::vector<size_t>& topology) {
        if (values.empty()) volume_ids = topology;
        values[key] = value;
    }
    // Preserve an honest usage marker when a topology operation cannot map per-object counts.
    void inherit_unknown(const AIRecommendationBaseline& source) {
        if (source.values.empty()) return;
        incomplete = true;
        for (const auto& [key, value] : source.values) {
            auto it = values.find(key);
            if (it == values.end() || it->second.apply_seq < value.apply_seq) values[key] = value;
        }
    }
    template<class Archive> void serialize(Archive& ar) { ar(values, volume_ids, incomplete); }
};

inline uint64_t next_ai_recommendation_sequence()
{
    static std::atomic<uint64_t> sequence{0};
    return ++sequence;
}

struct AIRecommendationMetrics {
    std::string recommend_id;
    uint64_t apply_seq = 0;
    int total_count = 0;
    int matched_count = 0;
    bool unknown = false;

    void observe(const AIRecommendationValue& baseline, bool comparable, bool matches) {
        if (baseline.apply_seq >= apply_seq) {
            apply_seq = baseline.apply_seq;
            recommend_id = baseline.recommend_id;
        }
        ++total_count;
        if (!comparable) unknown = true;
        else if (matches) ++matched_count;
    }
    bool valid() const { return !unknown && total_count > 0 && !recommend_id.empty(); }
    const char* status() const { return unknown ? "unknown" : valid() ? "valid" : "not_used"; }
};

enum class AIRecommendationComparison { Unknown, Different, Equal };

// Reuse the slicer's option parser and equality rules, including vector and enum
// types. The stored baseline is already canonical ConfigOption::serialize() output.
inline AIRecommendationComparison compare_ai_recommendation_option(
    const AIRecommendationValue& baseline, const ConfigOption* actual)
{
    if (!actual || int(actual->type()) != baseline.type) return AIRecommendationComparison::Unknown;
    try {
        std::unique_ptr<ConfigOption> expected(actual->clone());
        if (!expected->deserialize(baseline.value) || expected->serialize() != baseline.value)
            return AIRecommendationComparison::Unknown;
        if (actual->is_vector()) {
            const auto& a = static_cast<const ConfigOptionVectorBase&>(*actual);
            const auto& b = static_cast<const ConfigOptionVectorBase&>(*expected);
            if (a.size() == 0 || a.size() != b.size()) return AIRecommendationComparison::Unknown;
            for (size_t i = 0; i < a.size(); ++i)
                if (a.is_nil(i) || b.is_nil(i)) return AIRecommendationComparison::Unknown;
        } else if (actual->is_nil() || expected->is_nil()) {
            return AIRecommendationComparison::Unknown;
        }
        // The baseline was serialized at apply time. Normalize the actual value
        // through that same representation before using the native equality rule.
        std::unique_ptr<ConfigOption> normalized_actual(actual->clone());
        if (!normalized_actual->deserialize(actual->serialize())) return AIRecommendationComparison::Unknown;
        return *expected == *normalized_actual ? AIRecommendationComparison::Equal : AIRecommendationComparison::Different;
    } catch (...) { return AIRecommendationComparison::Unknown; }
}

} // namespace Slic3r
#endif
