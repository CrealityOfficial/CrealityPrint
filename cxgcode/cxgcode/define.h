#pragma once

#ifndef CXGCODE_DEFINE_HPP_
#define CXGCODE_DEFINE_HPP_

#include <QString>
#include "trimesh2/XForm.h"

#define USE_GCODE_PREVIEW_SIMPLE 1

#define SIMPLE_GCODE_IMPL 3    // 0 cpu tri soup  1 cpu indices 2 gpu tri soup 3 gpu indices

#define INDEX_START_AT_ONE 1

#ifndef  INDEX_START_AT_ONE
#define INDEX_START_AT_ONE 0
#endif // ! INDEX_START_AT_ONE

namespace cxgcode {
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
		gvt_num,
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
		float materialLenth;
		float materialDensity;
		float materialDiameter;
		float lineWidth;
		float layerHeight;
		float unitPrice;
		bool spiralMode;
		QString exportFormat;
		QString screenSize;

		TimeParts timeParts;
	
		int beltType;  // 1 creality print belt  2 creality slicer belt
		float beltOffset;
		float beltOffsetY;
		trimesh::fxform xf4;//cr30 fxform
	
		bool relativeExtrude;
	
		GCodeParseInfo()
		{
			machine_height = 250;
			machine_width = 220;
			machine_depth = 220;
			printTime = 0;
			materialLenth = 0.0f;
			materialDensity = 1.0f;
			materialDiameter = 0.0f;
			lineWidth = 0.1f;
			layerHeight = 0.1f;
			unitPrice = 0.3f;
			exportFormat = "png";
			screenSize = "Sermoon D3";
			spiralMode = false;

			timeParts = TimeParts();
	
			beltType = 0;
			beltOffset = 0.0f;
			beltOffsetY = 0.0f;
			xf4 = trimesh::fxform();
			relativeExtrude = false;
		}
	};
}  // namespace cxgcode

#endif  // CXGCODE_DEFINE_HPP_
