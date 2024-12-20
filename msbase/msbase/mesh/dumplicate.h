#ifndef MMESH_MNODE_DUMPLICATE_1622032440408_H
#define MMESH_MNODE_DUMPLICATE_1622032440408_H
#include "msbase/interface.h"
#include "trimesh2/TriMesh.h"
#include "ccglobal/tracer.h"

namespace msbase
{
	MSBASE_API bool dumplicateMesh(trimesh::TriMesh* mesh, ccglobal::Tracer* tracer = nullptr);

	MSBASE_API bool mergeNearPoints(trimesh::TriMesh* mesh, ccglobal::Tracer* tracer = nullptr, float eps = 1E-8F);
}

#endif // MMESH_MNODE_DUMPLICATE_1622032440408_H