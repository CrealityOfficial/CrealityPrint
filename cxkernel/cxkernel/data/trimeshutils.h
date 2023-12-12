#ifndef CXKERNEL_TRIMESH_2_GEOMETRY_RENDERER_H
#define CXKERNEL_TRIMESH_2_GEOMETRY_RENDERER_H
#include "cxkernel/cxkernelinterface.h"
#include "cxkernel/data/attribute.h"
#include "cxkernel/data/header.h"

namespace cxkernel
{
	CXKERNEL_API void generateGeometryColorDataFromMesh(trimesh::TriMesh* mesh, cxkernel::GeometryData& data);
	CXKERNEL_API void generateGeometryDataFromMesh(trimesh::TriMesh* mesh, cxkernel::GeometryData& data);
	CXKERNEL_API void generateIndexGeometryDataFromMesh(trimesh::TriMesh* mesh, cxkernel::GeometryData& data);

	CXKERNEL_API TriMeshPtr loadMeshFromName(const QString& fileName, ccglobal::Tracer* tracer);
	CXKERNEL_API void saveMesh(trimesh::TriMesh* mesh, const QString& fileName);
}

#endif // Q_UTIL_TRIMESH_2_GEOMETRY_RENDERER_H