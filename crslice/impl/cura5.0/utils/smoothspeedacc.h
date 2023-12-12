#ifndef SMOOTHSPEEDACC_INFILL274874
#define SMOOTHSPEEDACC_INFILL274874
#include <vector>
#include "types/Temperature.h"
#include "types/Velocity.h"
#include "types/FlowTempGraph.h"
#include "settings/Settings.h"

namespace cura52
{
    enum class LimitType
    {
        LIMIT_MESS,
        LIMIT_HEIGHT
    };

    struct LimitData
    {
        double value1;
        double value2;
        double speed1;
        double speed2;
        double Acc1;
        double Acc2;
        double Temp1;
        double Temp2;
    };

    class SmoothSpeedAcc
    {
    public:
        SmoothSpeedAcc();
        ~SmoothSpeedAcc();
        void init_limit(Settings* setting);
        bool get_acceleration_limit_mess_enable();
        bool get_acceleration_limit_height_enable();

        void detect_speed(Velocity& speed, const double value_e, const double current_layer_z);
        void detect_acc(Acceleration& acc, const double value_e, const double current_layer_z);

        void calculatMaxSpeedLimitToHeight(const FlowTempGraph& speed_limit_to_height, const float current_layer_z);

    protected:
        bool detect_limit_acc(const LimitType& limitType, const double value_e, const double current_layer_z);
        bool detect_limit_temp(const LimitType& limitType, const double value_e, const double current_layer_z);
        bool detect_limit_speed(const LimitType& limitType, const double value_e, const double current_layer_z);
    private:
        void paraseLimitStr(std::string str, std::vector<LimitData>& outData, const Velocity& init_limit_speed, const Acceleration& init_limit_acc, const Temperature& init_limit_temp);
    private:
        double material_diameter;
        double material_density;

        std::vector<LimitData> vctacceleration_limit_mass;
        std::vector<LimitData> vctacceleration_limit_height;
        Temperature current_limit_Temp = -1.0f;
        Velocity current_limit_Acc = -1.0f;
        Velocity current_limit_speed = -1.0f;
        Velocity max_speed_limit_to_height;

        bool acceleration_limit_mess_enable;
        bool acceleration_limit_height_enable;
    };

}

#endif  //SMOOTHSPEEDACC_INFILL274874