#include "NoWipeTowerPrintStart.hpp"

#include "../ExtrusionEntityCollection.hpp"
#include "../ShortestPath.hpp"

#include <algorithm>
#include <cmath>

namespace Slic3r {
namespace {

bool eligible_print_start_path(const ExtrusionPath& path)
{
    return path.role() == erInternalInfill &&
        dynamic_cast<const ExtrusionPathSloped*>(&path) == nullptr &&
        !path.has_path3() && !path.is_force_no_extrusion() &&
        path.zaa_path_policy() == ZaaPathPolicy::Conventional &&
        path.polyline.size() >= 2 && path.length() > EPSILON &&
        path.mm3_per_mm > 0. && path.width > 0.f && path.height > 0.f;
}

template<class Visitor>
void visit_legal_endpoints(const ExtrusionEntity& entity, std::vector<size_t>& indices, Visitor& visitor,
    bool locked = false)
{
    if (const auto* collection = dynamic_cast<const ExtrusionEntityCollection*>(&entity)) {
        locked = locked || !collection->can_sort();
        const size_t count = !locked ? collection->entities.size() :
            std::min(size_t(1), collection->entities.size());
        for (size_t i = 0; i < count; ++i) {
            indices.push_back(i);
            visit_legal_endpoints(*collection->entities[i], indices, visitor, locked);
            indices.pop_back();
        }
    } else if (const auto* path = dynamic_cast<const ExtrusionPath*>(&entity)) {
        if (!eligible_print_start_path(*path))
            return;
        visitor(*path, indices, false);
        if (!locked && path->can_reverse() && path->last_point() != path->first_point())
            visitor(*path, indices, true);
    }
}

bool validate_candidate(const ExtrusionEntity& root, const NoWipeTowerPrintStartCandidate& candidate)
{
    const ExtrusionEntity* entity = &root;
    bool locked = false;
    for (size_t child_index : candidate.child_indices) {
        const auto* collection = dynamic_cast<const ExtrusionEntityCollection*>(entity);
        if (collection == nullptr || child_index >= collection->entities.size())
            return false;
        locked = locked || !collection->can_sort();
        if (locked && child_index != 0)
            return false;
        entity = collection->entities[child_index];
    }
    const auto* path = dynamic_cast<const ExtrusionPath*>(entity);
    return path != nullptr && eligible_print_start_path(*path) &&
        (!candidate.reverse || (!locked && path->can_reverse())) &&
        candidate.start == (candidate.reverse ? path->last_point() : path->first_point());
}

std::unique_ptr<ExtrusionEntity> clone_preserving_metadata(const ExtrusionEntity& source)
{
    std::unique_ptr<ExtrusionEntity> result;
    if (const auto* collection = dynamic_cast<const ExtrusionEntityCollection*>(&source)) {
        auto clone = std::make_unique<ExtrusionEntityCollection>();
        clone->no_sort = collection->no_sort;
        clone->loop_node_range = collection->loop_node_range;
        if (!collection->can_reverse())
            clone->set_reverse();
        clone->entities.reserve(collection->entities.size());
        for (const ExtrusionEntity* child : collection->entities) {
            std::unique_ptr<ExtrusionEntity> child_clone = clone_preserving_metadata(*child);
            clone->entities.push_back(child_clone.release());
        }
        result = std::move(clone);
    } else if (const auto* sloped = dynamic_cast<const ExtrusionPathSloped*>(&source)) {
        // ExtrusionPathSloped inherits the base clone(), which would slice away
        // its slope metadata. It is never a candidate, but can be a sibling.
        result = std::make_unique<ExtrusionPathSloped>(*sloped);
    } else if (const auto* sloped_loop = dynamic_cast<const ExtrusionLoopSloped*>(&source)) {
        result = std::make_unique<ExtrusionLoopSloped>(*sloped_loop);
    } else {
        result.reset(source.clone());
    }
    // Several existing path copy constructors do not copy base-class fields.
    result->inset_idx = source.inset_idx;
    result->set_cooling_node(source.get_cooling_node());
    const auto copy_path_metadata = [](const ExtrusionPath& original, ExtrusionPath& copied) {
        copied.inset_idx = original.inset_idx;
        copied.set_cooling_node(original.get_cooling_node());
    };
    if (const auto* multipath = dynamic_cast<const ExtrusionMultiPath*>(&source)) {
        auto* clone = static_cast<ExtrusionMultiPath*>(result.get());
        for (size_t i = 0; i < multipath->paths.size(); ++i)
            copy_path_metadata(multipath->paths[i], clone->paths[i]);
    } else if (const auto* loop = dynamic_cast<const ExtrusionLoop*>(&source)) {
        auto* clone = static_cast<ExtrusionLoop*>(result.get());
        for (size_t i = 0; i < loop->paths.size(); ++i)
            copy_path_metadata(loop->paths[i], clone->paths[i]);
        if (const auto* sloped = dynamic_cast<const ExtrusionLoopSloped*>(loop)) {
            auto* sloped_clone = static_cast<ExtrusionLoopSloped*>(clone);
            for (size_t i = 0; i < sloped->starts.size(); ++i)
                copy_path_metadata(sloped->starts[i], sloped_clone->starts[i]);
            for (size_t i = 0; i < sloped->ends.size(); ++i)
                copy_path_metadata(sloped->ends[i], sloped_clone->ends[i]);
        }
    }
    return result;
}

void lock_output_order(ExtrusionEntity& entity)
{
    entity.set_reverse();
    if (auto* collection = dynamic_cast<ExtrusionEntityCollection*>(&entity)) {
        collection->no_sort = true;
        for (ExtrusionEntity* child : collection->entities)
            lock_output_order(*child);
    }
}

} // namespace

std::vector<NoWipeTowerPrintStartCandidate> no_wipe_tower_print_start_candidates(
    const ExtrusionEntity& selected_island, size_t max_candidates)
{
    std::vector<NoWipeTowerPrintStartCandidate> result;
    if (max_candidates == 0)
        return result;

    // Sample across the whole island rather than inspecting only its first few
    // paths, whose endpoints may all lie in the same small neighbourhood.
    std::vector<size_t> indices;
    size_t total = 0;
    auto count = [&](const ExtrusionPath&, const std::vector<size_t>&, bool) { ++total; };
    visit_legal_endpoints(selected_island, indices, count);
    const size_t wanted = std::min(total, max_candidates);
    if (wanted == 0)
        return result;
    result.reserve(wanted);
    size_t rank = 0;
    auto collect = [&](const ExtrusionPath& path, const std::vector<size_t>& child_indices, bool reverse) {
        const size_t next_rank = wanted == 1 ? 0 : static_cast<size_t>(
            static_cast<long double>(result.size()) * (total - 1) / (wanted - 1));
        if (result.size() < wanted && rank == next_rank)
            result.push_back({ reverse ? path.last_point() : path.first_point(), child_indices, reverse });
        ++rank;
    };
    visit_legal_endpoints(selected_island, indices, collect);
    return result;
}

std::unique_ptr<ExtrusionEntity> no_wipe_tower_print_start_clone(
    const ExtrusionEntity& selected_island, const NoWipeTowerPrintStartCandidate& candidate)
{
    if (!validate_candidate(selected_island, candidate))
        return nullptr;

    std::unique_ptr<ExtrusionEntity> output = clone_preserving_metadata(selected_island);
    ExtrusionEntity* entity = output.get();
    for (size_t child_index : candidate.child_indices) {
        auto* collection = static_cast<ExtrusionEntityCollection*>(entity);
        ExtrusionEntity* selected = collection->entities[child_index];
        // A stable move-to-front keeps every other sibling's existing order.
        collection->entities.erase(collection->entities.begin() + child_index);
        collection->entities.insert(collection->entities.begin(), selected);
        entity = selected;
    }
    if (candidate.reverse)
        entity->reverse();
    lock_output_order(*output);
    return output;
}

std::unique_ptr<ExtrusionEntity> no_wipe_tower_print_start_baseline(
    const ExtrusionEntity& selected_island, const Point& reference, bool reverse)
{
    if (reverse && !selected_island.can_reverse())
        return nullptr;
    std::unique_ptr<ExtrusionEntity> output = clone_preserving_metadata(selected_island);
    if (reverse)
        output->reverse();
    if (auto* collection = dynamic_cast<ExtrusionEntityCollection*>(output.get())) {
        if (collection->can_sort())
            chain_and_reorder_extrusion_entities(collection->entities, &reference);
    }
    return output;
}

std::unique_ptr<ExtrusionEntity> no_wipe_tower_print_start_baseline(
    const ExtrusionEntitiesPtr& selected_region, const Point& reference)
{
    if (selected_region.empty())
        return nullptr;
    // The chain routine only needs a mutable pointer vector; it does not own
    // or modify the original entities. Direction changes apply to copies below.
    ExtrusionEntitiesPtr sources = selected_region;
    const auto chain = chain_extrusion_entities(sources, &reference);
    auto output = std::make_unique<ExtrusionEntityCollection>();
    output->loop_node_range = { 0, 0 };
    output->entities.reserve(chain.size());
    for (const auto& item : chain) {
        auto child = no_wipe_tower_print_start_baseline(*sources[item.first], reference, item.second);
        if (!child)
            return nullptr;
        output->entities.push_back(child.release());
    }
    return output;
}

void NoWipeTowerPrintStartHistory::advance(size_t layer_index)
{
    if (m_initialized && layer_index < m_layer_index)
        m_entries.clear();
    m_layer_index = layer_index;
    m_initialized = true;
    m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(), [&](const Entry& entry) {
        return entry.layer_index > layer_index || layer_index - entry.layer_index > 3;
    }), m_entries.end());
}

void NoWipeTowerPrintStartHistory::remember(const Vec2d& world_mm)
{
    if (!m_initialized)
        advance(0);
    m_entries.push_back({ m_layer_index, world_mm });
}

bool NoWipeTowerPrintStartHistory::separated(const Vec2d& world_mm, double min_distance_mm) const
{
    const double radius_squared = std::max(0., min_distance_mm) * std::max(0., min_distance_mm);
    return std::none_of(m_entries.begin(), m_entries.end(), [&](const Entry& entry) {
        return (entry.world_mm - world_mm).squaredNorm() < radius_squared;
    });
}

} // namespace Slic3r
