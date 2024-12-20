#include "crslice/gcode/slicemodelbuilder.h"
#include "crslice/gcode/sliceresult.h"
#include "trimesh2/XForm.h"
#include <math.h>
#include <regex>
#include "crslice/gcode/thumbnail.h"
#include "crslice/gcode/gcodedata.h"

#define _INT2MM(n) (n / 1000.0f)

namespace gcode
{
    float getAngelOfTwoVector(const trimesh::vec& pt1, const trimesh::vec& pt2, const trimesh::vec& c)
    {
        float theta = atan2(pt1.y - c.y, pt1.x - c.x) - atan2(pt2.y - c.y, pt2.x - c.x);
        if (theta > M_PIf)
            theta -= 2 * M_PIf;
        if (theta < -M_PIf)
            theta += 2 * M_PIf;

        theta = theta * 180.0 / M_PIf;
        if (theta < 0)
        {
            theta = 360 + theta;
        }
        return theta;
    }

#define ARC_PER_BLOCK 2
    void getDevidePoint(const trimesh::vec& p0, const trimesh::vec& p1,
        std::vector<trimesh::vec>& out, float theta, bool clockwise)
    {
        //int x = 1, y = 2;//旋转的点
        //int dx = 1, dy = 1;//被绕着旋转的点

        int count = theta > ARC_PER_BLOCK ? theta / ARC_PER_BLOCK : theta / 2;
        float angle = theta > ARC_PER_BLOCK ? ARC_PER_BLOCK*1.0f : theta / 2.0f;
        //int count = theta / ARC_PER_BLOCK;
        //int angle = ARC_PER_BLOCK;
        out.reserve(count);
        for (int i = 1; i < count + 1; i++)
        {
            //if (clockwise)
            //{
            //    angle = -15 * i;
            //}
            //else
            //{
            //    angle = 15 * i;
            //}
            ////int angle = 45 * i;//逆时针
            ////int angle = -15 * i;//顺时针
            //trimesh::vec _out = p0;
            //_out.x = (p0.x - p1.x) * cos(angle * PI / 180) - (p0.y - p1.y) * sin(angle * PI / 180) + p1.x;
            //_out.y = (p0.y - p1.y) * cos(angle * PI / 180) + (p0.x - p1.x) * sin(angle * PI / 180) + p1.y;
            trimesh::vec _out = p0;
            if (clockwise)
            {
                float _angle = angle * i;
                _out.x = (p1.x - p0.x) * cos(_angle * M_PIf / 180) + (p1.y - p0.y) * sin(_angle * M_PIf / 180) + p0.x;
                _out.y = (p1.y - p0.y) * cos(_angle * M_PIf / 180) - (p1.x - p0.x) * sin(_angle * M_PIf / 180) + p0.y;
            }
            else
            {
                float _angle = angle * i;
                _out.x = (p1.x - p0.x) * cos(_angle * M_PIf / 180) - (p1.y - p0.y) * sin(_angle * M_PIf / 180) + p0.x;
                _out.y = (p1.y - p0.y) * cos(_angle * M_PIf / 180) + (p1.x - p0.x) * sin(_angle * M_PIf / 180) + p0.y;
            }

            out.push_back(_out);
        }
    }

    std::vector<std::string> splitString(const std::string& str, const std::string& delim = ",",bool addEmpty = false )
    {
        std::vector<std::string> elems;
        size_t pos = 0;
        size_t len = str.length();
        size_t delim_len = delim.length();
        if (delim_len == 0)
            return elems;
        while (pos < len)
        {
            int find_pos = str.find(delim, pos);
            if (find_pos < 0)
            {
                std::string t = str.substr(pos, len - pos);
                if (!t.empty())
                    elems.push_back(t);
                break;
            }
            std::string t = str.substr(pos, find_pos - pos);
            if (!t.empty()) {
                elems.push_back(t);
            }
            else{
                if (addEmpty){
                    elems.push_back("");
                }
            }
            pos = find_pos + delim_len;
        }
        if (pos == len && addEmpty)
        {
            elems.push_back("");
        }
        return elems;
    }

    inline std::string str_trimmed(const std::string& src)
    {
        std::string des = src;
        int lPos = src.find_first_not_of(' ');
        int rPos = src.find_last_not_of(' ');
        if (lPos >= 0 && rPos >= 0)
        {
            des = src.substr(lPos, rPos - lPos + 1);
        }

        int lPos2 = src.find_first_not_of('\n');
        int rPos2 = src.find_last_not_of('\n');
        if (lPos2 >= 0 && rPos2 >= 0)
        {
            return des.substr(lPos2, rPos2 - lPos2 + 1);
        }
        return des;
    }

    inline SliceLineType GetLineType(const std::string& strLineType)
    {
        if (strLineType.find(";TYPE:WALL-OUTER") != std::string::npos)
        {
            return SliceLineType::OuterWall;
        }
        else if (strLineType.find(";TYPE:WALL-INNER") != std::string::npos)
        {
            return SliceLineType::InnerWall;
        }
        else if (strLineType.find(";TYPE:SKIN") != std::string::npos)
        {
            return SliceLineType::Skin;
        }
        else if (strLineType.find(";TYPE:SUPPORT-INTERFACE") != std::string::npos)
        {
            return SliceLineType::SupportInterface;
        }
        else if (strLineType.find(";TYPE:SUPPORT-INFILL") != std::string::npos)
        {
            return SliceLineType::SupportInfill;
        }
        else if (strLineType.find(";TYPE:SUPPORT") != std::string::npos)
        {
            return SliceLineType::Support;
        }
        else if (strLineType.find(";TYPE:SKIRT") != std::string::npos)
        {
            return SliceLineType::SkirtBrim;
        }
        else if (strLineType.find(";TYPE:FILL") != std::string::npos)
        {
            return SliceLineType::Infill;
        }
        else if (strLineType.find(";TYPE:PRIME-TOWER") != std::string::npos)
        {
            return SliceLineType::PrimeTower;
        }
        else if (strLineType.find(";TYPE:Slow-Flow-Types") != std::string::npos)
        {
            return SliceLineType::FlowTravel;
        }
        else if (strLineType.find(";TYPE:Flow-In-Advance-Types") != std::string::npos)
        {
            return SliceLineType::AdvanceTravel;
        }
        else
		{
			return SliceLineType::NoneType;
        }
    }

    GCodeStruct::GCodeStruct()
        : tempCurrentType(SliceLineType::NoneType)
        , tempNozzleIndex(0)
        , tempCurrentE(0.0f)
        , tempSpeed(0.0f)
        , tempAcc(0.0f)
        , layerNumberParseSuccess(true)
        , m_tracer(nullptr)
        , nIndex(0)
    {
        m_positions.push_back(tempCurrentPos);
    }

    GCodeStruct::~GCodeStruct()
    {
    }

