#ifndef MMESH_BASE_FLUSHINGVOLUMES_1612341980784_H
#define MMESH_BASE_FLUSHINGVOLUMES_1612341980784_H
#include "msbase/interface.h"
#include "trimesh2/TriMesh.h"

#include <vector>

namespace msbase
{
	typedef trimesh::vec3 Colour;

	MSBASE_API void calc_flushing_volumes(std::vector<trimesh::vec3>& m_colours
		,std::vector<bool>& filament_is_support
		, /*out*/std::vector<float>& m_matrix
		, float flush_multiplier = 1.0
		, float extra_flush_volume = 0.0f
		);
}

#endif // MMESH_BASE_FLUSHINGVOLUMES_1612341980784_H
