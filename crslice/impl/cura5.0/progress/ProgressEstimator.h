//Copyright (c) 2020 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef PROGRESS_PROGRESS_ESTIMATOR_H
#define PROGRESS_PROGRESS_ESTIMATOR_H

#include <vector>

namespace cura52
{
    /*
     * ProgressEstimator is a finger-tree with ProgressEstimatorLinear as leaves.
     *
     * Each (non-leaf) node consists of a ProgressStageEstimator which consists of several stages.
     *
     * The structure of this tree is an oversimplification of the call graph of CuraEngine.
     *
     */

    class ProgressEstimator
    {
    public:
        virtual double progress(int current_step) = 0;
        virtual ~ProgressEstimator()
        {
        }
    };

} // namespace cura52

#endif // PROGRESS_PROGRESS_ESTIMATOR_H