    void GCodeStruct::processLayer(const std::string& layerCode, int layer, std::vector<int>& stepIndexMap)
    {
		std::vector<std::string> layerLines = splitString(layerCode,"\n"); //layerCode.split("\n");

        int startNumber = (int)m_moves.size();
        if (layerNumberParseSuccess && layerLines.size() > 0)
        {
			std::vector<std::string> layerNumberStr = splitString(layerLines[0],":");
            if (layerNumberStr.size() > 1)
            {
				int index = std::stoi(layerNumberStr[1].c_str());
                if (layerNumberParseSuccess)
                    tempBaseInfo.layerNumbers.push_back(index);
            }
            else
            {
                layerNumberParseSuccess = false;
            }
        }
        else
        {
            layerNumberParseSuccess = false;
        }

        //获取层高
        checkoutLayerHeight(layerLines);

        int nIndex = 0;
        for (const std::string& layerLine : layerLines)
        {
			std::string stepCode = str_trimmed(layerLine);  //" \n\r\t"
            if (stepCode.empty())
                continue;

            checkoutLayerInfo(layerLine, layer);
            processStep(stepCode, nIndex, stepIndexMap);
            ++nIndex;
        }

        tempBaseInfo.steps.push_back((int)m_moves.size() - startNumber);
    }

    void GCodeStruct::checkoutFan(const std::string& stepCode)
    {
        //std::vector <GcodeTemperature> m_temperatures;
        if (stepCode.find("M106") != std::string::npos
            || stepCode.find("M107") != std::string::npos)
        {
            GcodeFan gcodeFan = m_fans.size() > 0 ? m_fans.back() : GcodeFan();
			std::vector<std::string> strs = splitString(stepCode," ");

            float speed = 0.0f;
            int type = 0;
            int Mtypes = -1;  //1 M106 , 2 M107
            for (const auto& it3 : strs)
            {
                auto componentStr = str_trimmed(it3);
                if (componentStr.empty())
                    continue;

                if (componentStr[0] == 'S')
                {
					speed = std::atof(componentStr.substr(1).c_str());//componentStr.mid(1).toFloat();
                }

                if (Mtypes == 1)//M106
                {
                    if (!it3.compare("P1"))
                        type = 1;
                    else if (!it3.compare("P2"))
                        type = 2;
                }
                else if (Mtypes == 2)//M107
                {
                    if (!it3.compare("P1"))
                        type = 4;
                    else if (!it3.compare("P2"))
                        type = 5;
                }

                if (!it3.compare("M106"))
                {
                    Mtypes = 1;
                }
                else if (!it3.compare("M107"))
                {
                    Mtypes = 2;
                }
            }

            if (Mtypes == 1)//M106
            {
                if (type == 1)
                    gcodeFan.camberSpeed = speed;
                else if (type == 2)
                    gcodeFan.fanSpeed_1 = speed;
                else
                    gcodeFan.fanSpeed = speed;
            }
            else if (Mtypes == 2)//M107
            {
                if (type == 4)
                    gcodeFan.camberSpeed = 0;
                else if (type == 5)
                    gcodeFan.fanSpeed_1 = 0;
                else
                    gcodeFan.fanSpeed = 0;
            }

            if (Mtypes >= 0)
            {
                m_fans.push_back(gcodeFan);
            }
        }

    }

    void GCodeStruct::checkoutTemperature(const std::string& stepCode)
    {
        //std::vector <GcodeTemperature> m_temperatures;
        if (stepCode.find("M140") != std::string::npos
            || stepCode.find("M190") != std::string::npos
            || stepCode.find("M104") != std::string::npos
            || stepCode.find("M109") != std::string::npos
            || stepCode.find("M141") != std::string::npos
            || stepCode.find("M191") != std::string::npos
            || stepCode.find("EXTRUDER_TEMP") != std::string::npos
            || stepCode.find("BED_TEMP") != std::string::npos
            )
        {
            GcodeTemperature gcodeTemperature= m_temperatures.size()>0 ? m_temperatures.back(): GcodeTemperature();
			std::vector<std::string> strs = splitString(stepCode, " ");

            float temperature = 0.0f;
            int type = -1;
            for (const auto& it3 : strs)
            {
				auto componentStr = str_trimmed(it3);
                if (componentStr.empty())
                    continue;

                if (componentStr[0] == 'S')
                {
					temperature = std::atof(componentStr.substr(1).c_str());//temperature = componentStr.mid(1).toFloat();
                }
                else if (componentStr.find("EXTRUDER_TEMP") != std::string::npos)
                {
					std::vector<std::string> strs = splitString(componentStr, "=");//QStringList strs = componentStr.split("=");
                    if (strs.size() >= 2)
                    {
						temperature = std::atof(strs[1].c_str()); //strs[1].toFloat();
                    }
                    gcodeTemperature.temperature = temperature;
                    type = 4;
                }
                else if (componentStr.find("BED_TEMP") != std::string::npos)
                {
                    
					std::vector<std::string> strs = splitString(componentStr, "=");//QStringList strs = componentStr.split("=");
                    if (strs.size() >= 2)
                    {
						temperature = std::atof(strs[1].c_str()); //strs[1].toFloat();
                    }
                    gcodeTemperature.bedTemperature = temperature;
                    type = 4;
                }

                if (!it3.compare("M140")
                    || !it3.compare("M190"))
                {
                    type = 0;
                }
                else if (!it3.compare("M104")
                    || !it3.compare("M109") )
                {
                    if (!it3.compare("T1")
                        || !it3.compare("t1"))
                        type = 1;
                    else
                        type = 2;
                }
                else if (!it3.compare("M141")
                    || !it3.compare("M191"))
                {
                    type = 3;
                }
            }
            switch (type)
            {
            case 0:
                gcodeTemperature.bedTemperature = temperature;
                break;
            case 1:
                //gcodeTemperature.temperature0 = temperature;
                //break;
            case 2:
                gcodeTemperature.temperature = temperature;
                break;
            case 3:
                gcodeTemperature.camberTemperature = temperature;
                break;
            case 4:
            default:
                break;
            }

            //过滤温度为0 的情况
            if (type >= 0 && temperature > 0.0f)
            {
                if (!m_temperatures.empty())
                {
                    if (m_temperatures.back().bedTemperature != gcodeTemperature.bedTemperature
                        || m_temperatures.back().camberTemperature != gcodeTemperature.camberTemperature
                        || m_temperatures.back().temperature != gcodeTemperature.temperature)
                    {
                        tempTempIndex = m_temperatures.size();
                        m_temperatures.push_back(gcodeTemperature);
                    }
                }
                else
                {
                    tempTempIndex = m_temperatures.size();
                    m_temperatures.push_back(gcodeTemperature);
                }
            }
        }

    }

