#ifndef slic3r_Format_CUBE_hpp_
#define slic3r_Format_CUBE_hpp_
#include <functional>
#include "trimesh2/TriMesh.h"
#include "../interface.hpp"


namespace cadcore {


CADCORE_API trimesh::TriMesh * create_cube(double l, double w, int h);



}; // namespace Slic3r

#endif /* slic3r_Format_STEP_hpp_ */
