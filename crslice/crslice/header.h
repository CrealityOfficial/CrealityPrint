#ifndef CRSLICE_HEADER_INTERFACE
#define CRSLICE_HEADER_INTERFACE
#include "crslice/interface.h"
#include <memory>

#include "trimesh2/TriMesh.h"
#include "trimesh2/XForm.h"
#include "trimesh2/TriMesh_algo.h"

#include <fstream>
#include <unordered_map>
#include "ccglobal/tracer.h"

typedef std::shared_ptr<trimesh::TriMesh> TriMeshPtr;

#include "crslice/settings.h"
typedef std::shared_ptr<crslice::Settings> SettingsPtr;

namespace crslice
{

	struct PathTimeParts {
		float OuterWall{ 0.0f };
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

	struct PathParam
	{
		float machine_height;
		float machine_width;
		float machine_depth;
		int printTime;
		float materialLenth = { 0.0f };
		float material_diameter = { 1.75f }; //材料直径
		float material_density = { 1.24f };  //材料密度
		float cost = { 0.0f };
		float weight = { 0.0f };
		float lineWidth;
		float layerHeight;
		bool spiralMode;
		std::string exportFormat;//QString exportFormat;
		std::string	screenSize;//QString screenSize;

		PathTimeParts timeParts;

		int beltType;  // 1 creality print belt  2 creality slicer belt
		float beltOffset;
		float beltOffsetY;
		trimesh::fxform xf4;//cr30 fxform

		bool relativeExtrude;
		PathParam()
		{
			machine_height = 250.0f;
			machine_width = 220.0f;
			machine_depth = 220.0f;
			printTime = 0;
			materialLenth = 0.0f;
			lineWidth = 0.1f;
			layerHeight = 0.1f;
			exportFormat = "png";
			screenSize = "Sermoon D3";
			spiralMode = false;

			timeParts = PathTimeParts();

			beltType = 0;
			beltOffset = 0.0f;
			beltOffsetY = 0.0f;
			xf4 = trimesh::fxform();
			relativeExtrude = false;
		}
	};


	class FDMDebugger
	{
	public:
		virtual ~FDMDebugger() {}

		virtual void tick(const std::string& tag) = 0;

		//for gcode
		virtual void getPathData(const trimesh::vec3 point, float e, int type) = 0;
		virtual void getPathDataG2G3(const trimesh::vec3 point, float i, float j, float e, int type, bool isG2 = true) = 0;
		virtual void setParam(PathParam pathParam) = 0;
		virtual void setLayer(int layer) = 0;
		virtual void setLayers(int layer) = 0;
		virtual void setSpeed(float s) = 0;
		virtual void setTEMP(float temp) = 0;
		virtual void setExtruder(int nr) = 0;
		virtual void setTime(float time) = 0;
		virtual void setFan(float fan) = 0;
		virtual void setZ(float z, float h = -1) = 0;
		virtual void setE(float e) = 0;
		virtual void getNotPath() = 0;

		//for preview image
		virtual void onSupports(int layerIdx, float z, float thickness, const std::vector<std::vector<trimesh::vec3>>& paths) = 0;
		virtual void setSceneBox(const trimesh::box3& box) {};
	};

	class PathData :public crslice::FDMDebugger
	{
	public:
		void tick(const std::string& tag) override {};

		void getPathData(const trimesh::vec3 point, float e, int type) override {};
		void getPathDataG2G3(const trimesh::vec3 point, float i, float j, float e, int type, bool isG2 = true) override {};
		void setParam(PathParam pathParam)override {};
		void setLayer(int layer) override {};
		void setLayers(int layer) override {};
		void setSpeed(float s) override {};
		void setTEMP(float temp) override {};
		void setExtruder(int nr) override {};
		void setTime(float time) override {};
		void setFan(float fan) override {};
		void setZ(float z, float h = -1) override {};
		void setE(float e) override {};
		void getNotPath() override {};

		//for preview image
		void onSupports(int layerIdx, float z, float thickness, const std::vector<std::vector<trimesh::vec3>>& paths) override {};
		void setSceneBox(const trimesh::box3& box) override {};
	};
}
#endif // CRSLICE_HEADER_INTERFACE