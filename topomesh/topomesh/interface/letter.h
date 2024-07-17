#ifndef TOPOMESH_LETTER_1692613164094_H
#define TOPOMESH_LETTER_1692613164094_H
#include "topomesh/interface/idata.h"

namespace topomesh
{
	struct LetterParam
	{
		bool concave = true;
		float deep = 2.0f;

		//debug
		std::string fileName;
	};

	class LetterDebugger 
	{
	public:
		virtual ~LetterDebugger() {}

		virtual void onMeshProjected(const std::vector<trimesh::vec3>& triangles) = 0;
	};

	TOPOMESH_API trimesh::TriMesh* letter(trimesh::TriMesh* mesh, const SimpleCamera& camera, 
		const LetterParam& param, const std::vector<TriPolygons>& polygons,
		LetterDebugger* debugger = nullptr, ccglobal::Tracer* tracer = nullptr);

	TOPOMESH_API trimesh::TriMesh* letterFromFile(const std::string& fileName, LetterDebugger* debugger = nullptr, ccglobal::Tracer* tracer = nullptr);

	TOPOMESH_API void embedingAndCutting(trimesh::TriMesh* mesh, const std::vector<std::vector<trimesh::vec2>>& lines,
		std::vector<int>& facesIndex, bool is_close = true);

	TOPOMESH_API void polygonInnerFaces(trimesh::TriMesh* mesh, std::vector<std::vector<std::vector<trimesh::vec2>>>& poly, std::vector<int>& infaceIndex, std::vector<int>& outfaceIndex);
}

#endif // TOPOMESH_LETTER_1692613164094_H