#ifndef MSBASE_DESERIALIZECOLOR_1695441390758_H
#define MSBASE_DESERIALIZECOLOR_1695441390758_H
#include "msbase/interface.h"
#include "trimesh2/TriMesh.h"
#include "trimesh2/XForm.h"
#include "ccglobal/tracer.h"

namespace msbase
{
	MSBASE_API trimesh::TriMesh* mergeColorMeshes(trimesh::TriMesh* sourceMesh, const std::vector<std::string>& color2Facets, 
		std::vector<int>& facet2Facets, bool onlyFlags = false,int state = 0,ccglobal::Tracer* tracer = nullptr);

	MSBASE_API bool change_all_triangles_state(std::vector<std::string>& color2Facets,
		int ori_state);


	MSBASE_API bool getColorPloygon(trimesh::TriMesh* sourceMesh, const trimesh::xform& _xform, const std::vector<std::string>& color2Facets,
		const std::string& fileName, int state = 0, ccglobal::Tracer* tracer = nullptr);

	//set default colors
	MSBASE_API void fill_triangles(std::vector<std::string>& strList,int faceSize, int color);
}

#endif // MSBASE_DESERIALIZECOLOR_1695441390758_H