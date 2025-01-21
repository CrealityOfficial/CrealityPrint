#ifndef _FDM_FILAMENT_HPP
#define _FDM_FILAMENT_HPP
#include "libslic3r/PrintConfig.hpp"

namespace Slic3r
{
    enum FilamentTempType {
        HighTemp=0,
        LowTemp,
        HighLowCompatible,
        Undefine
    };
}

namespace creality
{
    class Filament
    {
    public:
        static Slic3r::FilamentTempType get_filament_temp_type(const std::string& filament_type);
        static int get_hrc_by_nozzle_type(const Slic3r::NozzleType& type);
        static bool check_multi_filaments_compatibility(const std::vector<std::string>& filament_types);
        // similar to check_multi_filaments_compatibility, but the input is int, and may be negative (means unset)
        static bool is_filaments_compatible(const std::vector<int>& types);
        // get the compatible filament type of a multi-material object
        // Rule:
        // 1. LowTemp+HighLowCompatible=LowTemp
        // 2. HighTemp++HighLowCompatible=HighTemp
        // 3. LowTemp+HighTemp+...=HighLowCompatible
        // Unset types are just ignored.
        static int get_compatible_filament_type(const std::set<int>& types);
    };
}
#endif // _FDM_FILAMENT_HPP