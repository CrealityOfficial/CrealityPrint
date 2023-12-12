#include "smoothspeedacc.h"
#include <algorithm>

#define _MM2INT(n) ((n) * 1000 + 0.5 * (((n) > 0) - ((n) < 0)))
namespace cura52
{
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


    void SmoothSpeedAcc::paraseLimitStr(std::string str, std::vector<LimitData>& outData, const Velocity& init_limit_speed, const Acceleration& init_limit_acc, const Temperature& init_limit_temp)
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
            limitData.speed1 = outData.empty() ? init_limit_speed.value : outData.back().speed2;
            limitData.Acc1 = outData.empty() ? init_limit_acc.value : outData.back().Acc2;
            limitData.Temp1 = outData.empty() ? init_limit_temp.value : outData.back().Temp2;

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

    void SmoothSpeedAcc::detect_speed(Velocity& speed, const double value_e, const double current_layer_z)
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

    void SmoothSpeedAcc::detect_acc(Acceleration& acc, const double value_e, const double current_layer_z)
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

    void SmoothSpeedAcc::init_limit(Settings* setting)
    {
        if (setting)
        { 
            material_diameter = setting->get<double>("material_diameter");
            material_density = setting->get<double>("material_density");

            //todo
            //init_limit_speed
            Velocity speed1 = setting->get<Velocity>("speed_wall_0");
            Velocity speed2 = setting->get<Velocity>("speed_wall_x");
            Velocity speed3 = setting->get<Velocity>("speed_infill");
            Velocity init_limit_speed = std::max(std::max(speed1, speed2), speed3);

            //init_limit_acc
            Acceleration acc1 = setting->get<Acceleration>("acceleration_infill");
            Acceleration acc2 = setting->get<Acceleration>("acceleration_wall_0");
            Acceleration acc3 = setting->get<Acceleration>("acceleration_wall_x");
            Acceleration acc4 = setting->get<Acceleration>("acceleration_roofing");
            Acceleration acc5 = setting->get<Acceleration>("acceleration_topbottom");
            Acceleration acc6 = setting->get<Acceleration>("acceleration_travel");
            Acceleration acc7 = setting->get<Acceleration>("acceleration_print_layer_0");
            Acceleration acc8 = setting->get<Acceleration>("acceleration_travel_layer_0");

            Acceleration init_limit_acc = std::max(std::max(std::max(std::max(std::max(std::max(std::max(acc1, acc2), acc3), acc4), acc5), acc6), acc7), acc8);

            //init_limit_temp
            Temperature init_limit_temp = setting->get<Temperature>("material_print_temperature");

            if (setting->get<bool>("acceleration_limit_mess_enable"))
            {
                std::string str = setting->get<std::string>("acceleration_limit_mess");

                paraseLimitStr(str, vctacceleration_limit_mass, init_limit_speed, init_limit_acc, init_limit_temp);
                acceleration_limit_mess_enable = true;;
            }
            if (setting->get<bool>("speed_limit_to_height_enable"))
            {
                std::string str = setting->get<std::string>("speed_limit_to_height");

                paraseLimitStr(str, vctacceleration_limit_height, init_limit_speed, init_limit_acc, init_limit_temp);
                acceleration_limit_height_enable = true;
            }
        }
    }

    bool SmoothSpeedAcc::detect_limit_acc(const LimitType& limitType, const double value_e, const double current_layer_z)
    {
        double value = 0.0f;

        if (limitType == LimitType::LIMIT_MESS)
        {
            double volume = value_e * (0.25 * material_diameter * material_diameter * 3.1415926);
            value = volume * (material_density / 1000.0);
        }
        else if (limitType == LimitType::LIMIT_HEIGHT)
        {
            value = current_layer_z;
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
            double volume = value_e * (0.25 * material_diameter * material_diameter * 3.1415926);
            value = volume * (material_density / 1000.0);
        }
        else if (limitType == LimitType::LIMIT_HEIGHT)
        {
            value = current_layer_z;
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
            double volume = value_e * (0.25 * material_diameter * material_diameter * 3.1415926);
            value = volume * (material_density / 1000.0);
        }
        else if (limitType == LimitType::LIMIT_HEIGHT)
        {
            value = current_layer_z;
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
}