#include "FilamentChangeTopology.hpp"

#include <algorithm>

namespace Slic3r
{

bool FilamentChangeTopology::has_action(FilamentChangeActionType type) const
{
    return std::any_of(actions.begin(), actions.end(), [type](const FilamentChangeAction& action) {
        return action.type == type;
    });
}

PhysicalNozzleStateRecorder::PhysicalNozzleStateRecorder(size_t physical_nozzle_count)
    : m_resident_filament_ids(physical_nozzle_count, -1)
{}

bool PhysicalNozzleStateRecorder::valid_nozzle(int physical_nozzle_id) const
{
    return physical_nozzle_id >= 0 && static_cast<size_t>(physical_nozzle_id) < m_resident_filament_ids.size();
}

int PhysicalNozzleStateRecorder::resident_filament(int physical_nozzle_id) const
{
    return valid_nozzle(physical_nozzle_id) ? m_resident_filament_ids[physical_nozzle_id] : -1;
}

bool PhysicalNozzleStateRecorder::seed(int physical_nozzle_id, int filament_id)
{
    if (!valid_nozzle(physical_nozzle_id) || filament_id < 0)
        return false;

    m_resident_filament_ids[physical_nozzle_id] = filament_id;
    return true;
}

std::optional<FilamentChangeTopology> PhysicalNozzleStateRecorder::evaluate(
    int next_filament_id,
    int previous_physical_nozzle_id,
    int next_physical_nozzle_id) const
{
    if (next_filament_id < 0 || !valid_nozzle(next_physical_nozzle_id))
        return std::nullopt;
    if (previous_physical_nozzle_id != -1 && !valid_nozzle(previous_physical_nozzle_id))
        return std::nullopt;

    FilamentChangeTopology topology;
    topology.next_filament_id            = next_filament_id;
    topology.next_physical_nozzle_id     = next_physical_nozzle_id;
    topology.target_resident_filament_id = resident_filament(next_physical_nozzle_id);

    const bool physical_nozzle_changed = previous_physical_nozzle_id >= 0 &&
                                         previous_physical_nozzle_id != next_physical_nozzle_id;
    topology.target_residency_known = topology.target_resident_filament_id >= 0;
    topology.requires_initialization_flush = !topology.target_residency_known;
    const bool requires_material_replacement = !topology.target_residency_known ||
                                               topology.target_resident_filament_id != next_filament_id;

    if (physical_nozzle_changed)
        topology.actions.push_back({FilamentChangeActionType::CrossNozzleSwitch});

    if (topology.requires_initialization_flush)
        topology.actions.push_back({FilamentChangeActionType::InitializeTargetNozzle});
    else if (requires_material_replacement)
        topology.actions.push_back({FilamentChangeActionType::SameNozzleSwitch});

    return topology;
}

bool PhysicalNozzleStateRecorder::commit(const FilamentChangeTopology& topology)
{
    return seed(topology.next_physical_nozzle_id, topology.next_filament_id);
}

} // namespace Slic3r
