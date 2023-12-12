#include "topomesh/interface/dlpdata.h"
#include "clipper/clipper.hpp"
#include "internal/polygon/polygon.h"

namespace topomesh
{
	trimesh::dvec2 convert(const ClipperLib::IntPoint& point)
	{
		return trimesh::dvec2(DLP_S_2MM(point.X), DLP_S_2MM(point.Y));
	}

	void convertRaw(const ClipperLib::Path& path, DLPPoly& poly)
	{
		poly.clear();
		int size = (int)path.size();
		if (size == 0)
			return;

		poly.resize(size);
		for (int i = 0; i < size; ++i)
		{
			const ClipperLib::IntPoint& point = path.at(i);
			poly[i] = convert(point);
		}
	}

	void convertRaw(const ClipperLib::Paths& paths, DLPPolys& polys)
	{
		polys.clear();
		int size = (int)paths.size();
		if (size == 0)
			return;

		polys.resize(size);
		for (int i = 0; i < size; ++i)
			convertRaw(paths.at(i), polys.at(i));
	}

	DLPData::DLPData()
	{
		impl = new DLPDataImpl();
	}

	DLPData::~DLPData()
	{
		delete impl;
	}

	int DLPData::layers() const
	{
		return (int)impl->layersData.size();
	}

	//ClipperLib::PolyTree* DLPData::layerData(int layer) const
	//{
	//	ClipperLib::PolyTree* tree = nullptr;
	//	if (layer >= 0 && layer < layers())
	//		tree = layersData.at(layer).polygons.splitIntoPolyTree(true);
	//
	//	return tree;
	//}

	bool DLPData::isValid() const
	{
		return layers() > 0;
	}

	void DLPData::traitPolys(int layer, DLPPolys& polys) const
	{
		polys.clear();

		if (layer >= 0 && layer < layers())
		{
			const Polygons& polygons = impl->layersData.at(layer).polygons;
			convertRaw(polygons.paths, polys);
		}
	}

	void DLPData::traitUmPolys(int layer, UmDLPPolys& polys) const
	{
		polys.clear();

		if (layer >= 0 && layer < layers())
		{
			const Polygons& polygons = impl->layersData.at(layer).polygons;

			polys.clear();
			int size = (int)polygons.paths.size();
			if (size == 0)
				return;

			polys.resize(size);
			for (int i = 0; i < size; ++i)
			{
				const ClipperLib::Path& path = polygons.paths.at(i);
				std::vector<trimesh::ivec2> &poly = polys.at(i);

				poly.clear();
				int size = (int)path.size();
				if (size == 0)
					continue;

				poly.resize(size);
				for (int i = 0; i < size; ++i)
				{
					const ClipperLib::IntPoint& point = path.at(i);
					poly[i] = trimesh::ivec2(DLP_S_UM(point.X), DLP_S_UM(point.Y));
				}	
			}
		}
	}

	std::vector<std::vector<trimesh::vec2>> DLPData::traitPolys(int layer) const
	{
		std::vector<std::vector<trimesh::vec2>> polys;

		if (layer >= 0 && layer < layers())
		{
			const Polygons& layerPolygons = impl->layersData.at(layer).polygons;

			auto f = [](const ClipperLib::IntPoint& point)->trimesh::vec2 {
				return trimesh::vec2(DLP_S_2MM(point.X), DLP_S_2MM(point.Y));
			};

			for (const ClipperLib::Path& path : layerPolygons.paths)
			{
				std::vector<trimesh::vec2> contour;
				int n = (int)path.size();
				if (n > 2)
				{
					for (int i = 0; i < n; ++i)
					{
						contour.push_back(f(path.at(i)));
						contour.push_back(f(path.at((i + 1) % n)));
					}

					polys.push_back(contour);
				}
			}
		}

		return polys;
	}

	void DLPData::calculateVolumeAreas(SliceInfo& info) const
	{
		if (!isValid())
			return;

		int layerNum = layers();
		info.details.resize(layerNum);

		double volume = 0.0;
		ClipperLib::cInt lz = 0;
		for (int i = 0; i < layerNum; ++i)
		{
			const DLPLayer& cxLayer = impl->layersData.at(i);
			LayerDetailInfo& detailInfo = info.details.at(i);

			ClipperLib::cInt z = cxLayer.printZ;
			ClipperLib::cInt dz = z - lz;

			if (lz == 0)
			{
				dz *= 2;
			}
			lz = z;

			double maxArea = 0.0;
			double maxDistance = 0.0;
			double totalArea = 0.0;

			int pathCount = (int)cxLayer.polygons.size();
			int maxIndex = -1;
			for (int j = 0; j < pathCount; ++j)
			{
				double area = DLP_S_UM(DLP_S_UM(ClipperLib::Area(cxLayer.polygons.paths.at(j))));
				if (area > maxArea)
				{
					maxArea = area;
					maxIndex = j;
				}
				totalArea += area;
			}
			topomesh::LightOffCircle circle;
			topomesh::lightOffDistance(cxLayer.polygons, circle);
			maxDistance = DLP_S_UM(circle.radius);

			detailInfo.maxArea = maxArea;
			detailInfo.totalArea = totalArea;
			detailInfo.maxDistances = maxDistance;
			volume += DLP_S_2MM(dz) * totalArea / 1000000.0;
		}

		info.volume = volume;
	}

	int DLPData::version()
	{
		return 0;
	}

	bool DLPData::save(std::fstream& out, ccglobal::Tracer* tracer)
	{
		int count = (int)impl->layersData.size();
		ccglobal::cxndSaveT(out, count);

		for (int i = 0; i < count; ++i)
		{
			const DLPLayer& l = impl->layersData.at(i);
			ccglobal::cxndSaveT(out, l.printZ);
			l.polygons.save(out);
		}
		return true;
	}

	bool DLPData::load(std::fstream& in, int ver, ccglobal::Tracer* tracer)
	{
		if (ver == 0)
		{
			int count = 0;
			ccglobal::cxndLoadT(in, count);

			if (count > 0)
			{
				impl->layersData.resize(count);
				for (int i = 0; i < count; ++i)
				{
					DLPLayer& l = impl->layersData.at(i);
					ccglobal::cxndLoadT(in, l.printZ);
					l.polygons.load(in);
				}
			}
			return true;
		}

		return false;
	}
}
