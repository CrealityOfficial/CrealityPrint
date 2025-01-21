#include "SmoothSpeedAndAcc.hpp"
#include <algorithm>

namespace Slic3r
{
#define _MM2INT(n) ((n) * 1000 + 0.5 * (((n) > 0) - ((n) < 0)))

    SmoothSpeedAcc::SmoothSpeedAcc()
        :max_speed_limit_to_height(-1)
        ,acceleration_limit_mess_enable(false)
        ,acceleration_limit_height_enable(false)
        ,material_diameter(1.75)
        ,material_density(1.24)
    {

    }
    SmoothSpeedAcc::~SmoothSpeedAcc()
    {
        
    }


    void SmoothSpeedAcc::paraseLimitStr(std::string str, std::vector<LimitData>& outData, const double& init_limit_speed, const double& init_limit_acc, const double& init_limit_temp)
    {
        //[[0.5,1.0,100,6000,220],[1.0,1.5,80,5500,210],[1.5,2.0,60,5000,200]]
        int len = str.length();
        if (len <= 3)
            return;

        int no = str.find_first_of('[');
        if (no < 0 || no >= len)
            return;

        str = str.substr(no + 1, str.length());
        no = str.find_last_of(']');
        if (no < 0 || no >= len)
            return;

        str = str.substr(0, no);

        int findIndex1 = str.find(']');
        while (findIndex1 >= 0 && findIndex1 < str.size())
        {
            LimitData limitData;
            limitData.speed1 = outData.empty() ? init_limit_speed: outData.back().speed2;
            limitData.Acc1 = outData.empty() ? init_limit_acc: outData.back().Acc2;
            limitData.Temp1 = outData.empty() ? init_limit_temp : outData.back().Temp2;

            std::string temp = str.substr(0, findIndex1);
            str = str.substr(findIndex1 + 1, str.length());

            int findIndex = temp.find_last_of('[');
            if (findIndex < 0 || findIndex >= temp.length())
                continue;
            temp = temp.substr(findIndex + 1, temp.length());

            findIndex = temp.find(',');
            std::vector<double> data;
            std::string str1;
            while (findIndex >= 0 && findIndex < temp.length())
            {
                str1 = temp.substr(0, findIndex);
                temp = temp.substr(findIndex + 1, temp.length());
                data.push_back(std::atof(str1.c_str()));
                findIndex = temp.find(',');

                if (findIndex < 0 || findIndex >= temp.length())
                {
                    data.push_back(std::atof(temp.c_str()));
                }
            }

            if (data.size() == 4)
            {
                limitData.value1 = _MM2INT(data[0]);
                limitData.value2 = _MM2INT(data[1]);
                limitData.speed2 = data[2];
                limitData.Acc2 = data[3];
                //limitData.data.temp;
            }
            else if (data.size() == 5)
            {
                limitData.value1 = _MM2INT(data[0]);
                limitData.value2 = _MM2INT(data[1]);
                limitData.speed2 = data[2];
                limitData.Acc2 = data[3];
                limitData.Temp2 = data[4];
            }
            outData.push_back(limitData);

            findIndex1 = str.find(']');
        }
    }

    bool SmoothSpeedAcc::get_acceleration_limit_mess_enable()
    {
        return  acceleration_limit_mess_enable;
    }

    bool SmoothSpeedAcc::get_acceleration_limit_height_enable()
    {
        return  acceleration_limit_height_enable;
    }

    void SmoothSpeedAcc::detect_speed(double& speed, const double value_e, const double current_layer_z)
    {
        if (acceleration_limit_mess_enable || acceleration_limit_height_enable)
        {
            if (acceleration_limit_mess_enable)
                detect_limit_speed(LimitType::LIMIT_MESS, value_e, current_layer_z);
            if (acceleration_limit_height_enable)
                detect_limit_speed(LimitType::LIMIT_HEIGHT, value_e, current_layer_z);

            if (current_limit_speed > 0.0f)
                speed = std::min(speed, current_limit_speed);
        }
    }