    void GCodeStruct::checkoutLayerInfo(const std::string& stepCode, int layer)
    {
        //;TIME_ELAPSED:548.869644
        if (stepCode.find("TIME_ELAPSED") != std::string::npos)
        {
            
			std::vector<std::string> strs = splitString(stepCode, ":");
            if (strs.size() >= 2)
            {
				float temp = std::atof(strs[1].c_str()) - tempCurrentTime;
                float templog = 0.0f;
                //if (temp >0)
                //{
                //    templog = std::log(temp);
                //}
                //m_layerTimeLogs.insert(std::pair<int, float>(layer, templog));
                m_layerTimes.insert(std::pair<int,float>(layer, temp));
				tempCurrentTime = std::atof(strs[1].c_str());//strs[1].toFloat();
            }
        }
    }
    
    void GCodeStruct::checkoutLayerHeight(const std::vector<std::string>& layerLines)
    {
        if (parseInfo.layerHeight > 0.0f && !parseInfo.adaptiveLayers)
        {
            GcodeLayerInfo  gcodeLayerInfo = m_gcodeLayerInfos.size() > 0 ? m_gcodeLayerInfos.back() : GcodeLayerInfo();
            gcodeLayerInfo.layerHight = parseInfo.layerHeight + 0.00001f - belowZ;
            m_gcodeLayerInfos.push_back(gcodeLayerInfo);
            return;
        }

        float height = tempCurrentZ;
        for (auto stepCode : layerLines)
        {
            if (stepCode.size() > 3)
            {
                std::vector<std::string> G01Strs = splitString(stepCode, " ");
                if (G01Strs.size() > 0)
                {
                    bool haveG123 = false;
                    bool haveXY = false;
                    bool haveE = false;
                    for (const std::string& it3 : G01Strs)
                    {
                        std::string componentStr = str_trimmed(it3);
                        if (componentStr.size()>0)
                        {
                            if (componentStr == "G1" || componentStr == "G2" || componentStr == "G3")
                            {
                                haveG123 = true;
                            }
                            else if (componentStr[0] == 'X' || componentStr[0] == 'Y')
                            {
                                haveXY = true;
                            }
                            else if (componentStr[0] == 'A' || componentStr[0] == 'P' || componentStr[0] == 'E')
                            {
                                haveE = true;
                            }
                            if (componentStr[0] == 'Z')
                            {
                                tempCurrentZ = std::atof(componentStr.substr(1).c_str());
                            }
                        }
                    }
                    if (haveG123 && haveXY && haveE)
                    {
                        height = tempCurrentZ;

                        GcodeLayerInfo  gcodeLayerInfo = m_gcodeLayerInfos.size() > 0 ? m_gcodeLayerInfos.back() : GcodeLayerInfo();
                        gcodeLayerInfo.layerHight = height + 0.00001f - belowZ;
                        m_gcodeLayerInfos.push_back(gcodeLayerInfo);
                        belowZ = height;

                        return;
                    }
                }
            }
        }

        if (m_gcodeLayerInfos.empty())
        {
            GcodeLayerInfo  gcodeLayerInfo;
            gcodeLayerInfo.layerHight = 0.1;
            gcodeLayerInfo.width = 0.4;
            m_gcodeLayerInfos.push_back(gcodeLayerInfo);
        }
    }

    void GCodeStruct::processPrefixCode(const std::string& stepCod)
    {
		std::vector<std::string> layerLines = splitString(stepCod, "\n"); //QStringList layerLines = stepCod.split("\n");
        

        for (const std::string& layerLine : layerLines)
        {
			std::string stepCode = str_trimmed(layerLine);  //" \n\r\t"
            if (stepCode.empty())
                continue;

            if (stepCode.find("M140") != std::string::npos
                || stepCode.find("M190") != std::string::npos
                || stepCode.find("M104") != std::string::npos
                || stepCode.find("M109") != std::string::npos
                || stepCode.find("M141") != std::string::npos
                || stepCode.find("M191") != std::string::npos
                || stepCode.find("EXTRUDER_TEMP") != std::string::npos
                || stepCode.find("BED_TEMP") != std::string::npos
                || stepCode.find("M106") != std::string::npos
                || stepCode.find("M107") != std::string::npos )
            {
                checkoutFan(stepCode);
                checkoutTemperature(stepCode);
            }
            else if (stepCode.find("Outer Wall Speed") != std::string::npos
                || stepCode.find("Inner Wall Speed") != std::string::npos
                || stepCode.find("Infill Speed") != std::string::npos
                || stepCode.find("Top/Bottom Speed") != std::string::npos
                || stepCode.find("Initial Layer Speed") != std::string::npos
                || stepCode.find("Skirt/Brim Speed") != std::string::npos
                || stepCode.find("Prime Tower Speed") != std::string::npos
                )
            {
                //获取速度最大限制
				std::vector<std::string> strs = splitString(stepCode, ":");//stepCode.split(":");
                if (strs.size() == 2)
                {
					std::string componentStr = str_trimmed(strs[1]);
                    if (componentStr.empty())
                        continue;
					float speed = tempSpeedMax;
					float component = std::atof(componentStr.c_str()) * 60;
                    tempSpeedMax = std::max(speed, component);
                }
            }
        }
    }

    void GCodeStruct::processStep(const std::string& stepCode, int nIndex, std::vector<int>& stepIndexMap)
    {
        checkoutTemperature(stepCode);
        checkoutFan(stepCode);

        if (stepCode.find(";TYPE:") != std::string::npos && stepCode.find(";TYPE:end") == std::string::npos)
        {
            tempCurrentType = GetLineType(stepCode);
        }
        if (stepCode.at(0) == 'T')
        {
			std::string nozzleIndexStr = stepCode.substr(1);
            if (nozzleIndexStr.size() > 0)
            {
                //bool success = false;
				int index = std::atoi(nozzleIndexStr.c_str());
                //if (success)
                    tempNozzleIndex = index;
            }

            if (tempBaseInfo.nNozzle < tempNozzleIndex + 1)
                tempBaseInfo.nNozzle = tempNozzleIndex + 1;
        }
        else if (stepCode[0] == 'G' && stepCode.size() > 1)
        {
            if (stepCode[1] == '2' || stepCode[1] == '3')
            {
                processG23(stepCode, nIndex, stepIndexMap);
            }
            else if (stepCode[1] == '0' || stepCode[1] == '1')
            {
                processG01(stepCode, nIndex, stepIndexMap);
            }
            else if (stepCode.find("G92") != std::string::npos)
            {
                tempCurrentE = 0.0f;
            }
        }
    }

