#ifndef SPREAD_DESERIALIZECOLOR_1695441390758_H
#define SPREAD_DESERIALIZECOLOR_1695441390758_H
#include "spread/header.h"

namespace spread
{
	SPREAD_API trimesh::TriMesh* mergeColorMeshes(trimesh::TriMesh* sourceMesh, const std::vector<std::string>& color2Facets, 
		std::vector<int>& facet2Facets, ccglobal::Tracer* tracer = nullptr);
}

#endif // SPREAD_DESERIALIZECOLOR_1695441390758_H