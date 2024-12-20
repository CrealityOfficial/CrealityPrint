#ifndef MSBASE_MERGE_1695441390758_H
#define MSBASE_MERGE_1695441390758_H
#include "msbase/interface.h"
#include "trimesh2/TriMesh.h"
#include "ccglobal/tracer.h"
#include <memory>

namespace msbase
{
	typedef std::shared_ptr<trimesh::TriMesh> BaseMeshPtr;

	MSBASE_API trimesh::TriMesh* mergeMeshes(const std::vector<trimesh::TriMesh*>& ins);
	MSBASE_API trimesh::TriMesh* mergeMeshes(const std::vector<BaseMeshPtr>& ins);

	MSBASE_API trimesh::TriMesh* partMesh(const std::vector<int>& indices, trimesh::TriMesh* inMesh);

	MSBASE_API std::vector<std::vector<BaseMeshPtr>> meshSplit(const std::vector<BaseMeshPtr>& meshes
		, ccglobal::Tracer* tracer = nullptr);

	MSBASE_API void copyTrimesh2Trimesh(trimesh::TriMesh* source, trimesh::TriMesh* dest);
}

#endif // MSBASE_MERGE_1695441390758_H