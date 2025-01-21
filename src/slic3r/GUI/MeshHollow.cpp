#include "MeshHollow.hpp"
#include <libslic3r/OpenVDBUtils.hpp>
#include <libslic3r/SLA/Hollowing.hpp>
#include <openvdb/tools/ValueTransformer.h>
#include <openvdb/tools/MeshToVolume.h>

namespace Slic3r {

	void MeshHollow(ModelVolumePtrs& input_mesh, float depth)
	{ 
		for (auto& m : input_mesh) {
            TriangleMesh mesh = m->mesh();
			//hollow
            Slic3r::sla::HollowingConfig hc;
			hc.quality=1.0f;
			hc.closing_distance=2.0f;
			//---1-----
            //hollow_mesh(mesh,hc);
            openvdb::FloatGrid::Ptr grid = mesh_to_grid(mesh.its, {}, hc.quality, 0.3f * depth, 1.8f * depth);
           // grid                         = redistance_grid(*grid, -(1.0f), 0.5f, 0.5f);

			indexed_triangle_set result_mesh = grid_to_mesh(*grid, -depth * hc.quality, 0.f);          
            mesh.merge(TriangleMesh{result_mesh});
            m->set_mesh(mesh);
		}
	}
}