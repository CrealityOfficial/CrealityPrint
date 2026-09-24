#ifndef slic3r_FilamentChangeTopology_hpp_
#define slic3r_FilamentChangeTopology_hpp_

#include <cstddef>
#include <optional>
#include <vector>

namespace Slic3r
{

enum class FilamentChangeActionType : unsigned char
{
    CrossNozzleSwitch,
    SameNozzleSwitch,
    InitializeTargetNozzle
};

struct FilamentChangeAction
{
    FilamentChangeActionType type;
};

struct FilamentChangeTopology
{
    int next_filament_id            {-1};
    int next_physical_nozzle_id     {-1};
    int target_resident_filament_id {-1};

    bool target_residency_known        {false};
    bool requires_initialization_flush {false};

    std::vector<FilamentChangeAction> actions;

    bool has_action(FilamentChangeActionType type) const;
};

class PhysicalNozzleStateRecorder
{
public:
    explicit PhysicalNozzleStateRecorder(size_t physical_nozzle_count);

    int  resident_filament(int physical_nozzle_id) const;
    bool seed(int physical_nozzle_id, int filament_id);

    std::optional<FilamentChangeTopology> evaluate(
        int next_filament_id,
        int previous_physical_nozzle_id,
        int next_physical_nozzle_id) const;

    bool commit(const FilamentChangeTopology& topology);

private:
    bool valid_nozzle(int physical_nozzle_id) const;

    std::vector<int> m_resident_filament_ids;
};

} // namespace Slic3r

#endif // slic3r_FilamentChangeTopology_hpp_
