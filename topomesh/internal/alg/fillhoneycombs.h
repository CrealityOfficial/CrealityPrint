#pragma once
#include "topomesh/interface/hex.h"

#include "internal/alg/fillhoneycombs.h"
#include "internal/data/CMesh.h"

#include "ccglobal/tracer.h"
#include <memory>

namespace topomesh {
	
    struct HoneyCombParam
    {
        int mode = 0; ///< 0 is shell, 1 is backfill.
        TriPolygon* polyline = nullptr; ///< input polygons boundary.
        trimesh::vec3 axisDir = trimesh::vec3(0, 0, 0); ///< default cylindrical orientation.
        //trimesh::vec3* axisDir = nullptr;
       
        std::vector<std::vector<int>> faces; ///< faces ids of the input flat region.
        // for honeycomb
        double honeyCombRadius = 2.0; ///<to generate honeycomb radius.
        double nestWidth = 1.0; ///<to generate honeycomb nest width.
        double shellThickness = 0.6; ///<exshell thickness of honeycomb model.
        
        // for bottom boundary clip
        double resolution = 1E-4; ///< clipper polygon resolution.
        double keepHexagonRate = 0.1; ///<minimum area ratio of the hexagonal grid to retain.
        double keepHexagonArea = 2.0; ///<minimum area of the hexagonal grid to retain.
        trimesh::vec2 arrayDir = trimesh::vec2(1, 0); ///<default the hexagonal grids array orientation.

        // for circular holes
        bool holeConnect = true; ///< is holes connected.
        float cheight = 5.0f; ///< the lowest point height of the first layer holes. 
        float ratio = 0.5f; ///< the portion of holes diameter to the edge length of the hexagonal grids.
        float delta = 0.5f; ///< the distance between the two separated circular holes.
        int nslices = 17;   ///< number of edges of the positive polygon fitting the circular hole.
        
        //debug
        bool bKeepHexagon = false; ///< Hexagons is to keep or not.
        int step_return = 9999; ///< debug quick return
    };

	class HoneyCombDebugger
	{
	public:
		virtual void onGenerateBottomPolygons(const TriPolygons& polygons) = 0;
		virtual void onGenerateInfillPolygons(const TriPolygons& polygons) = 0;
	};

    trimesh::TriMesh* generateHoneyCombs(trimesh::TriMesh* trimesh = nullptr, const HoneyCombParam& honeyparams = HoneyCombParam(),
        ccglobal::Tracer* tracer = nullptr, HoneyCombDebugger* debugger = nullptr);


	std::shared_ptr<trimesh::TriMesh> GenerateHoneyCombs(trimesh::TriMesh* trimesh, int& error, const HoneyCombParam& honeyparams = HoneyCombParam(),
		ccglobal::Tracer* tracer = nullptr, HoneyCombDebugger* debugger = nullptr);

    struct honeyLetterOpt {
        double side = 1.0;
        std::vector<int>bottom; ///<
        std::vector<int>others; ///<
        /*
        hexagon: 
        radius: 
        borders: 
        neighbors: 
        */
        std::vector<HexaPolygon>hexgons;
    };

    bool getTriMeshBoundarys(trimesh::TriMesh& trimesh, std::vector<std::vector<int>>& sequentials);
    bool GenerateBottomHexagons(const CMesh& honeyMesh, const HoneyCombParam& honeyparams, honeyLetterOpt& letterOpts, HoneyCombDebugger* debugger = nullptr);
    void GenerateTriPolygonsHexagons(const TriPolygons& polys, const HoneyCombParam& honeyparams, honeyLetterOpt& letterOpts, HoneyCombDebugger* debugger = nullptr);

	trimesh::vec3 adjustHoneyCombParam(trimesh::TriMesh* trimesh,const HoneyCombParam& honeyparams);
    TriPolygon traitPlanarCircle(const trimesh::vec3& c, float r, std::vector<int>& indexs, const trimesh::vec3& edgeDir = trimesh::vec3(0, 0, 1), int nums = 17);

    HexaPolygons generateEmbedHolesColumnar(trimesh::TriMesh* trimesh = nullptr, const HoneyCombParam& honeyparams = HoneyCombParam(),
        ccglobal::Tracer* tracer = nullptr, HoneyCombDebugger* debugger = nullptr);

	class MMeshT;
	void findNeighVertex(trimesh::TriMesh* mesh, const std::vector<int>& upfaceid, const std::vector<int>& botfaceid, std::vector<std::pair<int, int>>& vertex_distance);
	void findNeighVertex(MMeshT* mesh, const std::vector<int>& upfaceid, const std::vector<int>& botfaceid, std::vector<std::pair<int, float>>& vertex_distance);
	void innerHex(MMeshT* mesh, std::vector<std::vector<trimesh::vec2>>& poly, std::vector<int>& inFace, std::vector<int>& outFace,float len);

    trimesh::TriMesh* findOutlineOfDir(trimesh::TriMesh* mesh,std::vector<int>& botfaces);   
    void JointBotMesh(trimesh::TriMesh* mesh, trimesh::TriMesh* newmesh, std::vector<int>& botfaces,int mode=0);
    void SelectInnerFaces(trimesh::TriMesh* mesh, const std::vector<int>& in, int indicate, std::vector<int>& out);
    void SelectBorderFaces(trimesh::TriMesh* mesh, int indicate, std::vector<int>& out);
    void LastFaces(trimesh::TriMesh* mesh, const std::vector<int>& in, std::vector<std::vector<int>>& out, int threshold, std::vector<int>& other_shells);
    void FilterShells(trimesh::TriMesh* mesh, int threshold, std::vector<int>& other_shells);
    std::shared_ptr<trimesh::TriMesh> SetHoneyCombHeight(trimesh::TriMesh* mesh, const HoneyCombParam& honeyparams, honeyLetterOpt& letterOpts);
}