#ifndef TOPOMESH_MMESHT_1680853426715_H
#define TOPOMESH_MMESHT_1680853426715_H
#include "trimesh2/Vec.h"
#include "mmeshFace.h"
#include "mmeshVertex.h"
#include "mmeshHalfEdge.h"
#include <map>

namespace topomesh
{
	struct BoundingBox
	{
		float max_x = std::numeric_limits<float>::min(), min_x = std::numeric_limits<float>::max();
		float max_y = std::numeric_limits<float>::min(), min_y = std::numeric_limits<float>::max();
		float max_z = std::numeric_limits<float>::min(), min_z = std::numeric_limits<float>::max();
	};


	class MMeshT
	{
	public:
		MMeshT() :VVadjacent(true), VFadjacent(true), FFadjacent(true) { faces.reserve(8192); vertices.reserve(8192); };//��������������1000����
		MMeshT(int fsize, int vsize) :VVadjacent(true), VFadjacent(true), FFadjacent(true) { faces.reserve(fsize); vertices.reserve(vsize); };
		MMeshT(const MMeshT& mt);
		MMeshT(MMeshT&& mt) = default;
		//MMeshT(MMeshT& mt) {};
		MMeshT(MMeshT* mt,std::vector<int>& faceindex);		
		MMeshT(trimesh::TriMesh* currentMesh);
		MMeshT(trimesh::TriMesh* currentMesh,std::vector<int>& faces,std::map<int,int>& vmap, std::map<int, int>& fmap);
		//MMeshT& operator=(MMeshT mt) { return *this; };
		virtual ~MMeshT() { std::vector<MMeshVertex>().swap(vertices); std::vector<MMeshFace>().swap(faces); std::vector<MMeshHalfEdge>().swap(half_edge); };

		std::vector<MMeshVertex> vertices;
		std::vector<MMeshFace> faces;
		std::vector<MMeshHalfEdge> half_edge;
		BoundingBox boundingbox;
	public:
		static inline double det(trimesh::point& p0, trimesh::point& p1, trimesh::point& p2)
		{
			trimesh::vec3 a = p0 - p1;
			trimesh::vec3 b = p1 - p2;
			return sqrt(pow((a.y * b.z - a.z * b.y), 2) + pow((a.z * b.x - a.x * b.z), 2)
				+ pow((a.x * b.y - a.y * b.x), 2)) / 2.0f;
		}
		inline double det(int faceIndex)
		{
			return det(this->faces[faceIndex].V0(0)->index, this->faces[faceIndex].V0(1)->index, this->faces[faceIndex].V0(2)->index);
		}
		inline double det(int VertexIndex1, int VertexIndex2, int VertexIndex3)
		{
			return det(this->vertices[VertexIndex1].p, this->vertices[VertexIndex2].p, this->vertices[VertexIndex3].p);
		}
		void mmesh2trimesh(trimesh::TriMesh* currentMesh);

		void appendVertex(const MMeshVertex& v);
		void appendVertex(const trimesh::point& v);

		//inline void appendFace(MMeshFace& f);
		void appendFace(MMeshVertex& v0, MMeshVertex& v1, MMeshVertex& v2);
		void appendFace(int i0, int i1, int i2);

		void deleteFace(MMeshFace& f);
		void deleteFace(int i);

		void deleteVertex(MMeshVertex& v);
		void deleteVertex(int i);

		void quickTransform(trimesh::TriMesh* currentMesh);
		void shrinkMesh();
		void getMeshBoundary();
		void getMeshBoundaryFaces();
		void getEdge(std::vector<trimesh::ivec2>& edge, bool is_select = false);
		void rememory(int v_size,int f_size);
		
		void getFacesNormals();

		inline bool is_VVadjacent() { return VVadjacent; }
		inline bool is_VFadjacent() { return VFadjacent; }
		inline bool is_FFadjacent() { return FFadjacent; }
		inline bool is_VertexNormals() { return VertexNormals; }
		inline bool is_FaceNormals() { return FacesNormals; }
		inline bool is_FacePolygon() { return FacePolygon; }
		inline bool is_BoundingBox() { return bbox; }
		inline bool is_HalfEdge() { return HalfEdge; }

		inline void set_VVadjacent(bool b) { VVadjacent = b; }
		inline void set_VFadjacent(bool b) { VFadjacent = b; }
		inline void set_FFadjacent(bool b) { FFadjacent = b; }
		inline void set_VertexNormals(bool b) { VertexNormals = b; }
		inline void set_FacesNormals(bool b) { FacesNormals = b; }
		inline void set_BoundingBox(bool b) { bbox = b; }
		inline void set_HalfEdge(bool b) { HalfEdge = b; }

		inline void clear() { std::vector<MMeshVertex>().swap(vertices); std::vector<MMeshFace>().swap(faces); std::vector<MMeshHalfEdge>().swap(half_edge); }

		inline int VN() const { return vn; }
		inline int FN() const { return fn; }
		inline int EN() const { return en; }

		inline void adjustVN() { vn = vertices.size(); }
		inline void adjustFN() { fn = faces.size(); }

		void calculateCrossPoint(const std::vector<trimesh::ivec2>& edge,const std::pair<trimesh::point, trimesh::point>& line, std::vector<std::pair<float, trimesh::ivec2>>& tc);			
		void getEdgeNumber();
		void getBoundingBox();
		void init_halfedge();
		void getVertexSDFBlockCoord();

	private:
		int vn = 0;
		int fn = 0;
		int en = 0;
		bool VVadjacent = false;
		bool VFadjacent = false;
		bool FFadjacent = false;
		bool VertexNormals = false;
		bool FacesNormals = false;
		bool FacePolygon = false;
		bool bbox = false;
		bool HalfEdge = false;
		float span_x;
		float span_y;
		float span_z;
	};

	double  getTotalArea(std::vector<trimesh::point>& inVertices);
}

#endif // TOPOMESH_MMESHT_1680853426715_H