#pragma once
#include <vector>
#include "trimesh2/TriMesh.h"

typedef trimesh::point point;

namespace topomesh
{
	bool rayTriangleIntersect_1(const point&,const point&,const point&,const point&,const point&,point&);
	bool rayTriangleIntersect_2(const point&, const point&, const point&, const point&, const point&, point&);
	bool faceTriangleIntersect(const trimesh::point&, const trimesh::point&, const trimesh::point&,
								const trimesh::point&,const trimesh::point&,const trimesh::point&,
								trimesh::point&,trimesh::point&);
}