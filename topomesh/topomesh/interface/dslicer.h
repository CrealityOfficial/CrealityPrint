#ifndef CX_D_DLPSLICER_1602646711722_H
#define CX_D_DLPSLICER_1602646711722_H
#include "topomesh/interface/dlpdata.h"
#include "trimesh2/TriMesh.h"
#include <memory>
#include "ccglobal/tracer.h"

namespace topomesh
{
	struct DLPSliceParam
	{
		float initial_layer_thickness;
		float layer_thickness;
		int  initial_layer_count;
		float minimum_polygon_circumference;
		float line_segment_resolution;
		float line_segment_deviation;

		float xy_offset;
		bool enable_xy_offset;

		bool enable_b_offset;
		float sThickness;
		float sh;
		float sLength;
		float sWidth;
		float pixX;
		float pixY;

		DLPSliceParam()
		{
			initial_layer_thickness = 300.0f;
			layer_thickness = 100.0f;
			initial_layer_count = 1;

			minimum_polygon_circumference = 1000.0f;
			line_segment_resolution = 50.0f;
			line_segment_deviation = 50.0f;

			xy_offset = 0.0f;
			enable_xy_offset = false;

			enable_b_offset = false;
			sThickness = 0.5f * 1000.0f;
			sh = 100.0f * 1000.0f;
			sLength = 192.0f * 1000.0f;
			sWidth = 1000.0f;
			pixX = 3240.0f;
			pixY = 2400.0f;
		}
	};

	struct DLPSliceObject
	{
		std::shared_ptr<trimesh::TriMesh> mesh;
	};

	TOPOMESH_API bool dlp_slice(const std::vector<DLPSliceObject>& objects, const DLPSliceParam& param,
		/*out*/DLPData& data, ccglobal::Tracer* tracer);
}

#endif // CX_D_DLPSLICER_1602646711722_H