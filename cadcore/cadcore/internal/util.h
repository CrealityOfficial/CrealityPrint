#ifndef CADCORE_SPHERE_UTIL_1688716453388_H
#define CADCORE_SPHERE_UTIL_1688716453388_H
#include "TopoDS_Shape.hxx"
#include "trimesh2/TriMesh.h"

namespace cadcore
{
	trimesh::TriMesh* Shape_Triangulation(const TopoDS_Shape& shape, Standard_Real deflection = 1e-3);
}

#endif // CADCORE_SPHERE_UTIL_1688716453388_H