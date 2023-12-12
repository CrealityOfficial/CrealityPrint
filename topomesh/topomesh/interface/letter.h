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
		bool cacheInput = false;
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
}

#endif // TOPOMESH_LETTER_1692613164094_H