    void SmoothSpeedAcc::detect_speed_min(double& speed, const double value_e, const double current_layer_z)
    {
        if (acceleration_limit_mess_enable || acceleration_limit_height_enable)
        {
            if (acceleration_limit_mess_enable)
                detect_limit_speed(LimitType::LIMIT_MESS, value_e, current_layer_z);
            if (acceleration_limit_height_enable)
                detect_limit_speed(LimitType::LIMIT_HEIGHT, value_e, current_layer_z);

            if (current_limit_speed > 0.0f)
                speed = std::min(speed, current_limit_speed * 60.0f);
        }
    }

    void SmoothSpeedAcc::detect_acc(double& acc, const double value_e, const double current_layer_z)
    {
        if (acceleration_limit_mess_enable || acceleration_limit_height_enable)
        {
            if (acceleration_limit_mess_enable)
                detect_limit_acc(LimitType::LIMIT_MESS, value_e, current_layer_z);
            if (acceleration_limit_height_enable)
                detect_limit_acc(LimitType::LIMIT_HEIGHT, value_e, current_layer_z);

            if (current_limit_Acc > 0.0f)
                acc = std::min(acc, current_limit_Acc);
        }
    }

    void SmoothSpeedAcc::init_limit(const DynamicPrintConfig& setting)
    {
        //if (setting)
        { 
            //material_diameter = setting.option<ConfigOptionFloats>("filament_diameter")->get_at(0);
            //material_diameter = setting.option<ConfigOptionFloats>("filament_density")->get_at(0);

            //todo
            //init_limit_speed
            double init_limit_speed = 0.0f;
            init_limit_speed = std::max(init_limit_speed,setting.option<ConfigOptionFloat>("outer_wall_speed")->getFloat());
            init_limit_speed = std::max(init_limit_speed, setting.option<ConfigOptionFloat>("inner_wall_speed")->getFloat());
            //init_limit_speed = std::max(init_limit_speed, setting.option<ConfigOptionFloat>("sparse_infill_speed")->getFloat());
            //init_limit_speed = std::max(init_limit_speed, setting.option<ConfigOptionFloat>("internal_solid_infill_speed")->getFloat());
            //init_limit_speed = std::max(init_limit_speed, setting.option<ConfigOptionFloat>("top_surface_speed")->getFloat());
            //init_limit_speed = std::max(init_limit_speed, setting.option<ConfigOptionFloat>("bridge_speed")->getFloat());

            //init_limit_acc
            double init_limit_acc = 0.0f;
            //init_limit_acc = std::max(init_limit_acc, setting.option<ConfigOptionFloatOrPercent>("bridge_acceleration")->getFloat());
            init_limit_acc = std::max(init_limit_acc, setting.option<ConfigOptionFloat>("default_acceleration")->getFloat());
            init_limit_acc = std::max(init_limit_acc, setting.option<ConfigOptionFloat>("inner_wall_acceleration")->getFloat());
            //init_limit_acc = std::max(init_limit_acc, setting.option<ConfigOptionFloatOrPercent>("internal_solid_infill_acceleration")->getFloat());
            init_limit_acc = std::max(init_limit_acc, setting.option<ConfigOptionFloat>("outer_wall_acceleration")->getFloat());
            //init_limit_acc = std::max(init_limit_acc, setting.option<ConfigOptionFloatOrPercent>("sparse_infill_acceleration")->getFloat());
            //init_limit_acc = std::max(init_limit_acc, setting.option<ConfigOptionFloat>("top_surface_acceleration")->getFloat());
            //init_limit_acc = std::max(init_limit_acc, setting.travel_acceleration.value);

            
            //init_limit_temp
            //Temperature init_limit_temp = setting->get<Temperature>("material_print_temperature");

            double init_limit_temp = 0.0f;
            if (setting.option<ConfigOptionBool>("acceleration_limit_mess_enable")->value)
            {
                std::string str = setting.option<ConfigOptionString>("acceleration_limit_mess")->serialize();
                paraseLimitStr(str, vctacceleration_limit_mass, init_limit_speed, init_limit_acc, init_limit_temp);
                acceleration_limit_mess_enable = true;;
            }

            if (setting.option<ConfigOptionBool>("speed_limit_to_height_enable")->value)
            {
                std::string str = setting.option<ConfigOptionString>("speed_limit_to_height")->serialize();
                paraseLimitStr(str, vctacceleration_limit_height, init_limit_speed, init_limit_acc, init_limit_temp);
                acceleration_limit_height_enable = true;;
            }
        }
    }

