#ifndef MSBASE_GET_1695188680764_H
#define MSBASE_GET_1695188680764_H
#include "msbase/interface.h"
#include "trimesh2/TriMesh.h"

namespace msbase
{
	MSBASE_API trimesh::vec3 getFaceNormal(trimesh::TriMesh* mesh, int faceIndex, bool normalized = true);

	MSBASE_API void calculateFaceNormalOrAreas(trimesh::TriMesh* mesh, 
		/*out*/std::vector<trimesh::vec3>& normals, std::vector<float>* areas = nullptr);

	//get one facet area
	MSBASE_API double getFacetArea(trimesh::TriMesh* mesh, int facetId);
}

#endif // MSBASE_GET_1695188680764_H