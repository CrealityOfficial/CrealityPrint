// Copyright (c) 2022 Ultimaker B.V.
// CuraEngine is released under the terms of the AGPLv3 or higher

#include <sstream>
#include "Cache.h"

#include "communication/slicecontext.h"
#include "communication/sliceDataStorage.h"

#include "crslice/load.h"
#include "conv.h"
#include "SVG.h"

#define CACHE_CHECK_STEP(n) \
if (m_debugStep < n)  \
{                     \
	m_context->setFailed();  \
	return; \
}

namespace cura52
{
	void convertExtrusionLine(const ExtrusionLine& line, CrPolygon& poly)
	{
		const std::vector<ExtrusionJunction>& jcs = line.junctions;
		for (const ExtrusionJunction& j : jcs)
		{
			poly.push_back(crslice::convert(j.p));
		}
	}

	void convertVariableLines(const VariableWidthLines& lines, CrPolygons& polys)
	{
		polys.clear();
		int size = (int)lines.size();
		if (size > 0)
		{
			polys.resize(size);
			for (int i = 0; i < size; ++i)
				convertExtrusionLine(lines.at(i), polys.at(i));
		}
	}

	void convertVectorVariableLines(const std::vector<VariableWidthLines>& vecLines, 
		std::vector<CrPolygons>& vecPolys)
	{
		vecPolys.clear();
		int size = (int)vecLines.size();
		if (size > 0)
		{
			vecPolys.resize(size);
			for (int i = 0; i < size; ++i)
				convertVariableLines(vecLines.at(i), vecPolys.at(i));
		}
	}

	void convertExtrusionJunction(const ExtrusionJunction& junction, crslice::CrExtrusionJunction& junc)
	{
		junc.p = crslice::convert(junction.p);
		junc.perimeter_index = junction.perimeter_index;
		junc.w = INT2MM(junction.w);
	}

	void convertExtrusionLine(const ExtrusionLine& line, crslice::CrExtrusionLine& poly)
	{
		poly.is_odd = line.is_odd;
		poly.is_closed = line.is_closed;

		const std::vector<ExtrusionJunction>& jcs = line.junctions;
		int size = (int)jcs.size();
		if (size > 0)
		{
			poly.junctions.resize(size);
			for (int i = 0; i < size; ++i)
				convertExtrusionJunction(jcs.at(i), poly.junctions.at(i));
		}
	}

	void convertVariableLines(const VariableWidthLines& lines, crslice::CrVariableLines& polys)
	{
		polys.clear();
		int size = (int)lines.size();
		if (size > 0)
		{
			polys.resize(size);
			for (int i = 0; i < size; ++i)
				convertExtrusionLine(lines.at(i), polys.at(i));
		}
	}

	void convertVectorVariableLines(const std::vector<VariableWidthLines>& vecLines,
		std::vector<crslice::CrVariableLines>& vecPolys)
	{
		vecPolys.clear();
		int size = (int)vecLines.size();
		if (size > 0)
		{
			vecPolys.resize(size);
			for (int i = 0; i < size; ++i)
				convertVariableLines(vecLines.at(i), vecPolys.at(i));
		}
	}

	void convertLayerPart(const SkinPart& skinPart,
		crslice::CrSkinPart& crSkin)
	{
		crslice::convertPolygonRaw(skinPart.outline, crSkin.outline);
		convertVectorVariableLines(skinPart.inset_paths, crSkin.inset_paths);
		crslice::convertPolygonRaw(skinPart.skin_fill, crSkin.skin_fill);
		crslice::convertPolygonRaw(skinPart.roofing_fill, crSkin.roofing_fill);
		crslice::convertPolygonRaw(skinPart.top_most_surface_fill, crSkin.top_most_surface_fill);
		crslice::convertPolygonRaw(skinPart.bottom_most_surface_fill, crSkin.bottom_most_surface_fill);
	}

	void convertLayerPart(const SliceLayerPart& layerPart,
		crslice::CrSliceLayerPart& crPart)
	{
		crslice::convertPolygonRaw(layerPart.outline, crPart.outline);
		crslice::convertPolygonRaw(layerPart.inner_area, crPart.inner_area);
		int count = (int)layerPart.skin_parts.size();
		if (count > 0)
		{
			crPart.skin_parts.resize(count);
			for (int i = 0; i < count; ++i)
			{
				convertLayerPart(layerPart.skin_parts.at(i), crPart.skin_parts.at(i));
			}
		}

		convertVectorVariableLines(layerPart.wall_toolpaths, crPart.wall_toolpaths);
		convertVectorVariableLines(layerPart.infill_wall_toolpaths, crPart.infill_wall_toolpaths);
	}

	bool useCache = false;
	std::string useRootPath;

	int skeletalIndex = -1;
	int discretizeIndex = -1;
	void initUseCache(bool cache, const std::string& rootPath)
	{
		useCache = cache;
		useRootPath = rootPath;

		skeletalIndex = -1;
		discretizeIndex = -1;
	}

