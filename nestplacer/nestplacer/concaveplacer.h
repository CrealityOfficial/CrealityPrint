#ifndef CONCAVE_NESTPLACER_H
#define CONCAVE_NESTPLACER_H
#include "nestplacer/nestplacer.h"

namespace nestplacer
{
	typedef std::vector<trimesh::vec3> ConcaveItem;
	typedef std::vector<ConcaveItem> ConcaveItems;

	typedef trimesh::vec3 NestRT; 
	typedef std::function<void(int, const NestRT& rt)> NestCallback;
	struct NestConcaveParam
	{
		trimesh::box3 box;
		float distance = 0.0f;
		float eDistance = 0.0f;
		PlaceType packType = PlaceType::ALIGNMENT;
		float rotationAngle = 30.0f;

		std::string rootDir;
		NestCallback callback = {};
		ccglobal::Tracer* tracer = nullptr;
	};

	//debug
	typedef std::vector<trimesh::vec3> DebugContour;
	typedef std::vector<DebugContour> DebugContours;

	struct DebugPolygon
	{
		DebugContour outline;
		DebugContours holes;
	};

	struct RotateDebugger
	{
		std::vector<DebugPolygon> items;
		std::vector<DebugPolygon> merged_piles;
		std::vector<DebugPolygon> nfps;
		std::vector<DebugPolygon> chull;
	};

	struct ConcaveNestDebugger
	{
		std::vector<RotateDebugger> rotates;
	};
	//debug

	NESTPLACER_API void layout_all_nest(const ConcaveItems& models, const NestConcaveParam& param,
		ConcaveNestDebugger* debugger = nullptr);

    NESTPLACER_API int test_nest(ConcaveItems& models, NestConcaveParam& param, ConcaveNestDebugger* debugger = nullptr);

	class NestPlacerImpl;
	class NESTPLACER_API NestPlacer
	{
	public:
		NestPlacer();
		~NestPlacer();

		void setInput(const ConcaveItems& models);
		void layout(const NestConcaveParam& param, ConcaveNestDebugger* _debugger);

		void testNFP(trimesh::vec3& point, DebugPolygon& poly, std::vector<trimesh::vec3>& lines);
	protected:
		NestPlacerImpl* impl;
	};
}



#endif