#ifndef _GCODE_NULLSPACE_SLICEMODELBUILDER_1590033412412_H
#define _GCODE_NULLSPACE_SLICEMODELBUILDER_1590033412412_H
#include "crslice/interface.h"
#include <float.h>
#include <vector>
#include <map>
#include "gcode/gcode/sliceline.h"
#include "crslice/gcode/sliceresult.h"
#include "crslice/gcode/define.h"
#include "crslice/gcode/header.h"
#include "ccglobal/tracer.h"
#include "trimesh2/Box.h"

namespace gcode
{
	struct G2G3Info
	{
		float f;
		float e;
		float x;
		float y;
		float i;
		float j;
		int p;
		float currentE;
		bool isG2;
		bool bIsTravel;
	};

	//struct GCodeParseInfo;
	struct GCodeStructBaseInfo
	{
		std::vector<int> layerNumbers;
		std::vector<int> steps;
		int totalSteps;
		int layers;
		float minTimeOfLayer;   //单层最短时间
		float maxTimeOfLayer;   //单层最长时间

		float minFlowOfStep;  //单步最小流量，非零
		float maxFlowOfStep;  //单步最大流量

		float minLineWidth;
		float maxLineWidth;

		float minLayerHeight;
		float maxLayerHeight;

		float minTemperature;
		float maxTemperature;

		float minAcc;
		float maxAcc;

		trimesh::box3 gCodeBox;
		int nNozzle;
		float speedMin;
		float speedMax;

		GCodeStructBaseInfo()
			: speedMin(FLT_MAX)
			, speedMax(FLT_MIN)
			, layers(0)
			, nNozzle(0)
			, totalSteps(0)
			, minTimeOfLayer(FLT_MAX)
			, maxTimeOfLayer(FLT_MIN)
			, minFlowOfStep(FLT_MAX)
			, maxFlowOfStep(FLT_MIN)
			, minLineWidth(FLT_MAX)
			, maxLineWidth(FLT_MIN)
			, minLayerHeight(FLT_MAX)
			, maxLayerHeight(FLT_MIN)
			, minTemperature(FLT_MAX)
			, maxTemperature(FLT_MIN)
			, minAcc(FLT_MAX)
			, maxAcc(FLT_MIN)
		{
		}
	};

	struct GcodeTemperature
	{
		float bedTemperature{ 0.0f }; //平台温度
		float temperature{ 0.0f };  //喷嘴温度
		float camberTemperature{ 0.0f };  //腔体温度
	};

	struct GcodeFan
	{
		float fanSpeed{ 0.0f }; //风扇速度
		float camberSpeed{ 0.0f };  //腔体风扇速度
		float fanSpeed_1{ 0.0f };  //子风扇
	};

	struct GcodeLayerInfo
	{
		float layerHight{ 0.0f }; //层高
		float width{ 0.0f };  //线宽
		float flow{ 0.0f }; //体积流量
	};

	struct GCodeMove
	{
		int start;
		float speed;
		float acc;
		SliceLineType type;
		float e;  //流量
		int extruder;
	};

	class CRSLICE_API GCodeStruct
	{
	public:
		GCodeStruct();
		~GCodeStruct();

		void buildFromResult(SliceResultPointer result, const GCodeParseInfo& info, GCodeStructBaseInfo& baseInfo, std::vector<std::vector<int>>& stepIndexMaps, ccglobal::Tracer* tracer = nullptr);
		void buildFromResult(GCodeParseInfo& info, GCodeStructBaseInfo& baseInfo, std::vector<std::vector<int>>& stepIndexMaps, ccglobal::Tracer* tracer = nullptr);
		void buildFromResult(gcode::SliceResult* result, ccglobal::Tracer* tracer = nullptr);

		std::vector<trimesh::vec3> m_positions;
		std::vector<GCodeMove> m_moves;  //流量、速度..

