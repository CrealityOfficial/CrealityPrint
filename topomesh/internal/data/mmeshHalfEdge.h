#pragma once
#include "mmeshFace.h"
#include "mmeshVertex.h"

namespace topomesh {

	class MMeshHalfEdge
	{
	public:
		MMeshHalfEdge(){};
		MMeshHalfEdge(MMeshVertex* v1, MMeshVertex* v2) { edge_vertex.first = v1, edge_vertex.second = v2; };
		MMeshHalfEdge(int v1, int v2) {};
		~MMeshHalfEdge() { indication_face = nullptr; next = nullptr; opposite = nullptr;};

	private:
		enum halfedge_flag
		{
			ME_DELETE = 0x00000001,
			ME_SELECT = 0x00000002,
			ME_BORDER = 0x00000004,
			ME_VISITED = 0x00000008,
			ME_ACCUMULATE = 0x00000ff0,
			ME_LIMITING = 0x00001000,
			ME_MARKED = 0x00002000,
			ME_USER = 0xffff0000
		};
		int flag = 0;
	public:
		int index = -1;
		std::pair<MMeshVertex*, MMeshVertex*> edge_vertex;
		MMeshFace* indication_face;
		MMeshHalfEdge* next;
		MMeshHalfEdge* opposite = nullptr;

		int attritube;
		float attritube_f;
		std::vector<float> attritube_vec;
		inline void SetD() { flag |= ME_DELETE; }
		inline bool IsD() { return (ME_DELETE & flag) != 0 ? 1 : 0; }
		inline void ClearD() { flag &= ~ME_DELETE; }

		inline void SetS() { flag |= ME_SELECT; }
		inline bool IsS() { return (ME_SELECT & flag) != 0 ? 1 : 0; }
		inline void ClearS() { flag &= ~ME_SELECT; }

		inline void SetB() { flag |= ME_BORDER; }
		inline bool IsB() { return (ME_BORDER & flag) != 0 ? 1 : 0; }
		inline void ClearB() { flag &= ~ME_BORDER; }

		inline void SetV() { flag |= ME_VISITED; }
		inline bool IsV() { return (ME_VISITED & flag) != 0 ? 1 : 0; }
		inline void ClearV() { flag &= ~ME_VISITED; }

		inline void SetL() { flag |= ME_LIMITING; }
		inline bool IsL() { return (ME_LIMITING & flag) != 0 ? 1 : 0; }
		inline void ClearL() { flag &= ~ME_LIMITING; }

		inline void SetM() { flag |= ME_MARKED; }
		inline bool IsM() { return (ME_MARKED & flag) != 0 ? 1 : 0; }
		inline void ClearM() { flag &= ~ME_MARKED; }

		//0 - 255
		inline bool SetA() { int copy = flag & ME_ACCUMULATE; copy = copy >> 4;	copy += 1; if (copy > 255) return false; copy = copy << 4; flag &= ~ME_ACCUMULATE; flag |= copy; return true; }
		inline bool IsA(int i) { int copy = flag & ME_ACCUMULATE; copy = copy >> 4; if (i == copy) return true; return false; }
		inline void ClearA() { flag &= ~ME_ACCUMULATE; }
		inline int GetA() { int copy = flag & ME_ACCUMULATE; copy = copy >> 4; return copy; }

		inline bool SetU(int user) { if (user > 65536) return false; flag &= ~ME_USER; int copy = user << 16; flag |= copy; return true; }
		inline bool IsU(int i) { int copy = flag & ME_USER; copy = copy >> 16; if (i == copy)return true; return false; }
		inline void ClearU() { flag &= ~ME_USER; }
		inline int GetU() { int copy = flag & ME_USER; copy = copy >> 16; return copy; }

		inline void ClearALL() { flag &= ME_DELETE; }

		float dihedral();
	};
}