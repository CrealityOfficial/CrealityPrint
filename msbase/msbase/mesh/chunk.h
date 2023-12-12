#ifndef MSBASE_CHUNK_1695188680764_H
#define MSBASE_CHUNK_1695188680764_H
#include "msbase/interface.h"
#include "trimesh2/TriMesh.h"

namespace msbase
{
	MSBASE_API bool generateChunk(
		trimesh::TriMesh* mesh
		,const std::vector<trimesh::ivec3>& neigbs
		,const float max_area_per_chunk
		,std::vector<int>& faceChunkIDs
		,std::vector<std::vector<int>>& chunkFaces);
}

#endif // MSBASE_CHUNK_1695188680764_H