    void GCodeStruct::processG01_sub(SliceLineType tempType,double tempEndE,trimesh::vec3 tempEndPos,bool havaXYZ,int nIndex, std::vector<int>& stepIndexMap, bool isG2G3, bool fromGcode, bool isOrca, bool isseam) {
        
        GcodeLayerInfo gcodeLayerInfo = m_gcodeLayerInfos.size() > 0 ? m_gcodeLayerInfos.back() : GcodeLayerInfo();

        if (tempType == SliceLineType::React)
        {
            m_retractions.push_back(m_positions.size() - 1);
        }

        if (havaXYZ)
        {
            int index = (int)m_positions.size();
            m_positions.push_back(tempEndPos);
            GCodeMove move;
            move.start = index - 1;
            //limit speed
            if (tempSpeedMax > 0.0f)
            {
                tempSpeed = tempSpeed > tempSpeedMax ? tempSpeedMax : tempSpeed;
            }
            move.speed = tempSpeed;
            move.acc = tempAcc;
            if (!fromGcode)
                move.e = tempEndE - tempCurrentE;
            else
                move.e = tempEndE;
            
            move.type = tempType;
            if (move.e == 0.0f)
                move.type = SliceLineType::Travel;
            //else if ( (move.e > 0.0f && move.type == SliceLineType::OuterWall && m_moves.back().type == SliceLineType::Travel)
            //if ((move.type == SliceLineType::Travel || move.type == SliceLineType::React) && m_moves.size() > 0 && m_moves.back().type == SliceLineType::OuterWall)
            if (isseam)
            {
                m_zSeams.push_back(move.start);
            }

            move.extruder = tempNozzleIndex;
            m_moves.emplace_back(move);

            //add temperature fan and time ...
            if (m_temperatures.empty())
            {
                tempTempIndex = m_temperatures.size();
                m_temperatures.push_back(GcodeTemperature());
            }
            if (m_fans.empty())
            {
                m_fans.push_back(GcodeFan());
            }
            if (m_gcodeLayerInfos.empty())
            {
                m_gcodeLayerInfos.push_back(GcodeLayerInfo());
            }

            //float material_density = parseInfo.material_density;
            float material_diameter = parseInfo.material_diameter;
            if (!isG2G3 && SliceLineType::Travel != tempType && SliceLineType::React != tempType)
            {
                //calculate width
                trimesh::vec3 offset = tempCurrentPos - tempEndPos;
                offset.z = 0;
                float len = trimesh::length(offset);
                float h = m_gcodeLayerInfos.back().layerHight;

                float material_s = PI * (material_diameter * 0.5) * (material_diameter * 0.5);
                float width = m_gcodeLayerInfos.back().width;

                if (!isOrca)
                {
                    if (len > 0.0001 && h > 0.0001 && move.e > 0.0f)
                    {
                        width = move.e * material_s / len / h;
                    }
                }

                //calculate flow
                float flow = 0.0f;
                if (move.e > 0.0f && len > 0)
                {
                    float r = material_diameter / 2;
                    flow = r* r* PI* move.e * move.speed / 60.0 / len;
                }

                if ((std::abs(m_gcodeLayerInfos.back().width - width) > 0.001 && width > 0)
                    || (std::abs(m_gcodeLayerInfos.back().flow - flow) > 0.001 && flow > 0))
                {
                    GcodeLayerInfo  gcodeLayerInfo = m_gcodeLayerInfos.size() > 0 ? m_gcodeLayerInfos.back() : GcodeLayerInfo();
                    gcodeLayerInfo.width = width;
                    gcodeLayerInfo.flow = flow;
                    m_gcodeLayerInfos.push_back(gcodeLayerInfo);
                }
                //end
            }
 
            m_temperatureIndex.push_back(m_temperatures.size()-1);
            m_fanIndex.push_back(m_fans.size() - 1);
            m_layerInfoIndex.push_back(m_gcodeLayerInfos.size() - 1);
            //end

            stepIndexMap.push_back(nIndex);

            tempBaseInfo.gCodeBox += tempEndPos;

            if(tempType != SliceLineType::Travel)
                processSpeed(tempSpeed);
        }

        tempCurrentPos = tempEndPos;
        if (std::abs(tempEndE - tempCurrentE) > 100000.0f){
            //TODO :ERROR E!
            tempCurrentE = tempCurrentE;
        }
        else {
            tempCurrentE = tempEndE;
        }
    }

    void GCodeStruct::processG01(const std::string& G01Str, int nIndex, std::vector<int>& stepIndexMap, bool isG2G3, bool fromGcode, bool isOrca, bool isseam)
    {
        std::vector<std::string> G01Strs = splitString(G01Str," ");

        trimesh::vec3 tempEndPos = tempCurrentPos;
        double tempEndE = tempCurrentE;
        SliceLineType tempType = tempCurrentType;
        bool havaXYZ = false;

        GcodeLayerInfo gcodeLayerInfo = m_gcodeLayerInfos.size() > 0 ? m_gcodeLayerInfos.back() : GcodeLayerInfo();
        for (const std::string& it3 : G01Strs)
        {
            std::string componentStr = str_trimmed(it3);
            //it4 ==G1 / F4800 / X110.125 / Y106.709 /Z0.6 /E575.62352
            if (componentStr.empty())
                continue;

            if (componentStr[0] == 'F')
            {
                tempSpeed = std::atof(componentStr.substr(1).c_str());
            }
            else if (componentStr[0] == 'E' || componentStr[0] == 'P' || componentStr[0] == 'V')
            {
				double e = std::atof(componentStr.substr(1).c_str());
                if (parseInfo.relativeExtrude)
                    tempEndE += e;
                else
                    tempEndE = e;
            }
            else if (componentStr[0] == 'X')
            {
				tempEndPos.at(0) = std::atof(componentStr.substr(1).c_str());
                havaXYZ = true;
            }
            else if (componentStr[0] == 'Y')
            {
				tempEndPos.at(1) = std::atof(componentStr.substr(1).c_str());
                havaXYZ = true;
            }
            else if (componentStr[0] == 'Z')
            {
                tempEndPos.at(2) = std::atof(componentStr.substr(1).c_str());;
                havaXYZ = true;
            }
        }

        if (tempEndE == tempCurrentE)
        {
            if (G01Str[1] == '0' || havaXYZ)
            {
                tempType = SliceLineType::Travel;
            }
        }
        else if (tempEndE < tempCurrentE)
        {
            if (havaXYZ)
                tempType = SliceLineType::Travel;
            else
            {
                tempType = SliceLineType::React;
                m_retractions.push_back(m_positions.size()-1);
            }
                
        }

        processG01_sub(tempType, tempEndE, tempEndPos, havaXYZ, nIndex, stepIndexMap, isG2G3, fromGcode,isOrca,isseam);
    }
    
