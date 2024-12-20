#ifndef DEBUG_CONCAVE_NESTPLACER_H
#define DEBUG_CONCAVE_NESTPLACER_H
#include "nestplacer/concaveplacer.h"
#include "libnest2d/libnest2d.hpp"

namespace nestplacer
{
    typedef libnest2d::NestConfig<libnest2d::NfpPlacer, libnest2d::FirstFitSelection> NfpFisrtFitConfig;

    void initDebugger(NfpFisrtFitConfig& config, ConcaveNestDebugger* debugger);
}
#endif