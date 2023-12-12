#ifndef cadcore_slic3r_Format_STEP_hpp_
#define cadcore_slic3r_Format_STEP_hpp_
#include <functional>
#include "trimesh2/TriMesh.h"
#include "cadcore/interface.hpp"
#include "ccglobal/tracer.h"

namespace cadcore {

CADCORE_API trimesh::TriMesh *load_step(const std::string& fileName,  ccglobal::Tracer* tracer);

}; // namespace cadcore

#endif /* slic3r_Format_STEP_hpp_ */
