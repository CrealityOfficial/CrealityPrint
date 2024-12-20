//Copyright (c) 2020 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef PROGRESS_PROGRESS_ESTIMATOR_LINEAR_H
#define PROGRESS_PROGRESS_ESTIMATOR_LINEAR_H

#include <vector>

#include "progress/ProgressEstimator.h"

namespace cura52
{
    class ProgressEstimatorLinear : public ProgressEstimator
    {
        unsigned int total_steps;
    public:
        ProgressEstimatorLinear(unsigned int total_steps)
            : total_steps(total_steps)
        {
        }
        double progress(int current_step)
        {
            return double(current_step) / double(total_steps);
        }
    };

} // namespace cura52

#endif // PROGRESS_PROGRESS_ESTIMATOR_LINEAR_H