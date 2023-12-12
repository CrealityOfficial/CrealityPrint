#pragma once
#include "trimesh2/Vec.h"
#include "trimesh2/TriMesh.h"
#include "mmeshVertex.h"


namespace topomesh
{
	//class MMesh
	class MMeshHalfEdge;

	class MMeshFace
	{
	public:
		MMeshFace() {};
		MMeshFace(trimesh::TriMesh::Face f) { connect_vertex.reserve(3); };
		MMeshFace(MMeshVertex* v0, MMeshVertex* v1, MMeshVertex* v2)
		{		
			connect_vertex.reserve(3); connect_vertex.push_back(v0); connect_vertex.push_back(v1); connect_vertex.push_back(v2);
		}
		~MMeshFace() {};			
		bool operator==(MMeshFace* f) const { return this == f ? true : false; };
	private:
		enum faceflag
		{
			MF_DELETED = 0x00000001,
			MF_SELECT = 0x00000002,
			MF_BORDER = 0x00000004,
			MF_VISITED = 0x00000008,
			MF_ACCUMULATE = 0x00000ff0,
			MF_LIMITING = 0x00001000,
			MF_MARKED = 0x00002000,
			MF_USER = 0xffff0000
		};
		unsigned int flag = 0;
	public:
		MMeshHalfEdge* f_mhe = nullptr;
		trimesh::point normal;
		int index = -1;
		std::vector<MMeshVertex*> connect_vertex;
		std::vector<MMeshFace*> connect_face;

		std::vector<trimesh::vec4> uv_coord;
		std::vector<trimesh::vec4> inner_vertex;		
	
		inline void SetD() { flag |= MF_DELETED; }
		inline bool IsD() { return (MF_DELETED & flag)!=0 ? 1 : 0; }
		inline void ClearD() { flag &= ~MF_DELETED; }

		inline void SetS() { flag |= MF_SELECT; }
		inline bool IsS() { return (MF_SELECT & flag) != 0 ? 1 : 0; }
		inline void ClearS() { flag &= ~MF_SELECT; }

		inline void SetB() { flag |= MF_BORDER; }
		inline bool IsB() { return (MF_BORDER & flag) != 0 ? 1 : 0; }
		inline void ClearB() { flag &= ~MF_BORDER; }

		inline void SetV() { flag |= MF_VISITED; }
		inline bool IsV() { return (MF_VISITED & flag) != 0 ? 1 : 0; }
		inline void ClearV() { flag &= ~MF_VISITED; }

		//0 - 255
		inline bool SetA() { int copy = flag & MF_ACCUMULATE; copy = copy >> 4; copy += 1; if (copy > 255) return false; copy = copy << 4;flag&=~MF_ACCUMULATE ; flag |= copy; return true; }
		inline bool IsA(int i) { int copy = flag & MF_ACCUMULATE; copy = copy >> 4; if (i == copy) return true; return false; }
		inline void ClearA() { flag &= ~MF_ACCUMULATE; }
		inline int getA() { int copy = flag & MF_ACCUMULATE; copy = copy >> 4; return copy; }

		inline void SetL() { flag |= MF_LIMITING; }
		inline bool IsL() { return (MF_LIMITING & flag) != 0 ? 1 : 0; }
		inline void ClearL() { flag &= ~MF_LIMITING; }

		inline void SetM() { flag |= MF_MARKED; }
		inline bool IsM() { return (MF_MARKED & flag) != 0 ? 1 : 0; }
		inline void ClearM() { flag &= ~MF_MARKED; }

		inline bool SetU(int user) { if (user > 65535) return false; flag &= ~MF_USER; int copy = user << 16; flag |= copy; return true; }
		inline bool IsU(int i) { int copy = flag & MF_USER; copy = copy >> 16; if (i == copy)return true; return false; }
		inline void ClearU() { flag &= ~MF_USER; }
		inline int GetU() { unsigned int copy = flag & MF_USER; copy = copy >> 16; return copy; }

		inline void ClearALL() { flag &= MF_DELETED; }

		inline void open_uv() { uv_coord.reserve(8); }

		inline MMeshVertex* V0(int i) { return connect_vertex[(i + 0) % 3]; }
		inline MMeshVertex* V1(int i) { return connect_vertex[(i + 1) % 3]; }
		inline MMeshVertex* V2(int i) { return connect_vertex[(i + 2) % 3]; }
		inline MMeshVertex* V0(MMeshVertex* v){
			for (int i = 0; i < 3; i++)
			{
				if (v == this->connect_vertex[i])
					return this->connect_vertex[i];
			}
			return nullptr;
		}
		inline MMeshVertex* V1(MMeshVertex* v){ 
			for (int i = 0; i < 3; i++)
			{
				if (v == this->connect_vertex[i])
					return this->connect_vertex[(i+1)%3];
			}
			return nullptr;
		}
		inline MMeshVertex* V2(MMeshVertex* v) {
			for (int i = 0; i < 3; i++)
			{
				if (v == this->connect_vertex[i])
					return this->connect_vertex[(i + 2) % 3];
			}
			return nullptr;
		}
						
		
		inline int getVFindex(int vi) {
			for (int i = 0; i < this->connect_vertex.size(); i++)
				if (this->connect_vertex[i]->index == vi)
					return i;
			return -1;
		}

		inline int getVFindex(MMeshVertex* v) {
			for (int i = 0; i < this->connect_vertex.size(); i++)
				if (this->connect_vertex[i] == v)
					return i;
			return -1;
		}

		bool innerFace(trimesh::point v);
		std::pair<int, int> commonEdge(MMeshFace* f);
		float dihedral(MMeshFace* f);
	};
}