    bool SmoothSpeedAcc::detect_limit_acc(const LimitType& limitType, const double value_e, const double current_layer_z)
    {
        double value = 0.0f;

        if (limitType == LimitType::LIMIT_MESS)
        {
            //double volume = value_e * (0.25 * material_diameter * material_diameter * 3.1415926);
            //value = volume * (material_density / 1000.0);
            value = value_e ;
        }
        else if (limitType == LimitType::LIMIT_HEIGHT)
        {
            value = current_layer_z*1000;
        }

        if (value < 0)
            return false;

        //检测在第几个区间
        std::vector<LimitData> limit_data;
        if (limitType == LimitType::LIMIT_MESS) {
            limit_data = vctacceleration_limit_mass;
        }
        else if (limitType == LimitType::LIMIT_HEIGHT) {
            limit_data = vctacceleration_limit_height;
        }

        for (size_t i = 0; i < limit_data.size(); i++)
        {
            LimitData& limit = limit_data[i];
            double& value1 = limit.value1;
            double& value2 = limit.value2;

            if (value <= value2 && value >= value1)
            {
                //Acc
                {
                    double& s1 = limit.Acc1;
                    double& s2 = limit.Acc2;
                    if ((value2 - value1) && (s1 - s2))
                    {
                        //y = ax + b
                        double a = (s2 - s1) / (value2 - value1);
                        double b = s1 - a * value1;
                        double out = a * value + b;
                        if (out < current_limit_Acc || current_limit_Acc < 0.0f)
                        {
                            current_limit_Acc = out;
                        }
                    }
                }
            }
        }

        return true;
    }

    bool SmoothSpeedAcc::detect_limit_temp(const LimitType& limitType, const double value_e, const double current_layer_z)
    {
        double value = 0.0f;

        if (limitType == LimitType::LIMIT_MESS)
        {
            //double volume = value_e * (0.25 * material_diameter * material_diameter * 3.1415926);
            //value = volume * (material_density / 1000.0);
            value = value_e ;
        }
        else if (limitType == LimitType::LIMIT_HEIGHT)
        {
            value = current_layer_z *1000;
        }

        if (value < 0)
            return false;

        //检测在第几个区间
        std::vector<LimitData> limit_data;
        if (limitType == LimitType::LIMIT_MESS) {
            limit_data = vctacceleration_limit_mass;
        }
        else if (limitType == LimitType::LIMIT_HEIGHT) {
            limit_data = vctacceleration_limit_height;
        }

        for (size_t i = 0; i < limit_data.size(); i++)
        {
            LimitData& limit = limit_data[i];
            double& value1 = limit.value1;
            double& value2 = limit.value2;

            if (value <= value2 && value >= value1)
            {
                //Temp
                {
                    double& s1 = limit.Temp1;
                    double& s2 = limit.Temp2;
                    if ((value2 - value1) && (s1 - s2))
                    {
                        //y = ax + b
                        double a = (s2 - s1) / (value2 - value1);
                        double b = s1 - a * value1;
                        double out = a * value + b;
                        if (out < current_limit_Temp || current_limit_Temp < 0.0f)
                        {
                            current_limit_Temp = out;
                        }
                    }
                }
                //return true;
            }
        }

        return true;
    }