    void GCodeStruct::processG23_sub(G2G3Info info, int nIndex, std::vector<int>& stepIndexMap, bool isG2, bool fromGcode, bool isOrca, bool isseam)
    {
        bool bcircles = false;

        trimesh::vec3 circlePos = tempCurrentPos;
        circlePos.x += info.i;
        circlePos.y += info.j;
        trimesh::vec3 circleEndPos = tempCurrentPos;
        circleEndPos.x = info.x;
        circleEndPos.y = info.y;

        float theta = 0.0f;
        std::vector<trimesh::vec> out;
        if (info.isG2)
        {
            theta = getAngelOfTwoVector(tempCurrentPos, circleEndPos, circlePos);
        }
        else
        {
            theta = getAngelOfTwoVector(circleEndPos, tempCurrentPos, circlePos);
        }
        if (info.p >0)
        {
            theta = 360;
        }

        trimesh::vec3 offset = tempCurrentPos - circlePos;
        offset.z = 0;
        float radius = trimesh::length(offset);
        //float material_density = parseInfo.material_density;
        float material_diameter = parseInfo.material_diameter;
        float material_s = PI * (material_diameter * 0.5) * (material_diameter * 0.5);
        //计算弧长
        float len = theta * M_PIf * radius / 180.0;
        float r = material_diameter / 2.0f;
        float flow = r * r * PI * info.e * tempSpeed / 60.0 / len;

        float h = m_gcodeLayerInfos.back().layerHight;
        float width = m_gcodeLayerInfos.back().width;

        if (!isOrca)
        {
            if (len > 0.0001 && h > 0.0001 && info.e > 0.0f)
            {
                width = info.e * material_s / len / h;
            }
            if ((len < 0.05) && m_gcodeLayerInfos.back().width > 0.0f)
            {
                width = m_gcodeLayerInfos.back().width;
            }
        }

        if (std::abs(m_gcodeLayerInfos.back().flow - flow) > 0.001 && len >= 1.0f)
        {
            GcodeLayerInfo  gcodeLayerInfo = m_gcodeLayerInfos.size() > 0 ? m_gcodeLayerInfos.back() : GcodeLayerInfo();
            gcodeLayerInfo.width = width;
            gcodeLayerInfo.flow = flow;
            m_gcodeLayerInfos.push_back(gcodeLayerInfo);
        }


        getDevidePoint(circlePos, tempCurrentPos, out, theta, info.isG2);
        out.push_back(circleEndPos);
        float devideE = info.e;
        if (out.size() > 0)
        {
            devideE = info.e / out.size();
        }

        std::list<std::string> G23toG1s;
        for (size_t ii = 0; ii < out.size(); ii++)
        {
            std::string devideTemp = "";
            if (info.bIsTravel)
            {
                devideTemp += "G0 ";
            }
            else
            {
                devideTemp += "G1 ";
            }
            if (info.f)
            {
                char itc[20];
                sprintf(itc, "F%0.1f ", info.f);
                devideTemp += itc;
            }
            char itcx[20];
            sprintf(itcx, "X%0.3f ", out[ii].x);
            devideTemp += itcx;
            char itcy[20];
            sprintf(itcy, "Y%0.3f ", out[ii].y);
            devideTemp += itcy;
            if (!info.bIsTravel)
            {
                char itce[20];
                double ce = parseInfo.relativeExtrude ? devideE : tempCurrentE + devideE * (ii + 1);
                sprintf(itce, "E%0.5f", ce);
                devideTemp += itce;
            }

            G23toG1s.push_back(devideTemp.c_str());
        }

        bool first = true;
        for (const auto& G23toG01 : G23toG1s)
        {
            if (first)
            {
                first = false;
                processG01(G23toG01, nIndex, stepIndexMap, true, fromGcode, isOrca, isseam);
            }
            else
                processG01(G23toG01, nIndex, stepIndexMap, true, fromGcode, isOrca, false);
        }

        tempCurrentE = info.currentE;
    }

    void GCodeStruct::processG23(const std::string& G23Code, int nIndex, std::vector<int>& stepIndexMap, bool isG2, bool fromGcode, bool isOrca, bool isseam)
    {
        std::vector<std::string> G23Strs = splitString(G23Code," ");//G1 Fxxx Xxxx Yxxx Exxx
        //G3 F1500 X118.701 Y105.96 I9.55 J1.115 E7.96039
        //bool isG2 = true;

        G2G3Info info;

        info.isG2 = G23Code[1] == '3' ? false : true;
        info.bIsTravel = true;
        info.f = 0.0f;
        info.e = 0.0f;
        info.x = 0.0f;
        info.y = 0.0f;
        info.i = 0.0f;
        info.j = 0.0f;
        bool  bcircles = false;
        info.currentE = tempCurrentE;
        for (const std::string& it3 : G23Strs)
        {
            //it4 ==G1 / F4800 / X110.125 / Y106.709 /Z0.6 /E575.62352
            std::string G23Str = str_trimmed(it3);
            if (G23Str.empty())
                continue;


			float currentValue = std::atof(G23Str.substr(1).c_str());
            if (G23Str[0] == 'E' || G23Str[0] == 'V')
            {
				float _e = currentValue;//G23Str.mid(1).toFloat();
                if (_e > 0)
                {
                    if (!parseInfo.relativeExtrude)
                        info.e = _e - tempCurrentE;
                    else
                        info.e = _e;
                    info.bIsTravel = false;
                }

                info.currentE = _e;
            }
            else if (G23Str[0] == 'F')
            {
                info.f = currentValue;
                tempSpeed = currentValue;
            }
            else if (G23Str[0] == 'X')
            {
                info.x = currentValue;
            }
            else if (G23Str[0] == 'Y')
            {
                info.y = currentValue;
            }
            else if (G23Str[0] == 'I')
            {
                info.i = currentValue;
            }
            else if (G23Str[0] == 'J')
            {
                info.j = currentValue;
            }
            else if (G23Str[0] == 'P')
            {
                bcircles = true;
                info.bIsTravel = true;
                if (info.x == 0 && info.y == 0)
                {
                    info.x = tempCurrentPos.x;
                    info.y = tempCurrentPos.y;
                }
            }
        }

        GCodeStruct::processG23_sub(info,  nIndex, stepIndexMap,isG2, fromGcode, isOrca, isseam);


    }

    void GCodeStruct::processSpeed(float speed)
    {
        if (tempBaseInfo.speedMax < speed)
            tempBaseInfo.speedMax = speed;
        if (tempBaseInfo.speedMin > speed)
            tempBaseInfo.speedMin = speed;
    }