	Cache::Cache(const std::string& fileName, SliceContext* context)
		: m_root(fileName)
		, m_context(context)
		, m_skeletalIndex(0)
	{
		const Settings& settings = m_context->sceneSettings();

		m_debugStep = settings.get<int>("fdm_slice_debug_step");
		if (m_debugStep < 0)
			m_debugStep = 0;
	}

	Cache::~Cache()
	{

	}

	void cacheSlicedData(const std::vector<SlicedData>& datas)
	{
		if (!useCache)
			return;

		int meshSize = (int)datas.size();
		for (int i = 0; i < meshSize; ++i)
		{
			const SlicedData& data = datas.at(i);
			int layerCount = (int)data.layers.size();
			for (int j = 0; j < layerCount; ++j)
			{
				const SlicedLayer& layer = data.layers.at(j);
				std::string fileName = crslice::crsliceddata_name(useRootPath, i, j);
				crslice::SerailSlicedData sslayer;
				sslayer.data.z = INT2MM(layer.z);
				crslice::convertPolygonRaw(layer.polygons, sslayer.data.polygons);
				crslice::convertPolygonRaw(layer.openPolylines, sslayer.data.open_polygons);

				ccglobal::cxndSave(sslayer, fileName);
			}
		}
	}

	void cacheAll(const SliceDataStorage& storage)
	{
		if (!useCache)
			return;

		int meshSize = (int)storage.meshes.size();
		for (int i = 0; i < meshSize; ++i)
		{
			const SliceMeshStorage& data = storage.meshes.at(i);
			int layerCount = (int)data.layers.size();
			for (int j = 0; j < layerCount; ++j)
			{
				const SliceLayer& layer = data.layers.at(j);
				crslice::SerailCrSliceLayer serialLayer;
				int count = (int)layer.parts.size();
				if (count > 0)
				{
					serialLayer.layer.parts.resize(count);
					for (int w = 0; w < count; ++w)
					{
						convertLayerPart(layer.parts.at(w), serialLayer.layer.parts.at(w));
					}
				}

				std::string fileName = crslice::crslicelayer_name(useRootPath, i, j);
				ccglobal::cxndSave(serialLayer, fileName);
			}
		}
	}

	void cacheSkeletal(crslice::SerailCrSkeletal& skeletal)
	{
		if (!useCache)
			return;

		++skeletalIndex;
		discretizeIndex = -1;
		std::string fileName = crslice::crsliceskeletal_name(useRootPath, skeletalIndex);
		ccglobal::cxndSave(skeletal, fileName);
	}

	std::string generateDiscretizeName(const std::string& extension)
	{
		++discretizeIndex;
		char name[512] = { 0 };
		sprintf(name, "%s/%d_%d.%s", useRootPath.c_str(), skeletalIndex, discretizeIndex, extension.c_str());

		return name;
	}

	void cacheDiscretizeParabola(const Point& point, const PolygonsSegmentIndex& segment,
		const Point& start, const Point& end)
	{
		if (!useCache)
			return;

		const Point a = segment.from();
		const Point b = segment.to();
#if 0
		AABB box;
		box.include(a);
		box.include(b);
		box.include(start);
		box.include(end);
		box.include(point);
		box.expand(200);

		SVG svg(fileName, box);
		svg.writePoint(a);
		svg.writePoint(b);
		svg.writePoint(start);
		svg.writePoint(end);
		svg.writePoint(point);

		SVG::ColorObject bColor(SVG::Color::BLUE);
		svg.writeLine(a, b, bColor);

		SVG::ColorObject rColor(SVG::Color::RED);
		svg.writeLine(start, end, rColor);
#endif

		crslice::SerailDiscretize discretize;
		discretize.type = 1;
		discretize.points.push_back(crslice::convert(a));
		discretize.points.push_back(crslice::convert(b));
		discretize.points.push_back(crslice::convert(start));
		discretize.points.push_back(crslice::convert(end));
		discretize.points.push_back(crslice::convert(point));
		std::string bName = generateDiscretizeName("DiscretizeParabola");
		ccglobal::cxndSave(discretize, bName);
	}

	void cacheDiscretizeEdge(const Point& start, const Point& end)
	{
		if (!useCache)
			return;

		crslice::SerailDiscretize discretize;
		discretize.type = 0;
		discretize.points.push_back(crslice::convert(start));
		discretize.points.push_back(crslice::convert(end));
		std::string bName = generateDiscretizeName("DiscretizeEdge");
		ccglobal::cxndSave(discretize, bName);
	}

	void cacheDiscretizeStraightEdge(const Point& left, const Point& right,
		const Point& start, const Point& end)
	{
		if (!useCache)
			return;

		crslice::SerailDiscretize discretize;
		discretize.type = 2;
		discretize.points.push_back(crslice::convert(left));
		discretize.points.push_back(crslice::convert(right));
		discretize.points.push_back(crslice::convert(start));
		discretize.points.push_back(crslice::convert(end));
		std::string bName = generateDiscretizeName("DiscretizeStraightEdge");
		ccglobal::cxndSave(discretize, bName);
	}
} // namespace cura52
