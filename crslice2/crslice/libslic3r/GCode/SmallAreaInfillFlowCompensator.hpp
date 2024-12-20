#ifndef slic3r_GCode_SmallAreaInfillFlowCompensator_hpp_
#define slic3r_GCode_SmallAreaInfillFlowCompensator_hpp_

#include "../libslic3r.h"
#include "../PrintConfig.hpp"
#include "../ExtrusionEntity.hpp"
#include "spline/spline.h"

namespace Slic3r {

class SmallAreaInfillFlowCompensator
{
public:
    SmallAreaInfillFlowCompensator() = delete;
    explicit SmallAreaInfillFlowCompensator(const Slic3r::GCodeConfig& config);
    ~SmallAreaInfillFlowCompensator() = default;

    double modify_flow(const double line_length, const double dE, const ExtrusionRole role);

private:
    // Model points
    std::vector<double> eLengths;
    std::vector<double> flowComps;

    // TODO: Cubic Spline
    tk::spline flowModel;

    double flow_comp_model(const double line_length);

    double max_modified_length() { return eLengths.back(); }
};

} // namespace Slic3r

#endif /* slic3r_GCode_SmallAreaInfillFlowCompensator_hpp_ */
