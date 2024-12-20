#ifndef GCODEPROCESSHEADER_1632383314974_H
#define GCODEPROCESSHEADER_1632383314974_H

#include "ccglobal/tracer.h"
#include "trimesh2/TriMesh.h"
#include "trimesh2/XForm.h"

namespace gcode
{
    enum class GCodeVisualType
	{
		gvt_speed,
		gvt_structure,
		gvt_extruder,
		gvt_layerHight,  //层高
		gvt_lineWidth,   //线宽
		gvt_flow,        //流量
		gvt_layerTime,   //层时间
		gvt_fanSpeed,    //风扇速度
		gvt_temperature, //温度
		gvt_acc,         //加速度
		gvt_num,
	};

	enum class GProducer
	{
		Unknown,
		OrcaSlicer,
		Cura
	};

	struct TimeParts {
		float OuterWall{0.0f};
		float InnerWall{ 0.0f };
		float Skin{ 0.0f };
		float Support{ 0.0f };
		float SkirtBrim{ 0.0f };
		float Infill{ 0.0f };
		float SupportInfill{ 0.0f };
		float MoveCombing{ 0.0f };
		float MoveRetraction{ 0.0f };
		float PrimeTower{ 0.0f };
	};

	struct GCodeParseInfo {
		float machine_height;
		float machine_width;
		float machine_depth;
		int printTime;
		float materialLenth = { 0.0f };
		float materialDensity;//单位面积密度
		float material_diameter = {1.75f}; //材料直径
		float material_density = { 1.24f };  //材料密度
		float cost = { 0.0f };
		float weight = { 0.0f };
		float lineWidth;
		float layerHeight = {0.0f};
		float unitPrice;
		float filament_weight;
		bool spiralMode;
		bool adaptiveLayers;
		std::string exportFormat;//QString exportFormat;
		std::string	screenSize;//QString screenSize;
		int total_filamentchanges;//change color count
		std::vector<int> extruder_filamentchanges;
		std::vector<std::vector<std::pair<int,int>>> extruder_filamentchanges_index;
		std::vector<std::pair<int,double>> volumes_per_extruder;
		std::vector<std::pair<int, double>> flush_per_filament;
		std::vector<std::pair<int, double>> volumes_per_tower;
		GProducer producer;

		TimeParts timeParts;

		bool have_roles_time;
		std::vector<std::pair<int, float>> roles_time;
		//std::vector<std::pair<int,float>> moves_time;
		std::vector<std::pair<int, float>> layers_time;
	
		int beltType;  // 1 creality print belt  2 creality slicer belt
		float beltOffset;
		float beltOffsetY;
		trimesh::fxform xf4;//cr30 fxform
	
		bool relativeExtrude;
	
		GCodeParseInfo()
		{
			producer = GProducer::Cura;
			machine_height = 250.0f;
			machine_width = 220.0f;
			machine_depth = 220.0f;
			printTime = 0;
			materialLenth = 0.0f;
			materialDensity = 1.0f;
			lineWidth = 0.1f;
			layerHeight = 0.1f;
			unitPrice = 0.3f;
			exportFormat = "png";
			screenSize = "Sermoon D3";
			spiralMode = false;
			total_filamentchanges = 0;

			timeParts = TimeParts();
			cost = 0.0f;
			weight = 0.0f;
			beltType = 0;
			beltOffset = 0.0f;
			beltOffsetY = 0.0f;
			xf4 = trimesh::fxform();
			relativeExtrude = false;
			adaptiveLayers = false;
			have_roles_time = false;

			volumes_per_extruder.clear();
			flush_per_filament.clear();
			volumes_per_tower.clear();

			extruder_filamentchanges.clear();
			extruder_filamentchanges_index.clear();
		}
	};

	class GcodeTracer
	{
	public:
		virtual ~GcodeTracer() {}

		virtual void tick(const std::string& tag) = 0;
		virtual void getPathData(const trimesh::vec3 point, float e, int type, bool isOrca = false, bool isseam = false) = 0;
		virtual void getPathDataG2G3(const trimesh::vec3 point, float i, float j, float e, int type, int p,bool isG2 = true, bool isOrca = false, bool isseam = false) = 0;
		virtual void setParam(GCodeParseInfo& pathParam) = 0;
		virtual void setLayer(int layer) = 0;
		virtual void setLayers(int layer) = 0;
		virtual void setSpeed(float s) = 0;
		virtual void setAcc(float acc) = 0;
		virtual void setTEMP(float temp) = 0;
		virtual void setExtruder(int nr) = 0;
		virtual void setTime(float time) = 0;
		virtual void setFan(float fan) = 0;
		virtual void setZ(float z, float h = -1) = 0;
		virtual void setE(float e) = 0;
		virtual void setWidth(float width) = 0;
		virtual void setLayerHeight(float height) = 0;
		virtual void setLayerPause(int pause) = 0;
		virtual void getNotPath() = 0;
		virtual void set_data_gcodelayer(int layer, const std::string& gcodelayer) = 0;
		virtual void setNozzleColorList(std::string& colorList, std::string& defaultColorList, std::string& defaultType) = 0;
		virtual void writeImages(const std::vector<std::pair<trimesh::ivec2, std::vector<unsigned char>>>& images) =0;

		//for cloud : preview image
		virtual void onSupports(int layerIdx, float z, float thickness, const std::vector<std::vector<trimesh::vec3>>& paths) = 0;
		virtual void setSceneBox(const trimesh::box3& box) {};
	};


    class  SliceLine3D
    {
    public:
        SliceLine3D() {};
        ~SliceLine3D() {};

        trimesh::vec3 start;
        trimesh::vec3 end;
    };

    enum class SliceCompany
    {
        none,creality, cura, prusa, bambu, ideamaker, superslicer, ffslicer, simplify,craftware
    };

}


#endif // GCODEPROCESSHEADER_1632383314974_H