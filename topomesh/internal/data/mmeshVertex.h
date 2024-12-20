#pragma once

#include "trimesh2/Vec.h"
#include "trimesh2/TriMesh.h"


namespace topomesh
{
	class MMeshFace;

	class MMeshHalfEdge;
	
	class MMeshVertex
	{
	public:
		MMeshVertex() {};
		MMeshVertex(trimesh::point p) :p(p) {  /*connected_vertex.reserve(8); connected_face.reserve(8);*/ };
		~MMeshVertex() {
			std::vector<int>().swap(inner); std::vector<MMeshVertex*>().swap(connected_vertex);
			std::vector<MMeshFace*>().swap(connected_face); std::vector<MMeshHalfEdge*>().swap(v_mhe);
		};
		//MMeshFace* mf=nullptr;	
		//MMeshHalfEdge* me=nullptr;
		bool operator==(MMeshVertex* v) const { return this == v ? true : false; };
	private:
		enum vertexflag
		{
			MV_DELETE = 0x00000001,
			MV_SELECT = 0x00000002,
			MV_BORDER = 0x00000004,
			MV_VISITED = 0x00000008,
			MV_ACCUMULATE = 0x00000ff0,
			MV_LIMITING = 0x00001000,
			MV_MARKED = 0x00002000,
			MV_USER = 0xffff0000
		};
		int flag = 0;
	public:
		int index = -1;
		trimesh::point p;
		trimesh::point normal;
		std::vector<int> inner;
		std::vector<MMeshVertex*> connected_vertex;
		std::vector<MMeshFace*> connected_face;
		std::vector<MMeshHalfEdge*> v_mhe;

		inline void SetD() { flag |= MV_DELETE; }
		inline bool IsD() { return (MV_DELETE & flag) != 0 ? 1 : 0; }
		inline void ClearD() { flag &= ~MV_DELETE; }

		inline void SetS() { flag |= MV_SELECT; }
		inline bool IsS() { return (MV_SELECT & flag) != 0 ? 1 : 0; }
		inline void ClearS() { flag &= ~MV_SELECT; }

		inline void SetB() { flag |= MV_BORDER; }
		inline bool IsB() { return (MV_BORDER & flag) != 0 ? 1 : 0; }
		inline void ClearB() { flag &= ~MV_BORDER; }

		inline void SetV() { flag |= MV_VISITED; }
		inline bool IsV() { return (MV_VISITED & flag) != 0 ? 1 : 0; }
		inline void ClearV() { flag &= ~MV_VISITED; }

		//---L是专用于mesh内部
		inline void SetL() { flag |= MV_LIMITING; }
		inline bool IsL() { return (MV_LIMITING & flag) != 0 ? 1 : 0; }
		inline void ClearL() { flag &= ~MV_LIMITING; }

		inline void SetM() { flag |= MV_MARKED; }
		inline bool IsM() { return (MV_MARKED & flag) != 0 ? 1 : 0; }
		inline void ClearM() { flag &= ~MV_MARKED; }

		//0 - 255
		inline bool SetA() { int copy = flag & MV_ACCUMULATE; copy = copy >> 4;	copy += 1; if (copy > 255) return false; copy = copy << 4; flag&=~MV_ACCUMULATE; flag |= copy; return true; }
		inline bool IsA(int i) { int copy = flag & MV_ACCUMULATE; copy = copy >> 4; if (i == copy) return true; return false; }
		inline void ClearA() { flag &= ~MV_ACCUMULATE; }
		inline int GetA() { int copy = flag & MV_ACCUMULATE; copy = copy >> 4; return copy; }

		inline bool SetU(int user) { if (user > 65536) return false; flag &= ~MV_USER; int copy = user << 16; flag |= copy; return true; }
		inline bool IsU(int i) { int copy = flag & MV_USER; copy = copy >> 16; if (i == copy)return true; return false; }
		inline void ClearU() { flag &= ~MV_USER; }
		inline int GetU() { int copy = flag & MV_USER; copy = copy >> 16; return copy; }

		inline void ClearALL() { flag &= MV_DELETE; }

		bool is_neighbor(MMeshVertex* v);

	};
}