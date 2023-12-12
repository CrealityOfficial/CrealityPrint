#pragma once
#include "topomesh/interface/idata.h"

#include <list>

#include "ccglobal/tracer.h"


namespace topomesh {

	class MMeshT;
	class MMeshVertex;	
	struct VVertex
	{
		MMeshVertex* pri;
		MMeshVertex* next;
	};

	bool ModleCutting(const std::vector<trimesh::TriMesh*>& inMesh, std::vector<trimesh::TriMesh*>& outMesh, const SimpleCamera& camera,
		const TriPolygon& paths, ccglobal::Tracer* tracer = nullptr);
	bool JudgeCloseOfPath(const TriPolygon& paths);
	bool JudgeMeshIsVaild(const trimesh::TriMesh* inMesh);	
	void getPathOrder(MMeshT* mesh, const TriPolygon& paths,int start,std::vector<std::vector<int>>& order);
	void setMarkOfCorssPoint(std::vector<MMeshT*>& meshs, const TriPolygon& paths,const std::vector<std::pair<int,int>>& corssPoint );
	void splitMesh(MMeshT* mesh, std::vector<MMeshT>& outmesh,std::vector<int>& order);
	void ConnectMeshFace(MMeshT* mesh);	
}