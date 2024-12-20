//Copyright (c) 2022 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef CACHE_H
#define CACHE_H
#include <string>
#include "slice/sliceddata.h"
#include "crslice/load.h"
#include "utils/ExtrusionLine.h"
#include "utils/PolygonsSegmentIndex.h"
#include "crsliceinfo.h"

namespace cura52 
{
	class SkinPart;
	class SliceLayerPart;

	void convertExtrusionLine(const ExtrusionLine& line, CrPolygon& poly);
	void convertVariableLines(const VariableWidthLines& lines, CrPolygons& polys);
	void convertVectorVariableLines(const std::vector<VariableWidthLines>& vecLines,
		std::vector<CrPolygons>& vecPolys);

	void convertExtrusionJunction(const ExtrusionJunction& junction, crslice::CrExtrusionJunction& junc);
	void convertExtrusionLine(const ExtrusionLine& line, crslice::CrExtrusionLine& poly);
	void convertVariableLines(const VariableWidthLines& lines, crslice::CrVariableLines& polys);
	void convertVectorVariableLines(const std::vector<VariableWidthLines>& vecLines,
		std::vector<crslice::CrVariableLines>& vecPolys);

	void convertLayerPart(const SkinPart& skinPart,
		crslice::CrSkinPart& crSkin);
	void convertLayerPart(const SliceLayerPart& layerPart,
		crslice::CrSliceLayerPart& crPart);

	void initUseCache(bool cache, const std::string& path);

	class SliceContext;
	class SliceDataStorage;
	class Cache 
	{
	public:
		Cache(const std::string& fileName, SliceContext* context);
		virtual ~Cache();

	protected:
		SliceContext* m_context;
		std::string m_root;

		int m_debugStep;
		int m_skeletalIndex;
	};

	void cacheSlicedData(const std::vector<SlicedData>& datas);
	void cacheAll(const SliceDataStorage& storage);

	void cacheSkeletal(crslice::SerailCrSkeletal& skeletal);
	void cacheDiscretizeParabola(const Point& point, const PolygonsSegmentIndex& segment,
		const Point& start, const Point& end);
	void cacheDiscretizeEdge(const Point& start, const Point& end);
	void cacheDiscretizeStraightEdge(const Point& left, const Point& right, 
		const Point& start, const Point& end);
} // namespace cura52

#endif // SVG_H
