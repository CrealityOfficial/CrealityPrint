#ifndef TOPOMESH_INTERACTIVESLICE_1695453614025_H
#define TOPOMESH_INTERACTIVESLICE_1695453614025_H
#include "topomesh/interface/idata.h"

namespace topomesh
{
	struct Path {
		bool solid{ true };
		std::vector<trimesh::vec2> line;
	};
	using Contour = std::vector<Path>;

	class InteractiveSliceImpl;
	class TOPOMESH_API InteractiveSlice
	{
	public:
		InteractiveSlice();
		~InteractiveSlice();

		void reset(const std::vector<trimesh::TriMesh*>& meshes);
		void contourSplit(const std::vector<std::vector<trimesh::vec2>>& contours, float interval);

		void getCurrentContours(std::vector<Contour>& coutours);
	protected:
		InteractiveSliceImpl* impl;
	};
}

#endif // TOPOMESH_INTERACTIVESLICE_1695453614025_H