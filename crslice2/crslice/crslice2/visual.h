#ifndef CRSLICE_VISUAL_H_2
#define CRSLICE_VISUAL_H_2
#include "crslice2/header.h"

namespace crslice2
{
	typedef trimesh::vec2 VPoint;

	struct VPolygon
	{
		std::vector<VPoint> points;
	};

	struct VExPolygon
	{
		VPolygon  contour;
		std::vector<VPolygon> holes;
	};

	struct VExPolygons
	{
		std::vector<VExPolygon> polys;
	};

	struct VLayerRegion
	{
		//SurfaceCollection           slices;
		VExPolygons                  raw_slices;
		//ExtrusionEntityCollection   thin_fills;
		VExPolygons                  fill_expolygons;
		//SurfaceCollection           fill_surfaces;
		VExPolygons                  fill_no_overlap_expolygons;
		//Polylines          			unsupported_bridge_edges;
		//ExtrusionEntityCollection   perimeters;
		//ExtrusionEntityCollection   fills;
	};

	struct VLayer
	{
		VExPolygons          sharp_tails;
		VExPolygons          cantilevers;

		VExPolygons 		 lslices;
		VExPolygons          loverhangs;

		std::vector<VLayerRegion> regions;
	};

	struct VSupportLayer
	{

	};

	struct VPrintObject
	{
		std::vector<VLayer> layers;
		std::vector<VSupportLayer> supports;
	};

	struct VPrint
	{
		std::vector<VPrintObject> objects;
	};

	CRSLICE2_API void loadVPrint(const std::string& fileName, VPrint& print);
	CRSLICE2_API void saveVPrint(const std::string& fileName, const VPrint& print);
}
#endif  // MSIMPLIFY_SIMPLIFY_H