    bool SmoothSpeedAcc::detect_limit_speed(const LimitType& limitType, const double value_e, const double current_layer_z)
    {
        double value = 0.0f;

        if (limitType == LimitType::LIMIT_MESS)
        {
            //double volume = value_e * (0.25 * material_diameter * material_diameter * 3.1415926);
            //value = volume * (material_density / 1000.0);
            value = value_e  ;
        }
        else if (limitType == LimitType::LIMIT_HEIGHT)
        {
            value = current_layer_z*1000;
        }

        if (value < 0)
            return false;

        //检测在第几个区间
        std::vector<LimitData> limit_data;
        if (limitType == LimitType::LIMIT_MESS) {
            limit_data = vctacceleration_limit_mass;
        }
        else if (limitType == LimitType::LIMIT_HEIGHT) {
            limit_data = vctacceleration_limit_height;
        }

        for (size_t i = 0; i < limit_data.size(); i++)
        {
            LimitData& limit = limit_data[i];
            double& value1 = limit.value1;
            double& value2 = limit.value2;

            if (value <= value2 && value >= value1)
            {
                //speed
                {
                    double& s1 = limit.speed1;
                    double& s2 = limit.speed2;
                    if ((value2 - value1) && (s1 - s2))
                    {
                        //y = ax + b
                        double a = (s2 - s1) / (value2 - value1);
                        double b = s1 - a * value1;
                        double out = a * value + b;
                        if (out < current_limit_speed || current_limit_speed < 0.0f)
                        {
                            current_limit_speed = out;
                        }
                    }
                }
            }
        }

        return true;
    }

    void SmoothSpeedAcc::calculatMaxSpeedLimitToHeight(const FlowTempGraph& speed_limit_to_height,const float current_layer_z)
    {
        std::vector <std::pair<float, float>> limit_data;
        for (int i = 0; i < speed_limit_to_height.data.size(); i++)
        {
            limit_data.push_back(std::pair(speed_limit_to_height.data[i].flow, speed_limit_to_height.data[i].temp));
        }

        std::sort(limit_data.begin(), limit_data.end(),
            [](const std::pair<float, float>& a, const std::pair<float, float>& b) {
                if (a.first == b.first) {
                    return a.second > b.second;
                }
                return a.first < b.first; });

        auto iter = limit_data.begin();
        while (iter != limit_data.end())
        {
            if (current_layer_z >= _MM2INT(iter->first))
            {
                max_speed_limit_to_height = iter->second;
            }
            iter++;
        }
    }

    void FlowTempGraph::init_limit(const std::string& limit)
    {
        std::string str = limit;

        //[[3.5,200],[7.0,240]]
        int len = str.length();
        if (len <= 3)
            return;

        int no = str.find_first_of('[');
        if (no < 0 || no >= len)
            return;

        str = str.substr(no + 1, str.length());
        no = str.find_last_of(']');
        if (no < 0 || no >= len)
            return;

        str = str.substr(0, no);
        int findIndex1 = str.find(']');
        while (findIndex1 >= 0 && findIndex1 < str.size())
        {
            //std::vector<Datum> data
            Datum _data(0,0);

            std::string temp = str.substr(0, findIndex1);
            str = str.substr(findIndex1 + 1, str.length());

            int findIndex = temp.find_last_of('[');
            if (findIndex < 0 || findIndex >= temp.length())
                continue;
            temp = temp.substr(findIndex + 1, temp.length());

            findIndex = temp.find(',');
            std::vector<double> initdata;
            std::string str1;
            while (findIndex >= 0 && findIndex < temp.length())
            {
                str1 = temp.substr(0, findIndex);
                temp = temp.substr(findIndex + 1, temp.length());
                initdata.push_back(std::atof(str1.c_str()));
                findIndex = temp.find(',');

                if (findIndex < 0 || findIndex >= temp.length())
                {
                    initdata.push_back(std::atof(temp.c_str()));
                }
            }

            if (initdata.size() == 2)
            {
                _data.flow = initdata[0];
                _data.temp = initdata[1];
                //limitData.data.temp;
                data.push_back(_data);
            }
            findIndex1 = str.find(']');
        }

    }

    double FlowTempGraph::getTemp(const double flow, const double material_print_temperature, bool flow_dependent_temperature) const
    {
        if (!flow_dependent_temperature || data.empty() )
        {
            return material_print_temperature;
        }
        if (data.size() == 1)
        {
            return data.front().temp;
        }
        if (flow < data.front().flow)
        {
            return data.front().temp;
        }
        const Datum* last_datum = &data.front();
        for (unsigned int datum_idx = 1; datum_idx < data.size(); datum_idx++)
        {
            const Datum& datum = data[datum_idx];
            if (datum.flow >= flow)
            {
                return last_datum->temp + (datum.temp - last_datum->temp) * (flow - last_datum->flow) / (datum.flow - last_datum->flow);
            }
            last_datum = &datum;
        }

        return data.back().temp;
    }
}
