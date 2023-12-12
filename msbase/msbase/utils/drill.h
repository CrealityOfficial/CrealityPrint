#ifndef MSBASE_DRILL_1631339010743_H
#define MSBASE_DRILL_1631339010743_H
#include "msbase/interface.h"
#include "trimesh2/TriMesh.h"
#include "ccglobal/tracer.h"
#include <vector>

namespace msbase
{
	typedef std::vector<trimesh::TriMesh::Face> FacePatch;
	typedef std::vector<int> IndexPatch;
	typedef std::vector<int> FlagPatch;
	typedef std::vector<trimesh::vec3> TriPatch;

	class DrillDebugger
	{
	public:
		virtual ~DrillDebugger() {}

		virtual void onCylinderBoxFocus(FacePatch& focus) = 0;
		virtual void onMeshOuter(trimesh::TriMesh* mesh) = 0;
		virtual void onMeshCollide(trimesh::TriMesh* mesh) = 0;
		virtual void onMeshInner(trimesh::TriMesh* mesh) = 0;
	};

	struct DrillParam
	{
		int cylinder_resolution;
		double cylinder_radius;
		double cylinder_depth;                         	// depth 设置为小于等于 0 时，则打洞只打穿一层壁，若大于 0，则打穿指定深度内的所有壁
		trimesh::vec3 cylinder_startPos;          //  打洞起点的世界坐标
		trimesh::vec3 cylinder_Dir;
	};

	MSBASE_API trimesh::TriMesh* drill(trimesh::TriMesh* mesh, trimesh::TriMesh* cylinderMesh,
		ccglobal::Tracer* tracer, DrillDebugger* debugger);

	// depth 设置为小于等于 0 时，则打洞只打穿一层壁，若大于 0，则打穿指定深度内的所有壁
	MSBASE_API trimesh::TriMesh* drillCylinder(trimesh::TriMesh* mesh, const DrillParam& param, ccglobal::Tracer* tracer, DrillDebugger* debugger,bool useNewDrill = false); //type false: 原始打洞 true:去重后的打洞
	//打印多个洞
	MSBASE_API trimesh::TriMesh* drillCylinder(trimesh::TriMesh* mesh, std::vector<DrillParam>& params, ccglobal::Tracer* tracer, DrillDebugger* debugger, bool useNewDrill = false);
}

#endif // MSBASE_DRILL_1631339010743_H