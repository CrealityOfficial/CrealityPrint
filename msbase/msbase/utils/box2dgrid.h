#ifndef MSBASE_CREATIVE_KERNEL_BOX2DGRID_1594095137563_H
#define MSBASE_CREATIVE_KERNEL_BOX2DGRID_1594095137563_H
#include "msbase/interface.h"
#include "trimesh2/TriMesh.h"
#include "trimesh2/Box.h"
#include "trimesh2/XForm.h"

namespace msbase
{
	struct VerticalSeg
	{
		trimesh::vec3 t;
		trimesh::vec3 b;
	};

	struct CollideResult
	{
		trimesh::vec3 c;
		trimesh::vec3 n;
	};

	struct GenSource
	{
		int vertexNum;
		int faceNum;
		std::vector<int> supportVertexes;
		std::vector<trimesh::ivec2> supportEdges;
		std::vector<std::vector<int>> supportFaces;

		std::vector<int> vertexFlags;   // 0 undefined, 1 lowest, 2 edge add

		std::vector<trimesh::vec4> saveSupports;
	};

	struct DLPResult
	{
		trimesh::vec3 top;
		trimesh::vec3 middle;
		trimesh::vec3 bottom;
		bool platform;
	};

	struct DLPSource
	{
		trimesh::vec3 position;
		trimesh::vec3 normal;
	};

	class MSBASE_API Box2DGrid  
	{
		struct VerticalCollide
		{
			trimesh::vec2 xy;
			float z;
			int faceid;
		};

		enum class CheckDir
		{
			eUp,
			eDown
		};
	public:
		Box2DGrid();
		~Box2DGrid();

		void build(trimesh::TriMesh* mesh, trimesh::fxform xf, bool isFanZhuan);

		void buildGlobalProperties();
		void clear();

		bool contain(trimesh::vec2 xy);

		void check(std::vector<VerticalSeg>& segments, trimesh::vec3& c, int faceID);
		std::vector<VerticalSeg> recheckVertexSupports(std::vector<VerticalSeg> Vertexes);
		void checkDown(std::vector<CollideResult>& collides, trimesh::vec3& c);
		void autoCheckDown(DLPSource& source, DLPResult& result, float len);
		void autoCheckDown(std::vector<DLPSource>& sources, std::vector<DLPResult>& results, float len);
		void autoDLPCheckDown(std::vector<DLPResult>& results, float len);

		void dlpSource(std::vector<DLPSource>& sources);
		trimesh::ivec2 index(trimesh::vec2& xy);

		bool checkFace(int primitiveID);
		trimesh::vec3 checkFaceNormal(int primitiveID);

		void autoSupportOfVecAndSeg(std::vector<VerticalSeg>& supports, float size, bool platform, bool uplight = true);
		void autoSupport(std::vector<VerticalSeg>& segments, float size, float angle, bool platform);
		void autoDLPSupport(std::vector<VerticalSeg>& supports, float ratio, float angle, bool platform);

		void genSource(GenSource& source);
		void genSourceOfVecAndSeg(GenSource& source);
	protected:
		inline int halfcode(int face, int index)
		{
			return (face << 2) + index;
		}
		inline void halfdecode(int half, int& face, int& index)
		{
			index = half & 3;
			face = half >> 2;
		}

		inline int faceid(int half)
		{
			return half >> 2;
		}

		inline int startvertexid(int half)
		{
			int face = half >> 2;
			int idx = half & 3;
			return m_mesh->faces.at(face)[idx];
		}

		inline int endvertexid(int half)
		{
			int face = half >> 2;
			int idx = ((half & 3) + 1) % 3;
			return m_mesh->faces.at(face)[idx];
		}

		void genSupports(std::vector<VerticalSeg>& segments, GenSource& source);

		void addVertexSupports(std::vector<VerticalSeg>& segments, GenSource& source);
		void addEdgeSupports(std::vector<VerticalSeg>& segments, GenSource& source, float minDelta = 3.0);
		void addFaceSupports(std::vector<VerticalSeg>& segments, GenSource& source);

		void addVertexSources(std::vector<DLPSource>& sources, GenSource& source);
		void addEdgeSources(std::vector<DLPSource>& sources, GenSource& source);
		void addFaceSources(std::vector<DLPSource>& sources, GenSource& source);

		bool testInsert(std::vector<trimesh::vec2>& samples, trimesh::vec2& xy, float radius);

		void check(std::vector<VerticalCollide>& collides, trimesh::vec3& c, CheckDir dir, int id, bool useFace = true, bool skip = false);
	protected:
		trimesh::TriMesh* m_mesh;
		trimesh::fxform m_xf;

		std::vector<trimesh::box2> m_boxes;
		std::vector<trimesh::vec2> m_depths;
		std::vector<trimesh::ivec4> m_indexes;

		std::vector<trimesh::vec3> m_vertexes;
		std::vector<float> m_dotValues;
		std::vector<trimesh::vec3> m_faceNormals;

		trimesh::box2 m_globalBox;

		int m_width;
		int m_height;
		const float m_gridSize;

		std::vector<std::vector<int>> m_cells;

		std::vector<std::vector<int>> m_outHalfEdges;
		std::vector<trimesh::ivec3> m_oppositeHalfEdges;
		bool m_globalBuilded;
	};
}
#endif // MSBASE_CREATIVE_KERNEL_BOX2DGRID_1594095137563_H