	void GCodeStruct::buildFromResult(gcode::SliceResult* result, ccglobal::Tracer* tracer)
	{
		gcode::parseGCodeInfo(result, parseInfo);

		//get temperature and fan
		processPrefixCode(result->prefixCode());

		tempBaseInfo.nNozzle = 1;
		int layer = 0;
		int layerCount = (int)result->layerCode().size();
		for (const auto& it : result->layerCode())
		{
			std::vector<int> stepIndexMap;
			std::string layerCode = str_trimmed(it);
			processLayer(layerCode, layer, stepIndexMap);
			++layer;

			if (m_tracer)
			{
				m_tracer->progress((float)layer / (float)layerCount);
				if (m_tracer->interrupt())
				{
					m_tracer->failed("GCodeStruct::buildFromResult interrupt.");
					layerNumberParseSuccess = false;
					break;
				}
			}
		}

		tempBaseInfo.totalSteps = (int)m_moves.size();
		tempBaseInfo.layers = (int)tempBaseInfo.layerNumbers.size();
		if (!layerNumberParseSuccess)
		{
			tempBaseInfo.layerNumbers.clear();
			tempBaseInfo.layers = 1;
			tempBaseInfo.steps.clear();
			tempBaseInfo.steps.push_back(tempBaseInfo.totalSteps);
		}


		for (GCodeMove& move : m_moves)
		{
            move.speed = (move.speed - tempBaseInfo.speedMin) / (tempBaseInfo.speedMax - tempBaseInfo.speedMin + 0.01f);
		}

		{
			float minFlow = FLT_MAX, maxFlow = FLT_MIN;
			float minWidth = FLT_MAX, maxWidth = FLT_MIN;
			float minHeight = FLT_MAX, maxHeight = FLT_MIN;

			for (GcodeLayerInfo& info : m_gcodeLayerInfos)
			{
				if (info.flow > 0.0)
				{
					minFlow = fminf(info.flow, minFlow);
					maxFlow = fmaxf(info.flow, maxFlow);
				}

				if (info.width > 0.0)
				{
					minWidth = fminf(info.width, minWidth);
					maxWidth = fmaxf(info.width, maxWidth);
				}

				minHeight = fminf(info.layerHight, minHeight);
				maxHeight = fmaxf(info.layerHight, maxHeight);
			}
			tempBaseInfo.minFlowOfStep = minFlow;
			tempBaseInfo.maxFlowOfStep = maxFlow;
			tempBaseInfo.minLineWidth = minWidth;
			tempBaseInfo.maxLineWidth = maxWidth;
			tempBaseInfo.minLayerHeight = minHeight;
			tempBaseInfo.maxLayerHeight = maxHeight;

		}

		{
			float minTime = FLT_MAX, maxTime = FLT_MIN;
			for (auto t : m_layerTimes)
			{
				float time = t.second;
				minTime = fminf(time, minTime);
				maxTime = fmaxf(time, maxTime);
			}
			tempBaseInfo.minTimeOfLayer = minTime;
			tempBaseInfo.maxTimeOfLayer = maxTime;
		}


		{
			float minTemp = FLT_MAX, maxTemp = FLT_MIN;
			for (GcodeTemperature& t : m_temperatures)
			{
				minTemp = fminf(t.temperature, minTemp);
				maxTemp = fmaxf(t.temperature, maxTemp);
			}
			tempBaseInfo.minTemperature = minTemp;
			tempBaseInfo.maxTemperature = maxTemp;
		}
	}

	void GCodeStruct::buildFromResult(SliceResultPointer result, const GCodeParseInfo& info, GCodeStructBaseInfo& baseInfo, std::vector<std::vector<int>>& stepIndexMaps, ccglobal::Tracer* tracer /*= nullptr*/)
	{
		m_tracer = tracer;

		//get temperature and fan
		processPrefixCode(result->prefixCode());

		parseInfo = info;
		tempBaseInfo.nNozzle = 1;
		int layer = 0;
		int layerCount = (int)result->layerCode().size();
		for (const auto& it : result->layerCode())
		{
			std::vector<int> stepIndexMap;
			std::string layerCode = str_trimmed(it);
			processLayer(layerCode, layer, stepIndexMap);
			stepIndexMaps.push_back(stepIndexMap);
			++layer;

			if (m_tracer)
			{
				m_tracer->progress((float)layer / (float)layerCount);
				if (m_tracer->interrupt())
				{
					m_tracer->failed("GCodeStruct::buildFromResult interrupt.");
					layerNumberParseSuccess = false;
					break;
				}

                m_tracer->variadicFormatMessage(0, layer);
			}
		}

		tempBaseInfo.totalSteps = (int)m_moves.size();
		tempBaseInfo.layers = (int)tempBaseInfo.layerNumbers.size();
		if (!layerNumberParseSuccess)
		{
			tempBaseInfo.layerNumbers.clear();
			tempBaseInfo.layers = 1;
			tempBaseInfo.steps.clear();
			tempBaseInfo.steps.push_back(tempBaseInfo.totalSteps);
			if (stepIndexMaps.size() > 0)
			{
				stepIndexMaps.resize(1);
			}
		}


		//for (GCodeMove& move : m_moves)
		//{
		//	move.speed = (move.speed - tempBaseInfo.speedMin )/ (tempBaseInfo.speedMax - tempBaseInfo.speedMin + 0.01f);
		//}

		{
			float minFlow = FLT_MAX, maxFlow = FLT_MIN;
			float minWidth = FLT_MAX, maxWidth = FLT_MIN;

			for (GcodeLayerInfo& info : m_gcodeLayerInfos)
			{
				if (info.flow > 0.0)
				{
					minFlow = fminf(info.flow, minFlow);
					maxFlow = fmaxf(info.flow, maxFlow);
				}

                if (info.width > 0.0)
                {
                    minWidth = fminf(info.width, minWidth);
                    maxWidth = fmaxf(info.width, maxWidth);
                }

			}
			tempBaseInfo.minFlowOfStep = minFlow;
			tempBaseInfo.maxFlowOfStep = maxFlow;
			tempBaseInfo.minLineWidth = minWidth;
			tempBaseInfo.maxLineWidth = maxWidth;
		}

        {
            float minHeight = FLT_MAX, maxHeight = FLT_MIN;
            float minAcc = FLT_MAX, maxAcc = FLT_MIN;
            if (m_layerInfoIndex.size() == m_moves.size())
            {
                for (int i = 0; i < m_moves.size(); i++)
                {
                    const GCodeMove& move = m_moves.at(i);
                    SliceLineType type = move.type;
                    if (type == SliceLineType::Travel || type == SliceLineType::MoveCombing || type == SliceLineType::React || type == SliceLineType::erWipeTower || type == SliceLineType::Wipe || type == SliceLineType::Unretract)
                    {
                        continue;
                    }

                    int idx = m_layerInfoIndex.at(i);
                    if (idx < m_gcodeLayerInfos.size())
                    {
                        const GcodeLayerInfo& info = m_gcodeLayerInfos.at(idx);
                        minHeight = fminf(info.layerHight, minHeight);
                        maxHeight = fmaxf(info.layerHight, maxHeight);
                    }

                    // for acc
                    minAcc = fminf(move.acc, minAcc);
                    maxAcc = fmaxf(move.acc, maxAcc);
                }

                tempBaseInfo.minLayerHeight = minHeight;
                tempBaseInfo.maxLayerHeight = maxHeight;
                tempBaseInfo.minAcc = minAcc;
                tempBaseInfo.maxAcc = maxAcc;
            }
            else {
                printf("m_layerInfoIndex size not equal to m_moves\n");
            }
        }

		{
			float minTime = FLT_MAX, maxTime = FLT_MIN;
			for (auto t : m_layerTimes)
			{
				float time = t.second;
				minTime = fminf(time, minTime);
				maxTime = fmaxf(time, maxTime);
			}
			tempBaseInfo.minTimeOfLayer = minTime;
			tempBaseInfo.maxTimeOfLayer = maxTime;
		}


		{
			float minTemp = FLT_MAX, maxTemp = FLT_MIN;
			for (GcodeTemperature& t : m_temperatures)
			{
                if (t.temperature > 0)
                {
					minTemp = fminf(t.temperature, minTemp);
					maxTemp = fmaxf(t.temperature, maxTemp);
				}
			}
			tempBaseInfo.minTemperature = minTemp;
			tempBaseInfo.maxTemperature = maxTemp;
		}
		baseInfo = tempBaseInfo;
	}

