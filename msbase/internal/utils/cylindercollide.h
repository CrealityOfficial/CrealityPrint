#ifndef MMESH_CYLINDERCOLLIDE_1631348831075_H
#define MMESH_CYLINDERCOLLIDE_1631348831075_H
#include "msbase/utils/drill.h"

namespace msbase
{
	struct TriTri
	{
		trimesh::dvec3 v1;
		trimesh::dvec3 v2;
		bool topPositive;

		trimesh::ivec2 index; // -1 not edge, 1 edge

		//cylinder 
		trimesh::ivec2 cindex;
		int cylinderTopIndex;
		bool cylinderTopPositive;
	};

	struct FaceCollide
	{
		std::vector<TriTri> tris;
		int flag;
	};

	//flags -1 inner, 0 collide, 1 outer
	const int CylinderCollideOuter = 1;
	const int CylinderCollideCollide = 0;
	const int CylinderCollideInner = -1;
	const int CylinderCollideInvalid = -2;

	class OptimizeCylinderCollide
	{
	public:
		OptimizeCylinderCollide(trimesh::TriMesh* mesh, trimesh::TriMesh* cylinder,
			ccglobal::Tracer* tracer, DrillDebugger* debugger);

		// depth 设置为小于等于 0 时，则打洞只打穿一层壁，若大于 0，则打穿指定深度内的所有壁
		OptimizeCylinderCollide(trimesh::TriMesh* mesh,
			int resolution, double radius, double depth, trimesh::point pointStart, trimesh::point dir, float bottomOffset,
			ccglobal::Tracer* tracer, DrillDebugger* debugger);
		~OptimizeCylinderCollide();

		bool valid();

		trimesh::TriMesh* drill(ccglobal::Tracer* tracer);

		trimesh::TriMesh* drilldrill(ccglobal::Tracer* tracer);
	protected:
		void calculate();
		void mycalculate();

		std::vector<trimesh::point> getCirclePerimeterPoint(const trimesh::point& centerPoint, const trimesh::vec3& dir, double radius, int num);
		double getDrillDepth(const trimesh::point& checkPoint);
		double getMaxDrillDepth();
		bool lineCollideTriangle(trimesh::dvec3 linePos, trimesh::dvec3 lineDir, trimesh::dvec3 A, trimesh::dvec3 B, trimesh::dvec3 C, trimesh::dvec3& intersectedPos);

		trimesh::TriMesh* postProcess(trimesh::TriMesh* Mout, trimesh::TriMesh* Cin);
		trimesh::TriMesh* postProcessDrill(trimesh::TriMesh* Mout, trimesh::TriMesh* Cin);
	protected:
		trimesh::TriMesh* m_mesh;
		trimesh::TriMesh* m_cylinder;

		int focusTriangle;
		IndexPatch meshFocusFacesMapper;
		FacePatch meshFocusFaces;
		std::vector<FaceCollide> meshTris;
		std::vector<trimesh::vec3> focusNormals;
		std::vector<int> meshFocusVertices;

		int cylinderTriangles;
		std::vector<FaceCollide> cylinderTris;
		std::vector<trimesh::vec3> cylinderNormals;

		std::vector<int> totalMeshFlag;
		std::vector<int> cylinderFlag;

		ccglobal::Tracer* m_tracer;
		DrillDebugger* m_debugger;

		int m_cylinderResolution;
		double m_cylinderRadius;
		double m_cylinderDepth;
		trimesh::point m_cylinderPointStart;
		trimesh::point m_cylinderDir;
		float m_bottomOffset;
	};

	trimesh::TriMesh* getNewMesh(trimesh::TriMesh* mesh,
		int resolution, double radius, double depth, trimesh::point pointStart, trimesh::point dir,
		ccglobal::Tracer* tracer, DrillDebugger* debugger = nullptr);
}

#endif // MMESH_CYLINDERCOLLIDE_1631348831075_H