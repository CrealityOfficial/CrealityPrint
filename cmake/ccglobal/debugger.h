#ifndef CCGLOBAL_RENDER_DEBUGGER_1684896500824_H
#define CCGLOBAL_RENDER_DEBUGGER_1684896500824_H
#include "trimesh2/TriMesh.h"
#include "trimesh2/XForm.h"

namespace ccglobal
{
	typedef std::vector<trimesh::vec3> Polygon;
	
	class VisualDebugger
	{
	public:
		virtual ~VisualDebugger() {}

		virtual void clear_visual(const std::string& pattern) = 0;
		virtual void visual_polygon(const std::string& name, const Polygon& polygon, const trimesh::vec4& color, float width) = 0;
		virtual void visual_color_polygon(const std::string& name, const Polygon& polygons, const std::vector<trimesh::vec4>& colors, float width) = 0;
		virtual void visual_color_points(const std::string& name, const Polygon& points, const trimesh::vec4& color, float point_size) = 0;
	};
}

#endif 