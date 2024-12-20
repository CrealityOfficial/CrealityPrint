#ifndef CRSLICE_SCENE_1668840402293_H_2
#define CRSLICE_SCENE_1668840402293_H_2
#include "crslice2/interface.h"
#include "crslice2/header.h"
#include <vector>

#include <fstream>

namespace crslice2
{
	class CrGroup;
	class CRSLICE2_API CrScene
	{
		friend class CrSlice;
	public:
		CrScene();
		~CrScene();

		std::vector<TriMeshPtr> collectScene();

		int addOneGroup();
		int addObject2Group(int groupID);
		void setOjbectMesh(int groupID, int objectID, TriMeshPtr mesh);
		void setOjbectMeshPaint(int groupID, int objectID, TriMeshPtr mesh, const trimesh::xform& componentXform
			, const std::vector<std::string>& colors2Facets
			, const std::vector<std::string>& seam2Facets
			, const std::vector<std::string>& support2Facets
			, const std::string& objectName
			, const std::vector<double>& layerHeight
			, const int modelType);

		void setGroupOffset(int groupID, trimesh::vec3 offset);
		void setGroupTransform(int groupID, trimesh::xform gxform);
		void setGroupSceneObjectId(int groupID, int64_t sceneObjId);

		void setObjectSettings(int groupID, int objectID, SettingsPtr settings);
		void setGroupSettings(int groupID, SettingsPtr settings);
		void setSceneSettings(SettingsPtr settings);
		void setSceneJsonFile(const std::string& fileName);
		void addSceneParameter(const std::string& key, const std::string& value);
		void setTempDirectory(const std::string& directory);
		void setSliceBLDirectory(const std::string& directory);
		void setBLCompareErrorDirectory(const std::string& directory);
		void setBLName(const std::string&name);
		void release();
		CrGroup* getGroupsIndex(int groupID);

		void setOutputGCodeFileName(const std::string& fileName);
		void setPloygonFileName(const std::string& fileName);
		void setSupportFileName(const std::string& fileName);
		void setAntiSupportFileName(const std::string& fileName);
		void setSeamFileName(const std::string& fileName);
		void setAntiSeamFileName(const std::string& fileName);
		void setcalibParams(Calib_Params& _calibParams);
		bool valid();

		void save(const std::string& fileName);
		void load(const std::string& fileName);

		//save ploygon
		void savePloygons(const std::vector<std::vector<trimesh::vec2>>& polys);
		void savePloygons(const std::vector<std::vector<trimesh::vec2>>& polys, const std::string filename);
		void setOjbectExclude(int groupID, int objectID, const std::string& fileName, std::vector<trimesh::vec3>& outline_ObjectExclude);
		void makeSureParameters();

		void recordObjectIdInfo(int64_t sceneObjId, size_t sliceObjId);
		int64_t getSceneObjectIdBySliceObjId(size_t sliceObjId);

	public:
		std::vector<CrGroup*> m_groups;
		SettingsPtr m_settings;
		std::vector<SettingsPtr> m_extruders;
		bool machine_center_is_zero;

		std::string m_gcodeFileName;
		std::string m_ploygonFileName;
		std::string m_supportFile;
		std::string m_antiSupportFile;
		std::string m_seamFile;
		std::string m_antiSeamFile;
	
		std::string m_tempDirectory;
		std::string m_sliceBLDirectory;
		std::string m_BLCompareErrorDirectory;
		std::string m_blName = "";
		std::vector<std::string> m_Object_Exclude_FileName;
		Calib_Params m_calibParams;

		std::vector<ThumbnailData> thumbnailDatas;

		std::map<int, PlateInfo> plates_custom_gcodes;

		bool m_isBBLPrinter;
		int m_plate_index;
		int m_unittest_type;   //0:none,1:ganerate,2:compare,3:update

		std::map<int64_t, size_t> m_sceneObjectIdWithSliceObjectIdMap;
	};

	class SceneCreator
	{
	public:
		virtual ~SceneCreator() {}

		virtual CrScene* createOrca(ccglobal::Tracer* tracer = nullptr) = 0;
	};

	typedef std::shared_ptr<crslice2::CrScene> CrScenePtr;
}

#endif // CRSLICE_SCENE_1668840402293_H_2