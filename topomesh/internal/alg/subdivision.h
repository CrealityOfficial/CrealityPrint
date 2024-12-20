#pragma once
#include "internal/data/mmesht.h"


namespace topomesh {
	void SimpleMidSubdivision(MMeshT* mesh, std::vector<int>& faceindexs);
	void loopSubdivision(MMeshT* mesh, std::vector<int>& faceindexs,std::vector<std::tuple<int, trimesh::point>>& vertex,
		std::vector<std::tuple<int,trimesh::ivec3>>& face_vertex , bool is_move,int iteration);
	void sqrt3Subdivision(MMeshT* mesh, std::vector<int>& faceindexs);
}