#ifndef CRSLICE_CACHE_SLICE_H_2
#define CRSLICE_CACHE_SLICE_H_2
#include "crslice2/header.h"

#include <list>

namespace ccglobal
{
	class VisualDebugger;
}

namespace crslice2
{
	struct CacheSliceParam
	{
		std::string fileName; //middle scene
		std::string tempDirectory; //debug 

		std::string outName;

		int baselineType = -1;
		std::string blDir = "";
		std::string resultDir = "";
		std::string blName = "";
	};

	/////cache interface
	class CrSliceObject;
	class CrSliceVolume;
	class CrSlicePrintImpl;
	class CRSLICE2_API CrSlicePrint
	{
	public:
		CrSlicePrint();
		~CrSlicePrint();

		CrSlicePrintImpl* impl;
	};

	class CrSliceModelImpl;
	class CRSLICE2_API CrSliceModel
	{
	public:
		CrSliceModel();
		~CrSliceModel();

		void setParameter(const std::string& key, const std::string& value);
		void setPlateInfo(const PlateInfo& plate);
		CrSliceObject* add_object();
		void remove_object(CrSliceObject* object);

		CrSliceModelImpl* impl;
		std::list<CrSliceObject*> m_objects;
	};

	class CrSliceObjectImpl;
	class CRSLICE2_API CrSliceObject
	{
	public:
		CrSliceObject();
		~CrSliceObject();

		void clearParameter();
		void setName(const std::string& name);
		void setParameter(const std::string& key, const std::string& value);
		void setMatrix(const trimesh::xform& matrix);
		void setLayerHeight(const std::vector<double>& layer_heights);
		void setHostID(int64_t id);
		void setVisible(bool visible);

		CrSliceVolume* add_volume();
		void remove_volume(CrSliceVolume* volume);

		CrSliceObjectImpl* impl;
	};

	class CrSliceVolumeImpl;
	class CRSLICE2_API CrSliceVolume
	{
	public:
		CrSliceVolume();
		~CrSliceVolume();

		void clearParameter();
		void setParameter(const std::string& key, const std::string& value);
		void setMatrix(const trimesh::xform& matrix);
		void regenerateID();
		void setMeshData(TriMeshPtr mesh);
		void setSpreadColor(const std::vector<std::string>& colors);
		void setSpreadSeam(const std::vector<std::string>& seams);
		void setSpreadSupport(const std::vector<std::string>& supports);
		void setName(const std::string& name);
		void setModelType(const int model_type);
		void setHostID(int64_t id);

		CrSliceVolumeImpl* impl;
	};

	struct CrSliceResult
	{
		bool success = true;
		std::map<std::string, std::pair<std::string, int64_t>> warnings;

		//statics
		std::vector<double>                                 volumes_per_color_change;
		std::map<size_t, double>                            volumes_per_extruder;
		std::map<size_t, double>                            wipe_tower_volumes_per_extruder;
		//BBS: the flush amount of every filament
		std::map<size_t, double>                            flush_per_filament;
		std::map<int, std::pair<double, double>>  used_filaments_per_role;
		unsigned int                                        total_filamentchanges;
	};

	CRSLICE2_API void update_print_volume_state(CrSliceModel& model, const std::vector<trimesh::dvec2>& shapes, double height);

	CRSLICE2_API CrSliceResult slice(CrSlicePrint& print, CrSliceModel& model, const std::vector<ThumbnailData>& thumbnails,
		const CacheSliceParam& param, ccglobal::Tracer* tracer = nullptr);
}
#endif  // MSIMPLIFY_SIMPLIFY_H
