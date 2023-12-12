#include "crslice/load.h"

namespace crslice
{
	void _load(std::fstream& in, CrPolygon& poly)
	{
		ccglobal::cxndLoadVectorT(in, poly);
	}

	void _save(std::fstream& out, const CrPolygon& poly)
	{
		ccglobal::cxndSaveVectorT(out, poly);
	}

	void _load(std::fstream& in, CrPolygons& polys)
	{
		int size = 0;
		ccglobal::cxndLoadT(in, size);
		if (size > 0)
		{
			polys.resize(size);
			for(int i = 0; i < size; ++i)
				_load(in, polys.at(i));
		}
	}

	void _save(std::fstream& out, const CrPolygons& polys)
	{
		int size = (int)polys.size();
		ccglobal::cxndSaveT(out, size);
		for (int i = 0; i < size; ++i)
			_save(out, polys.at(i));
	}

	void _load(std::fstream& in, CrSliceLayerPart& part)
	{
		_load(in, part.outline);
		_load(in, part.inner_area);
		cxndLoadComplexVectorT(in, part.skin_parts);
		cxndLoadComplexVectorT(in, part.wall_toolpaths);
		cxndLoadComplexVectorT(in, part.infill_wall_toolpaths);
	}

	void _save(std::fstream& out, const CrSliceLayerPart& part)
	{
		_save(out, part.outline);
		_save(out, part.inner_area);
		cxndSaveComplexVectorT(out, part.skin_parts);
		cxndSaveComplexVectorT(out, part.wall_toolpaths);
		cxndSaveComplexVectorT(out, part.infill_wall_toolpaths);
	}

	void _load(std::fstream& in, CrSkinPart& skin)
	{
		_load(in, skin.outline);
		cxndLoadComplexVectorT(in, skin.inset_paths);
		_load(in, skin.skin_fill);
		_load(in, skin.roofing_fill);
		_load(in, skin.top_most_surface_fill);
		_load(in, skin.bottom_most_surface_fill);
	}

	void _save(std::fstream& out, const CrSkinPart& skin)
	{
		_save(out, skin.outline);
		cxndSaveComplexVectorT(out, skin.inset_paths);
		_save(out, skin.skin_fill);
		_save(out, skin.roofing_fill);
		_save(out, skin.top_most_surface_fill);
		_save(out, skin.bottom_most_surface_fill);
	}

	int SerailSlicedData::version()
	{
		return 0;
	}

	bool SerailSlicedData::save(std::fstream& out, ccglobal::Tracer* tracer)
	{
		ccglobal::cxndSaveT(out, data.z);
		_save(out, data.open_polygons);
		_save(out, data.polygons);

		return true;
	}

	bool SerailSlicedData::load(std::fstream& in, int ver, ccglobal::Tracer* tracer)
	{
		if (ver == 0)
		{
			ccglobal::cxndLoadT(in, data.z);
			_load(in, data.open_polygons);
			_load(in, data.polygons);

			return true;
		}
		return false;
	}

	int SerailCrSliceLayer::version()
	{
		return 0;
	}

	bool SerailCrSliceLayer::save(std::fstream& out, ccglobal::Tracer* tracer)
	{
		cxndSaveComplexVectorT(out, layer.parts);
		return true;
	}

	bool SerailCrSliceLayer::load(std::fstream& in, int ver, ccglobal::Tracer* tracer)
	{
		if (ver == 0)
		{
			cxndLoadComplexVectorT(in, layer.parts);
			return true;
		}

		return false;
	}

	int SerailCrSkeletal::version()
	{
		return 0;
	}

	bool SerailCrSkeletal::save(std::fstream& out, ccglobal::Tracer* tracer)
	{
		_save(out, polygons);
		ccglobal::cxndSaveT(out, param);

		return true;
	}

	bool SerailCrSkeletal::load(std::fstream& in, int ver, ccglobal::Tracer* tracer)
	{
		if (ver == 0)
		{
			_load(in, polygons);
			ccglobal::cxndLoadT(in, param);
			return true;
		}

		return false;
	}

	int SerailDiscretize::version()
	{
		return 0;
	}

	bool SerailDiscretize::save(std::fstream& out, ccglobal::Tracer* tracer)
	{
		ccglobal::cxndSaveT(out, type);
		_save(out, points);
		return true;
	}

	bool SerailDiscretize::load(std::fstream& in, int ver, ccglobal::Tracer* tracer)
	{
		if (ver == 0)
		{
			ccglobal::cxndLoadT(in, type);
			_load(in, points);
			return true;
		}

		return false;
	}

	/// file name
	std::string crsliceddata_name(const std::string& root, int meshId, int layer)
	{
		char buffer[1024] = { 0 };

		sprintf(buffer, "%s/mesh%d_layer%d.crsliceddata", root.c_str(), meshId, layer);
		return std::string(buffer);
	}

	std::string crslicelayer_name(const std::string& root, int meshId, int layer)
	{
		char buffer[1024] = { 0 };

		sprintf(buffer, "%s/mesh%d_layer%d.crslicelayer", root.c_str(), meshId, layer);
		return std::string(buffer);
	}

	std::string crsliceskeletal_name(const std::string& root, int index)
	{
		char buffer[1024] = { 0 };

		sprintf(buffer, "%s/%d.skeletal", root.c_str(), index);
		return std::string(buffer);
	}
}