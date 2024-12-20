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
		double cylinder_depth;                         	// depth ����ΪС�ڵ��� 0 ʱ�����ֻ��һ��ڣ������� 0�����ָ������ڵ����б�
		trimesh::vec3 cylinder_startPos;          //  ��������������
		trimesh::vec3 cylinder_Dir;
		float bottomOffset = 1.0f;
	};

	MSBASE_API trimesh::TriMesh* drill(trimesh::TriMesh* mesh, trimesh::TriMesh* cylinderMesh,
		ccglobal::Tracer* tracer, DrillDebugger* debugger);

	// depth ����ΪС�ڵ��� 0 ʱ�����ֻ��һ��ڣ������� 0�����ָ������ڵ����б�
	MSBASE_API trimesh::TriMesh* drillCylinder(trimesh::TriMesh* mesh, const DrillParam& param, ccglobal::Tracer* tracer, DrillDebugger* debugger,bool useNewDrill = false); //type false: ԭʼ�� true:ȥ�غ�Ĵ�
	//��ӡ�����
	MSBASE_API trimesh::TriMesh* drillCylinder(trimesh::TriMesh* mesh, std::vector<DrillParam>& params, ccglobal::Tracer* tracer, DrillDebugger* debugger, bool useNewDrill = false);
}

#endif // MSBASE_DRILL_1631339010743_H