#include "mmeshHalfEdge.h"

namespace topomesh {
	float MMeshHalfEdge::dihedral()
	{
		MMeshFace* f1 = this->indication_face;
		MMeshFace* f2 = this->opposite->indication_face;
		float arc = f1->normal ^ f2->normal;
		arc = arc > 1.f ? 1 : arc;
		arc = arc < -1.f ? -1.f : arc;
		float ang = std::acos(arc) * 180 / M_PI;
		trimesh::point mid = (this->edge_vertex.first->p + this->edge_vertex.second->p) / 2.f;
		trimesh::point other = this->next->edge_vertex.second->p;
		trimesh::point ori = mid - other;
		if ((ori ^ f2->normal) < 0)
			return 180.f - ang;
		else
			return 180.f + ang;
	}
}