    void GCodeStruct::buildFromResult(GCodeParseInfo& info, GCodeStructBaseInfo& baseInfo, std::vector<std::vector<int>>& stepIndexMaps, ccglobal::Tracer* tracer)
    {
        info = parseInfo;
        tempBaseInfo.totalSteps = (int)m_moves.size();
        tempBaseInfo.layers = (int)tempBaseInfo.layerNumbers.size();
        //if (!layerNumberParseSuccess)
        //{
        //    tempBaseInfo.layerNumbers.clear();
        //    tempBaseInfo.layers = 1;
        //    tempBaseInfo.steps.clear();
        //    tempBaseInfo.steps.push_back(tempBaseInfo.totalSteps);
        //    if (stepIndexMaps.size() > 0)
        //    {
        //        stepIndexMaps.resize(1);
        //    }
        //}

        if ((int)m_moves.size() - startNumber)
            tempBaseInfo.steps.push_back((int)m_moves.size() - startNumber);

        stepIndexMaps = m_stepIndexMaps;
        //m_stepIndexMaps.clear();

        /*for (GCodeMove& move : m_moves)
        {
            move.speed = (move.speed - tempBaseInfo.speedMin) / (tempBaseInfo.speedMax - tempBaseInfo.speedMin + 0.01f);
        }*/

        {
            float minFlow = FLT_MAX, maxFlow = FLT_MIN;
            float minWidth = FLT_MAX, maxWidth = FLT_MIN;

            for (GcodeLayerInfo& info : m_gcodeLayerInfos)
            {
                if (info.flow > 0.0)
                {
                    minFlow = fminf(info.flow, minFlow);
                    maxFlow = fmaxf(info.flow, maxFlow);
                }

                if (info.width > 0.0)
                {
                    minWidth = fminf(info.width, minWidth);
                    maxWidth = fmaxf(info.width, maxWidth);
                }

            }
            tempBaseInfo.minFlowOfStep = minFlow;
            tempBaseInfo.maxFlowOfStep = maxFlow;
            tempBaseInfo.minLineWidth = minWidth;
            tempBaseInfo.maxLineWidth = maxWidth;

        }

        {
            float minHeight = FLT_MAX, maxHeight = FLT_MIN;
            float minAcc = FLT_MAX, maxAcc = FLT_MIN;
            if (m_layerInfoIndex.size() == m_moves.size())
            {
                for (int i = 0; i < m_moves.size(); i++)
                {
                    const GCodeMove& move = m_moves.at(i);
                    SliceLineType type = move.type;
                    if (type == SliceLineType::Travel || type == SliceLineType::MoveCombing || type == SliceLineType::React || type == SliceLineType::erWipeTower || type == SliceLineType::Wipe || type == SliceLineType::Unretract)
                    {
                        continue;
                    }

                    int idx = m_layerInfoIndex.at(i);
                    if (idx < m_gcodeLayerInfos.size())
                    {
                        const GcodeLayerInfo& info = m_gcodeLayerInfos.at(idx);
                        minHeight = fminf(info.layerHight, minHeight);
                        maxHeight = fmaxf(info.layerHight, maxHeight);
                    }

                    // for acc
                    minAcc = fminf(move.acc, minAcc);
                    maxAcc = fmaxf(move.acc, maxAcc);
                }

                tempBaseInfo.minLayerHeight = minHeight;
                tempBaseInfo.maxLayerHeight = maxHeight;
                tempBaseInfo.minAcc = minAcc;
                tempBaseInfo.maxAcc = maxAcc;
            }
            else {
                printf("m_layerInfoIndex size not equal to m_moves\n");
            }
        }

        {
            float minTime = FLT_MAX, maxTime = FLT_MIN;
            for (auto t : m_layerTimes)
            {
                float time = t.second;
                minTime = fminf(time, minTime);
                maxTime = fmaxf(time, maxTime);
            }
            tempBaseInfo.minTimeOfLayer = minTime;
            tempBaseInfo.maxTimeOfLayer = maxTime;
        }


        {
            float minTemp = FLT_MAX, maxTemp = FLT_MIN;
            for (GcodeTemperature& t : m_temperatures)
            {
                if (t.temperature > 0)
                {
                    minTemp = fminf(t.temperature, minTemp);
                    maxTemp = fmaxf(t.temperature, maxTemp);
                }
            }
            tempBaseInfo.minTemperature = minTemp;
            tempBaseInfo.maxTemperature = maxTemp;
        }
        baseInfo = tempBaseInfo;
    }

    void GCodeStruct::getPathData(const trimesh::vec3 point, float e, int type, bool fromGcode, bool isOrca, bool isseam)
    {
        if (layerNumberParseSuccess)
        {
            layerNumberParseSuccess = false;
        }

        trimesh::vec3 tempEndPos = tempCurrentPos;
        double tempEndE = tempCurrentE;
        SliceLineType tempType = tempCurrentType;
        //bool havaXYZ = false;

        tempType = (SliceLineType)type;
        GcodeLayerInfo gcodeLayerInfo = m_gcodeLayerInfos.size() > 0 ? m_gcodeLayerInfos.back() : GcodeLayerInfo();

        if (!fromGcode)
        {
            if (e > -999)
            {
                if (parseInfo.relativeExtrude)
                    tempEndE += e;
                else
                    tempEndE = e;
            }

            if (point.z > -999)
            {
                tempEndPos = point;
                tempEndPos = { _INT2MM(tempEndPos.x),_INT2MM(tempEndPos.y) ,_INT2MM(tempEndPos.z) };
            }
            else
            {
                tempEndPos.x = point.x;
                tempEndPos.y = point.y;
                tempEndPos = { _INT2MM(tempEndPos.x),_INT2MM(tempEndPos.y) ,tempEndPos.z };
            }
        }
        else
        {
            tempEndE = e;
            tempEndPos = point;
            //tempEndPos.y = point.y;
            //tempEndPos = { tempEndPos.x ,tempEndPos.y  ,tempEndPos.z };
        }


        //TODO
        //std::vector<int> stepIndexMap;
        if(m_stepIndexMaps.empty())
            m_stepIndexMaps.push_back(std::vector<int>());
        processG01_sub(tempType, tempEndE, tempEndPos, true , nIndex++, m_stepIndexMaps.back(), false, fromGcode, isOrca,isseam);
    }