		std::vector <GcodeTemperature> m_temperatures;//温度设置值
		std::vector<int> m_temperatureIndex;//温度步进索引
		std::vector <GcodeFan> m_fans;//风扇设置值
		std::vector<int> m_fanIndex;//风扇步进索引
		std::vector <GcodeLayerInfo> m_gcodeLayerInfos;  //层高、线宽设置值
		std::vector<int> m_layerInfoIndex;  //层高、线宽 步进索引

		std::vector<float> m_layerHeights;
		std::vector<int> m_pause;

		std::map<int,float> m_layerTimes;  //每层时间
		//std::map<int, float> m_layerTimeLogs;  //每层时间对数

		std::vector<int> m_zSeams;
		std::vector<int> m_retractions;

		std::vector<std::string> m_nozzleColorList;
		std::vector<std::string> m_nozzleColorListDefault;
		std::vector<std::string> m_nozzleTypeDefault;
		gcode::GCodeParseInfo& getParam();
		void getPathData(const trimesh::vec3 point, float e, int type, bool fromGcode = false, bool isOrca = false, bool isseam =false);
		void getPathDataG2G3(const trimesh::vec3 point, float i, float j, float e, int type, int p, bool isG2 = true,bool fromGcode = false, bool isOrca = false, bool isseam=false);
		void setParam(gcode::GCodeParseInfo& pathParam);
		void setLayer(int layer);
		void setSpeed(float s);
		void setAcc(float acc);
		void setTEMP(float temp);
		void setExtruder(int nr);
		void setFan(float fan);
		void setZ(float z, float h);
		void setZ_from_gcode(float z);
		void setE(float e);
		void setWidth(float width);
		void setLayerHeight(float height);
		void setLayerPause(int pause);
		void setTime(float time);
		void getNotPath();
		void setNozzleColorList(std::string& colorList, std::string& defaultColorList, std::string& defaultType);
	private:
		void processLayer(const std::string& layerCode, int layer, std::vector<int>& stepIndexMap);
		void processStep(const std::string& stepCode, int nIndex, std::vector<int>& stepIndexMap);
		void processG01(const std::string& G01Str, int nIndex, std::vector<int>& stepIndexMap,bool isG2G3 =false, bool fromGcode = false, bool isOrca = false, bool isseam = false);
		void processG01_sub(SliceLineType tempType, double tempEndE, trimesh::vec3 tempEndPos, bool havaXYZ, int nIndex, std::vector<int>& stepIndexMap, bool isG2G3, bool fromGcode = false, bool isOrca = false, bool isseam = false);
		void processG23(const std::string& G23Str, int nIndex, std::vector<int>& stepIndexMap, bool isG2 = false, bool fromGcode = false, bool isOrca = false, bool isseam = false);
		void processG23_sub(G2G3Info info, int nIndex, std::vector<int>& stepIndexMap, bool isG2 = false, bool fromGcode = false, bool isOrca = false, bool isseam = false);
		void processSpeed(float speed);

		void processPrefixCode(const std::string& stepCod);
		void checkoutFan(const std::string& stepCod);
		void checkoutTemperature(const std::string& stepCode);
		void checkoutLayerInfo(const std::string& stepCode,int layer);
		void checkoutLayerHeight(const std::vector<std::string>& layerLines);
	protected:
		SliceLineType  tempCurrentType;
		int tempNozzleIndex;
		double tempCurrentE;
		float tempCurrentTime{ 0.0f };
		float tempCurrentZ{ 0.0f };
		float belowZ{ 0.0f };//上层层高
		trimesh::vec3 tempCurrentPos;
		float tempSpeed;
		float tempAcc;
		float tempSpeedMax{ 0.0f };//最大速度限制
		int tempTempIndex{ 0 };; //当前温度索引
		bool layerNumberParseSuccess;

		int nIndex = 0;

		int startNumber = 0;
		std::vector<std::vector<int>> m_stepIndexMaps;

		GCodeParseInfo parseInfo;
		GCodeStructBaseInfo tempBaseInfo;

		ccglobal::Tracer* m_tracer;
	};
}
#endif // _GCODE_NULLSPACE_SLICEMODELBUILDER_1590033412412_H
