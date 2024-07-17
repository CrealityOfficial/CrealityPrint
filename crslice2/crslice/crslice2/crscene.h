#ifndef CRSLICE_SCENE_1668840402293_H_2
#define CRSLICE_SCENE_1668840402293_H_2
#include "crslice2/interface.h"
#include "crslice2/header.h"
#include <vector>

#include <fstream>

namespace crslice2
{
	enum class CalibMode : int {
		Calib_None = 0,
		Calib_PA_Line,
		Calib_PA_Pattern,
		Calib_PA_Tower,
		Calib_Flow_Rate,
		Calib_Temp_Tower,
		Calib_Vol_speed_Tower,
		Calib_VFA_Tower,
		Calib_Retraction_tower,
		Calib_Retraction_tower_speed,
		Calib_Limit_Speed,
		Calib_Limit_Acceleration,
		Calib_Speed_Tower,
		Calib_Acceleration_Tower,
		Calib_Arc2Lerance,
		Calib_Accel2Decel,
		Calib_X_Y_Jerk,
		Calib_Fan_Speed
	};

	struct Calib_Params
	{
		Calib_Params() {
			start = 0;
			end = 0;
			step = 0;
			print_numbers = false;
			mode = CalibMode::Calib_None;
		}
		double start, end, step;
		double highStep;
		bool print_numbers=false;
		CalibMode mode;
	};

	enum Plate_Mode
	{
		Undef,
		SingleExtruder,
		MultiAsSingle,
		MultiExtruder
	};

	enum Plate_Type
	{
		ColorChange,
		PausePrint,
		ToolChange,
		Template,
		Custom,
		Unknown,
	};

	struct Plate_Item
	{
		double      print_z;
		Plate_Type        type;
		int         extruder;
		std::string color;
		std::string extra;
	};

	struct ThumbnailData
	{
		unsigned int width;
		unsigned int height;
		unsigned int pos_s; //for cr_png
		unsigned int pos_e; //for cr_png
		unsigned int pos_h; //for cr_png
		std::vector<unsigned char> pixels;
	};

	struct plateInfo
	{
		Plate_Mode mode = Undef;
		std::vector<Plate_Item> gcodes;
	};

	class CrGroup;
	class CRSLICE2_API CrScene
	{
		friend class CrSlice;
	public:
		CrScene();
		~CrScene();

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

		std::map<int, plateInfo> plates_custom_gcodes;

		bool m_isBBLPrinter;
		int m_plate_index;
		int m_unittest_type;   //0:none,1:ganerate,2:compare,3:update
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