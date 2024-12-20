#include "crslice/gcode/gcodedata.h"
#include "crslice/gcode/sliceresult.h"
#include "crslice/gcode/thumbnail.h"
#include <regex>

namespace gcode
{
	bool gcodeGenerate(const std::string& fileName, gcode::GCodeStruct& _gcodeStruct, ccglobal::Tracer* tracer)
	{
		gcode::SliceResult* result=new gcode::SliceResult();
		result->load(fileName, tracer);
		_gcodeStruct.buildFromResult(result, tracer);
		return true;
	}

	bool _SaveGCode(const std::string& fileName, const std::string& previewImageString, const std::vector<std::string>& layerString, const std::string& prefixString, const std::string& tailCode)
	{
		std::ofstream out(fileName);

		if (!out.is_open())
		{
			out.close();
			return false;
		}

		///添加预览图信息
		if (!previewImageString.empty())
		{
			out.write(previewImageString.c_str(), previewImageString.size());
			out.write("\r\n", 2); //in << endl;
		}

		//添加打印时间等信息
		if (!prefixString.empty())
		{
			out.write(prefixString.c_str(), prefixString.size());
			out.write("\n", 1);
		}

		for (const std::string& layerLine : layerString)
		{
			out.write(layerLine.c_str(), layerLine.size());
			out.write("\n", 1);
		}

		if (!tailCode.empty())
		{
			out.write(tailCode.c_str(), tailCode.size());
			out.write("\n", 1);
		}
		out.close();
		return true;
	}

	bool  regex_match(std::string& gcodeStr, std::string key, std::smatch& sm)
	{
		std::string temp2 = ".*" + key + ":(\\s{0,}[0-9]{0,8}).*";
		std::string temp1 = ".*" + key + ":(\\s{0,}[-]{0,1}[0-9]{0,8}\\.[0-9]{0,8}).*";
		if (std::regex_match(gcodeStr, sm, std::regex(temp1.c_str())) ||
			std::regex_match(gcodeStr, sm, std::regex(temp2.c_str())))
		{
			return true;
		}
		return false;
	}

	bool  regex_match_float(std::string& gcodeStr, std::string key, std::smatch& sm)
	{
		std::string temp1 = ".*" + key + ":(\\s{0,}[-]{0,1}[0-9]{0,8}\\.[0-9]{0,8}).";
		std::string temp2 = ".*" + key + ":(\\s{0,}[-]{0,1}[0-9]{0,8}\\.[0-9]{0,8}).*";
		if (std::regex_match(gcodeStr, sm, std::regex(temp1.c_str())) ||
			std::regex_match(gcodeStr, sm, std::regex(temp2.c_str())))
		{
			return true;
		}
		return false;
	}

	bool  regex_match_time(std::string& gcodeStr, std::smatch& sm, gcode::GCodeParseInfo& parseInfo)
	{
		if (std::regex_match(gcodeStr, sm, std::regex(".*OuterWall Time:(\\s{0,}[0-9]{0,8}).*")))
		{
			std::string tStr = sm[1];
			float tmp = atof(tStr.c_str());
			parseInfo.timeParts.OuterWall = tmp;
		}
		if (std::regex_match(gcodeStr, sm, std::regex(".*InnerWall Time:(\\s{0,}[0-9]{0,8}).*")))
		{
			std::string tStr = sm[1];
			float tmp = atof(tStr.c_str());
			parseInfo.timeParts.InnerWall = tmp;
		}
		if (std::regex_match(gcodeStr, sm, std::regex(".*Skin Time:(\\s{0,}[0-9]{0,8}).*")))
		{
			std::string tStr = sm[1];
			float tmp = atof(tStr.c_str());
			parseInfo.timeParts.Skin = tmp;
		}
		if (std::regex_match(gcodeStr, sm, std::regex(".*Support Time:(\\s{0,}[0-9]{0,8}).*")))
		{
			std::string tStr = sm[1];
			float tmp = atof(tStr.c_str());
			parseInfo.timeParts.Support = tmp;
		}
		if (std::regex_match(gcodeStr, sm, std::regex(".*SkirtBrim Time:(\\s{0,}[0-9]{0,8}).*")))
		{
			std::string tStr = sm[1];
			float tmp = atof(tStr.c_str());
			parseInfo.timeParts.SkirtBrim = tmp;
		}
		if (std::regex_match(gcodeStr, sm, std::regex(".*Infill Time:(\\s{0,}[0-9]{0,8}).*")))
		{
			std::string tStr = sm[1];
			float tmp = atof(tStr.c_str());
			parseInfo.timeParts.Infill = tmp;
		}
		if (std::regex_match(gcodeStr, sm, std::regex(".*InfillSupport Time:(\\s{0,}[0-9]{0,8}).*")))
		{
			std::string tStr = sm[1];
			float tmp = atof(tStr.c_str());
			parseInfo.timeParts.SupportInfill = tmp;
		}
		if (std::regex_match(gcodeStr, sm, std::regex(".*Combing Time:(\\s{0,}[0-9]{0,8}).*")))
		{
			std::string tStr = sm[1];
			float tmp = atof(tStr.c_str());
			parseInfo.timeParts.MoveCombing = tmp;
		}
		if (std::regex_match(gcodeStr, sm, std::regex(".*Retraction Time:(\\s{0,}[0-9]{0,8}).*")))
		{
			std::string tStr = sm[1];
			float tmp = atof(tStr.c_str());
			parseInfo.timeParts.MoveRetraction = tmp;
		}
		if (std::regex_match(gcodeStr, sm, std::regex(".*PrimeTower Time:(\s{0,}[0-9]{0,8}).*")))
		{
			std::string tStr = sm[1];
			float tmp = atof(tStr.c_str());
			parseInfo.timeParts.PrimeTower = tmp;
		}

		return true;
	}

