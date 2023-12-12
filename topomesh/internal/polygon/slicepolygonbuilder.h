#ifndef SLICE_SLICEPOLYGONBUILDER_1598763429390_H
#define SLICE_SLICEPOLYGONBUILDER_1598763429390_H
#include "vertex.h"

#include "polygon.h"
#include <vector>
#include <unordered_map>

namespace topomesh
{
	class SliceHelper;
	class SlicePolygonBuilder
	{
	public:
		SlicePolygonBuilder();
		~SlicePolygonBuilder();

		void sliceOneLayer_dst(SliceHelper* helper, int z, Polygons* polygons, Polygons* openPolygons);
		void makePolygon(Polygons* polygons, Polygons* openPolygons);

        void connectOpenPolylines(Polygons& polygons, Polygons& openPolygons,ClipperLib::Path& intersectPoints);

		void connectOpenPolylines(Polygons& polygons, Polygons& openPolygons);

		void removeSamePoint(Polygons& openPolygons);

		std::vector<SlicerSegment> segments;
		std::unordered_map<int, int> face_idx_to_segment_idx; // topology

		void write(std::fstream& out);
	protected:
		int tryFaceNextSegmentIdx(const SlicerSegment& segment,
			const int face_idx, const size_t start_segment_idx);

		int getNextSegmentIdx(const SlicerSegment& segment, const size_t start_segment_idx);

	protected:
		std::vector<bool> visitFlags;
	};
}

#endif // SLICE_SLICEPOLYGONBUILDER_1598763429390_H