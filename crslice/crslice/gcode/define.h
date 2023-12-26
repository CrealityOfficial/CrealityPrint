#pragma once

#ifndef GCODE_DEFINE_HPP_
#define GCODE_DEFINE_HPP_

//#include <QString>
#include "trimesh2/XForm.h"

#define USE_GCODE_PREVIEW_SIMPLE 1

#define SIMPLE_GCODE_IMPL 3    // 0 cpu tri soup  1 cpu indices 2 gpu tri soup 3 gpu indices

#define INDEX_START_AT_ONE 1

#ifndef  INDEX_START_AT_ONE
#define INDEX_START_AT_ONE 0
#endif // ! INDEX_START_AT_ONE

namespace gcode {



}  // namespace gcode

#endif  // GCODE_DEFINE_HPP_