    void GCodeStruct::getPathDataG2G3(const trimesh::vec3 point, float i, float j, float e, int type,int p, bool isG2, bool fromGcode, bool isOrca, bool isseam) {
        
        trimesh::vec3 tempEndPos = tempCurrentPos;

        if (!fromGcode)
        {
            if (point.z > -999)
            {
                tempEndPos = point;
                tempEndPos = { _INT2MM(tempEndPos.x),_INT2MM(tempEndPos.y) ,_INT2MM(tempEndPos.z) };
                i = _INT2MM(i);
                j = _INT2MM(j);
            }
            else
            {
                tempEndPos.x = point.x;
                tempEndPos.y = point.y;
                tempEndPos = { _INT2MM(tempEndPos.x),_INT2MM(tempEndPos.y) ,tempEndPos.z };
                i = _INT2MM(i);
                j = _INT2MM(j);
            }
        }
        else
        {
            tempEndPos = point;
            tempCurrentPos.z = point.z;
        }



        tempCurrentType = (SliceLineType)type;

        G2G3Info info;
        //info.bIsTravel = true;
        info.isG2 = isG2;
        info.f = tempSpeed;
        info.e = e;
        info.x = tempEndPos.x;
        info.y = tempEndPos.y;
        info.i = i;
        info.j = j;
        info.p = p;
        info.bIsTravel = (tempCurrentType == SliceLineType::Travel || tempCurrentType == SliceLineType::React);
        info.currentE = tempCurrentE;

        if (!fromGcode)
        {
            if (e > -999)
            {
                if (!parseInfo.relativeExtrude)
                    info.e = e - tempCurrentE;
                else
                    info.e = e;

                info.currentE = e;
            }
        }
        else
        {
            info.e = e;
            info.currentE = e;
        }


        //TODO
        if (m_stepIndexMaps.empty())
            m_stepIndexMaps.push_back(std::vector<int>());
        GCodeStruct::processG23_sub(info, nIndex++, m_stepIndexMaps.back(), isG2,  fromGcode,  isOrca,  isseam);
    }

    void GCodeStruct::getNotPath()
    {
        nIndex++;
    }

    void GCodeStruct::setNozzleColorList(std::string& colorList, std::string& defaultColorList, std::string& defaultType)
    {
        //TODO std::vector<std::string> nozzleColorList;
        //#006cff, #ffaaff, #ff0e00, #aaaa7f, #d87206, #43b7b0
        if (!colorList.empty())
        {
            if (colorList.find(",") != std::string::npos)
                m_nozzleColorList = splitString(colorList, ",");
            else if (colorList.find(";") != std::string::npos)
                m_nozzleColorList = splitString(colorList, ";");
            for (auto& color : m_nozzleColorList)
                color = str_trimmed(color);

            if (defaultColorList.find(",") != std::string::npos)
                m_nozzleColorListDefault = splitString(defaultColorList, ",",true);
            else if (defaultColorList.find(";") != std::string::npos)
                m_nozzleColorListDefault = splitString(defaultColorList, ";", true);
            for (auto& color : m_nozzleColorListDefault)
                color = str_trimmed(color);
            
            if (defaultType.find(",") != std::string::npos)
                m_nozzleTypeDefault = splitString(defaultType, ",", true);
            else if (defaultType.find(";") != std::string::npos)
                m_nozzleTypeDefault = splitString(defaultType, ";", true);
            for (auto& color : m_nozzleTypeDefault)
                color = str_trimmed(color);
        }
    }

    void GCodeStruct::setParam(gcode::GCodeParseInfo& pathParam){
        parseInfo = pathParam;
    }

    gcode::GCodeParseInfo& GCodeStruct::getParam()
    {
        return parseInfo;
    }

    void GCodeStruct::setLayer(int layer){
        tempBaseInfo.layerNumbers.push_back(layer);

        //if (m_stepIndexMaps.empty())
        {
            nIndex = 0;
            m_stepIndexMaps.push_back(std::vector<int>());
        }

        if (m_moves.size() > 0)
        {
            tempBaseInfo.steps.push_back((int)m_moves.size() - startNumber);
        }
        startNumber = m_moves.size();
    }
    void GCodeStruct::setSpeed(float s){
        tempSpeed = s;
    }
    void GCodeStruct::setAcc(float acc)
    {
        tempAcc = acc;
    }
    void GCodeStruct::setTEMP(float temp){
        GcodeTemperature gcodeTemperature = m_temperatures.size() > 0 ? m_temperatures.back() : GcodeTemperature();
        gcodeTemperature.temperature = temp;
        m_temperatures.push_back(gcodeTemperature);
    }
    void GCodeStruct::setExtruder(int nr){
        tempNozzleIndex = nr;
    }
    void GCodeStruct::setFan(float fan){
        GcodeFan gcodeFan = m_fans.size() > 0 ? m_fans.back() : GcodeFan();
        gcodeFan.fanSpeed = fan;
        m_fans.push_back(gcodeFan);
    }

    void GCodeStruct::setZ(float z, float h){
        if (h >= 0)
        {
            if (belowZ < h)
            {
                GcodeLayerInfo  gcodeLayerInfo = m_gcodeLayerInfos.size() > 0 ? m_gcodeLayerInfos.back() : GcodeLayerInfo();

                float layerHight = h - belowZ;
                gcodeLayerInfo.layerHight = layerHight;
                m_gcodeLayerInfos.push_back(gcodeLayerInfo);

                belowZ = h;
            }
        }
        tempCurrentZ = z;
    }

    void GCodeStruct::setZ_from_gcode(float z) {
        if (z >= 0)
        {
            GcodeLayerInfo  gcodeLayerInfo = m_gcodeLayerInfos.size() > 0 ? m_gcodeLayerInfos.back() : GcodeLayerInfo();
            float layerHight = z;
            gcodeLayerInfo.layerHight = layerHight;
            m_gcodeLayerInfos.push_back(gcodeLayerInfo);

            belowZ += z;
        }
        tempCurrentZ = z;
    }

    void GCodeStruct::setE(float e){
        tempCurrentE = e;
    }

    void GCodeStruct::setWidth(float width)
    {
        GcodeLayerInfo  gcodeLayerInfo = m_gcodeLayerInfos.size() > 0 ? m_gcodeLayerInfos.back() : GcodeLayerInfo();
        gcodeLayerInfo.width = width;
        m_gcodeLayerInfos.push_back(gcodeLayerInfo);
    }

    void GCodeStruct::setLayerHeight(float height)
    {
        m_layerHeights.push_back(height);
    }

    void GCodeStruct::setLayerPause(int pause)
    {
        m_pause.push_back(pause);
    }
    void GCodeStruct::setTime(float time)
    {
        if (time <= 0)
        {
            m_layerTimes.insert(std::pair<int, float>(tempBaseInfo.layerNumbers.back() - 1, 0));
            return;

        }
        float temp = time - tempCurrentTime;
        float templog = 0.0f;
        
        if (!tempBaseInfo.layerNumbers.empty())
        {
            m_layerTimes.insert(std::pair<int, float>(tempBaseInfo.layerNumbers.back()-1, temp));
        }
        tempCurrentTime = time;//strs[1].toFloat();

        parseInfo.printTime = tempCurrentTime;
    }
}