	void getImage(std::string p, gcode::SliceResult* result)
	{
		std::string tail1 = p;

		size_t pos = p.find(";");
		if (pos > 0 && pos < p.size())
		{
			tail1 = p.substr(pos, p.size());
			tail1.erase(std::remove(tail1.begin(), tail1.end(), ' '), tail1.end());
			tail1.erase(std::remove(tail1.begin(), tail1.end(), ';'), tail1.end());
		}

		std::vector<std::string> prevData;
		prevData.push_back(tail1);
		std::vector<unsigned char> decodeData;
		gcode::thumbnail_base2image(prevData, decodeData);

		if (decodeData.size())
		{
			result->previewsData.push_back(decodeData);
		}
	}

	void parseGCodeInfo(gcode::SliceResult* result, gcode::GCodeParseInfo& parseInfo)
	{
		std::string gcodeStr = result->prefixCode();

		std::replace(gcodeStr.begin(), gcodeStr.end(), '\n', ' ');
		std::replace(gcodeStr.begin(), gcodeStr.end(), '\r', ' ');
		std::smatch sm;

		result->previewsData.clear();
		if (gcodeStr.find("jpg") != std::string::npos 
			|| gcodeStr.find("png") != std::string::npos
			|| gcodeStr.find("bmp") != std::string::npos
			|| gcodeStr.find("thumbnail") != std::string::npos)
		{
			if (std::regex_match(gcodeStr, sm, std::regex(".*jpg begin(.*)jpg end.*jpg begin(.*)jpg end.*"))) //jpg
			{
				std::string strTemp = gcodeStr;
				getImage(sm[1], result);

				int pos = strTemp.find(sm[1]);
				if (pos != std::string::npos)
				{
					std::string str1 = strTemp.substr(0, pos);
					std::string str2 = strTemp.substr(pos + sm[1].length(), strTemp.length());
					strTemp = str1 + str2;
				}

				if (sm.size() > 2)
				{
					getImage(sm[2], result);

					int pos = strTemp.find(sm[2]);
					if (pos != std::string::npos)
					{
						std::string str1 = strTemp.substr(0, pos);
						std::string str2 = strTemp.substr(pos + sm[2].length(), strTemp.length());
						strTemp = str1 + str2;
					}

				}
				gcodeStr = strTemp;
			}
			else  if (std::regex_match(gcodeStr, sm, std::regex(".*png begin(.*)png end.*png begin(.*)png end.*"))) //png
			{
				std::string strTemp = gcodeStr;

				getImage(sm[1], result);

				int pos = strTemp.find(sm[1]);
				if (pos != std::string::npos)
				{
					std::string str1 = strTemp.substr(0, pos);
					std::string str2 = strTemp.substr(pos + sm[1].length(), strTemp.length());
					strTemp = str1 + str2;
				}

				if (sm.size() > 2)
				{
					getImage(sm[2], result);

					int pos = strTemp.find(sm[2]);
					if (pos != std::string::npos)
					{
						std::string str1 = strTemp.substr(0, pos);
						std::string str2 = strTemp.substr(pos + sm[2].length(), strTemp.length());
						strTemp = str1 + str2;
					}

				}

				gcodeStr = strTemp;
			}
			else  if (std::regex_match(gcodeStr, sm, std::regex(".*bmp begin(.*)bmp end.*bmp begin(.*)bmp end.*"))) //bmp
			{
				std::string strTemp = gcodeStr;
				getImage(sm[1], result);

				int pos = strTemp.find(sm[1]);
				if (pos != std::string::npos)
				{
					std::string str1 = strTemp.substr(0, pos);
					std::string str2 = strTemp.substr(pos + sm[1].length(), strTemp.length());
					strTemp = str1 + str2;
				}

				if (sm.size() > 2)
				{
					getImage(sm[2], result);

					int pos = strTemp.find(sm[2]);
					if (pos != std::string::npos)
					{
						std::string str1 = strTemp.substr(0, pos);
						std::string str2 = strTemp.substr(pos + sm[2].length(), strTemp.length());
						strTemp = str1 + str2;
					}

				}
				gcodeStr = strTemp;
			}
			if (std::regex_match(gcodeStr, sm, std::regex(".*thumbnail begin(.*)thumbnail end.*"))) //thumbnail
			{
				std::string strTemp = gcodeStr;
				getImage(sm[1], result);

				int pos = strTemp.find(sm[1]);
				if (pos != std::string::npos)
				{
					std::string str1 = strTemp.substr(0, pos);
					std::string str2 = strTemp.substr(pos + sm[1].length(), strTemp.length());
					strTemp = str1 + str2;
				}
				gcodeStr = strTemp;
			}
		}

		if (gcodeStr.find("TIME") != std::string::npos)
			if (std::regex_match(gcodeStr, sm, std::regex(".*TIME:\\s{0,}([0-9]{0,8}).*"))) ////get print time
		{
			std::string tStr = sm[1];
			int tmp = atoi(tStr.c_str());
			parseInfo.printTime = tmp;

			regex_match_time(gcodeStr, sm, parseInfo);
		}
		
		if (gcodeStr.find("Filament used") != std::string::npos)
			if (std::regex_match(gcodeStr, sm, std::regex(".*Filament used:\\s{0,}([0-9]{0,8}\\.[0-9]{0,8}).*"))) ////get print time
		{
			std::string tStr = sm[1];
			float tmp = atof(tStr.c_str());
			parseInfo.materialLenth = tmp;
		}

		if (gcodeStr.find("machine belt offset") != std::string::npos)
			if (regex_match(gcodeStr, "machine belt offset", sm))
		{
			std::string tStr = sm[1];
			float tmp = atof(tStr.c_str());
			parseInfo.beltOffset = tmp;
		}

		if (gcodeStr.find("machine belt offset Y") != std::string::npos)
			if (regex_match(gcodeStr, "machine belt offset Y", sm))
		{
			std::string tStr = sm[1];
			float tmp = atof(tStr.c_str());
			parseInfo.beltOffsetY = tmp;
		}

		bool hasMachineSpaceBox = false;
		if (gcodeStr.find("Machine Height") != std::string::npos)
			if (regex_match(gcodeStr, "Machine Height", sm))
			{
				std::string tStr = sm[1];
				float tmp = atof(tStr.c_str());
				parseInfo.machine_height = tmp;
			}
			else
				hasMachineSpaceBox = true;

		if (gcodeStr.find("Machine Width") != std::string::npos)
			if (regex_match(gcodeStr, "Machine Width", sm))
			{
				std::string tStr = sm[1];
				float tmp = atof(tStr.c_str());
				parseInfo.machine_width = tmp;
			}
			else
				hasMachineSpaceBox = true;

		if (gcodeStr.find("Machine Depth") != std::string::npos)
			if (regex_match(gcodeStr, "Machine Depth", sm))
			{
				std::string tStr = sm[1];
				float tmp = atof(tStr.c_str());
				parseInfo.machine_depth = tmp;
			}
			else
				hasMachineSpaceBox = true;

		if (hasMachineSpaceBox)
		{//获取模型尺寸信息
			if (gcodeStr.find("MAXX") != std::string::npos)
				if (regex_match(gcodeStr, "MAXX", sm))
				{
					std::string tStr = sm[1];
					parseInfo.machine_width = atof(tStr.c_str()) + 20; //gap
				}

			if (gcodeStr.find("MAXY") != std::string::npos)
				if (regex_match(gcodeStr, "MAXY", sm))
				{
					std::string tStr = sm[1];
					parseInfo.machine_depth = atof(tStr.c_str()) + 20; //gap
				}

			if (gcodeStr.find("MAXZ") != std::string::npos)
				if (regex_match(gcodeStr, "MAXZ", sm))
				{
					std::string tStr = sm[1];
					parseInfo.machine_height = atof(tStr.c_str()) + 20; //gap
				}
		}

		int pos1 = gcodeStr.find("M82");// absolute extrusion mode
		int pos2 = gcodeStr.find("M83");//relative extrusion mode
		int pos3 = gcodeStr.find("G90");// absolute extrusion mode
		int pos4 = gcodeStr.find("G91");//relative extrusion mode

		if (std::max(pos1, pos2) > std::max(pos3, pos4)) //M82 M83
		{
			if (pos2 != std::string::npos && (pos2 > pos1))
			{
				parseInfo.relativeExtrude = true;
			}
		}
		else //M90 M91
		{
			if (pos4 != std::string::npos && (pos4 > pos3))
			{
				parseInfo.relativeExtrude = true;
			}
		}


		//int ipos1 = gcodeStr.find("M82");
		//int ipos2 = gcodeStr.find("M83");
		//if (ipos2 != std::string::npos && (ipos2 > ipos1))
		//{
		//	parseInfo.relativeExtrude = true;
		//}

		//ipos1 = gcodeStr.find("G90");
		//ipos2 = gcodeStr.find("G91");
		//if (ipos2 != std::string::npos && (ipos2 > ipos1))
		//{
		//	parseInfo.relativeExtrude = true;
		//}

		//float material_diameter = 1.75;
		//float material_density = 1.24;
		if (gcodeStr.find("Material Diameter") != std::string::npos)
			if (regex_match(gcodeStr, "Material Diameter", sm))
			{
				std::string tStr = sm[1];
				parseInfo.material_diameter = atof(tStr.c_str()); //gap
			}
		if (gcodeStr.find("Material Density") != std::string::npos)
			if (regex_match(gcodeStr, "Material Density", sm))
			{
				std::string tStr = sm[1];
				parseInfo.material_density = atof(tStr.c_str()); //gap
			}

		//单位面积密度
		parseInfo.materialDensity = PI * (parseInfo.material_diameter * 0.5) * (parseInfo.material_diameter * 0.5) * parseInfo.material_density;

		float filament_cost = 0.0;
		if (gcodeStr.find("Filament Cost") != std::string::npos)
			if (regex_match(gcodeStr, "Filament Cost", sm))
			{
				std::string tStr = sm[1];
				filament_cost = atof(tStr.c_str()); //gap
			}
		float filament_weight = 0.0;
		if (gcodeStr.find("Filament Weight") != std::string::npos)
			if (regex_match(gcodeStr, "Filament Weight", sm))
			{
				std::string tStr = sm[1];
				filament_weight = atof(tStr.c_str()); //gap
			}

		float filament_length = filament_weight / parseInfo.materialDensity;
		parseInfo.unitPrice = filament_cost / filament_length;

		parseInfo.lineWidth = 0.4;
		if (gcodeStr.find("Out Wall Line Width") != std::string::npos)
			if (regex_match(gcodeStr, "Out Wall Line Width", sm))
			{
				std::string tStr = sm[1];
				parseInfo.lineWidth = atof(tStr.c_str()); //gap
			}

		parseInfo.exportFormat = "jpg";
		int ipos = gcodeStr.find("Preview Img Type");
		if (ipos != std::string::npos)
		{
			parseInfo.exportFormat = gcodeStr.substr(ipos + 17, 3);
		}

		parseInfo.layerHeight = 0.0;
		if (gcodeStr.find("Layer Height") != std::string::npos
			|| gcodeStr.find("Layer height") != std::string::npos)
		{
			if (regex_match(gcodeStr, "Layer Height", sm)
				|| regex_match(gcodeStr, "Layer height", sm))
			{
				std::string tStr = sm[1];
				parseInfo.layerHeight = atof(tStr.c_str()); //gap
				//兼容老的
				if (parseInfo.layerHeight >= 50)
					parseInfo.layerHeight = parseInfo.layerHeight / 1000.0f;
			}
		}
		if (gcodeStr.find("Adaptive Layers") != std::string::npos)
			if (regex_match(gcodeStr, "Adaptive Layers", sm))
			{
				std::string tStr = sm[1];
				parseInfo.adaptiveLayers = bool(atoi(tStr.c_str())); //gap
			}

		parseInfo.screenSize = "Sermoon D3";
		if (gcodeStr.find("Screen Size:CR-200B Pro") != std::string::npos)
		{
			parseInfo.screenSize = "CR - 200B Pro";
		}
		else if (gcodeStr.find("Screen Size:CR-10 Inspire") != std::string::npos)
		{
			parseInfo.screenSize = "CR-10 Inspire";
		}
		else if (gcodeStr.find("Screen Size:Ender-3 S1") != std::string::npos)
		{
			parseInfo.screenSize = "Ender-3 S1";
		}
		else if (gcodeStr.find("Screen Size:Ender-5 S1") != std::string::npos)
		{
			parseInfo.screenSize = "Ender-5 S1";
		}


		

		const std::string& preStr = result->prefixCode();
		parseInfo.spiralMode = preStr.find(";Vase Model:true");
		if (preStr.find(";machine is belt:true") != std::string::npos)
			parseInfo.beltType = 1;
		if (preStr.find("Crealitybelt") != std::string::npos)
			parseInfo.beltType = 2;
	}

}
