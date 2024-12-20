#ifndef TOPOMESH_LETTER_1680853426716_H
#define TOPOMESH_LETTER_1680853426716_H
#include "topomesh/interface/letter.h"

#include "internal/data/mmesht.h"
#include "internal/alg/trans.h"

#include "trimesh2/XForm.h"
#include "trimesh2/quaternion.h"

#include "cmath"
#include <vector>
#include <queue>
#include <numeric>

#include "ccglobal/tracer.h"

namespace topomesh
{		
	void concaveOrConvexOfFaces(MMeshT* mt,std::vector<int>& faces,bool concave=false,float deep=2.0);
	void splitPoint(MMeshT* mt, MMeshVertex* v, trimesh::point ori);	
	void TrimeshEmbedingAndCutting(trimesh::TriMesh* mesh, const std::vector<std::vector<trimesh::vec2>>& lines, std::vector<int>& facesIndex, bool is_close = true);
	void embedingAndCutting(MMeshT* mesh, std::vector<std::vector<trimesh::vec2>>& lines,const std::vector<int>& facesIndex,bool is_close=true);	
	trimesh::point getWorldPoint(const CameraParam& camera, trimesh::ivec2 p);
	bool intersectionTriangle(MMeshT* mt,trimesh::point p,trimesh::point normal);
	void TrimeshpolygonInnerFaces(trimesh::TriMesh* mesh, std::vector<std::vector<std::vector<trimesh::vec2>>>& poly, std::vector<int>& infaceIndex, std::vector<int>& outfaceIndex);
	void polygonInnerFaces(MMeshT* mt,const std::vector<std::vector<std::vector<trimesh::vec2>>>& poly, std::vector<int>& infaceIndex, std::vector<int>& outfaceIndex);
	void loadCameraParam(CameraParam& camera);
	void fillTriangle(MMeshT* mesh, std::vector<int>& vindex);
	void fillTriangleForTraverse(MMeshT* mesh, std::vector<int>& vindex,bool is_rollback=false);
	void getMeshFaces(MMeshT* mesh, const std::vector<std::vector<trimesh::vec2>>& polygons, const CameraParam& camera, std::vector<int>& faces);
	void getMeshFaces(trimesh::TriMesh* mesh, const std::vector<std::vector<std::vector<trimesh::vec2>>>& polygons, const CameraParam& camera, std::vector<int>& faces,float threshold);
	void getDisCoverFaces(MMeshT* mesh, std::vector<int>& faces, std::map<int, int>& fmap, std::vector<trimesh::vec3>& mesh_normal
		, std::vector<trimesh::vec3>& face_center, trimesh::vec3 camera_pos);
	void mapping(MMeshT* mesh, trimesh::TriMesh* trimesh, std::map<int, int>& vmap, std::map<int, int>& fmap,bool is_thread=false);
	void fillholes(trimesh::TriMesh* mesh);
	void simpleCutting(MMeshT* mesh, const std::vector<std::vector<std::vector<trimesh::vec2>>>& polygons, std::vector<std::vector<int>>& faceindexs);
	bool checkCamera(const CameraParam& camera,trimesh::TriMesh* mesh);
	void MeshGroupInterface(const std::vector<trimesh::TriMesh*>& mesh_group, const SimpleCamera& camera, const LetterParam& Letter, const std::vector<TriPolygons>& polygons, bool& letterOpState,
		std::vector<trimesh::TriMesh*>& out_mesh,LetterDebugger* debugger=nullptr, ccglobal::Tracer* tracer=nullptr);
	void setMark(std::vector<std::vector<trimesh::vec2>>& totalpoly);
	
	trimesh::TriMesh* letter(trimesh::TriMesh* mesh, const SimpleCamera& camera, const LetterParam& Letter, const std::vector<TriPolygons>& polygons, bool& letterOpState,
		LetterDebugger* debugger = nullptr, ccglobal::Tracer* tracer = nullptr);
}

#endif // TOPOMESH_LETTER_1680853426716_H