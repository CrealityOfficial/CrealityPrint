#ifndef SMOOTHSPEEDACC_INFILL274874
#define SMOOTHSPEEDACC_INFILL274874
#include <vector>
//#include "types/Temperature.h"
//#include "types/Velocity.h"
//#include "types/FlowTempGraph.h"
//#include "settings/Settings.h"

#include "libslic3r/PrintConfig.hpp"

namespace Slic3r
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

    struct Datum
    {
        double flow; //!< The flow in mm^3/s
        double temp; //!< The temperature in *C
        Datum(const double flow, const double temp)
            : flow(flow)
            , temp(temp)
        {}
    };

    class FlowTempGraph
    {
    public:

        void init_limit(const std::string& limit);
        double getTemp(const double flow, const double material_print_temperature, bool flow_dependent_temperature = true) const;

        std::vector<Datum> data; 
    };

    class SmoothSpeedAcc
    {
    public:
        SmoothSpeedAcc();
        ~SmoothSpeedAcc();
        void init_limit(const DynamicPrintConfig& setting);
        bool get_acceleration_limit_mess_enable();
        bool get_acceleration_limit_height_enable();

        void detect_speed(double& speed, const double value_e, const double current_layer_z);
        void detect_speed_min(double& speed, const double value_e, const double current_layer_z);
        void detect_acc(double& acc, const double value_e, const double current_layer_z);

        void calculatMaxSpeedLimitToHeight(const FlowTempGraph& speed_limit_to_height, const float current_layer_z);

    protected:
        bool detect_limit_acc(const LimitType& limitType, const double value_e, const double current_layer_z);
        bool detect_limit_temp(const LimitType& limitType, const double value_e, const double current_layer_z);
        bool detect_limit_speed(const LimitType& limitType, const double value_e, const double current_layer_z);
    private:
        void paraseLimitStr(std::string str, std::vector<LimitData>& outData, const double& init_limit_speed, const double& init_limit_acc, const double& init_limit_temp);
    private:
        double material_diameter;
        double material_density;

        std::vector<LimitData> vctacceleration_limit_mass;
        std::vector<LimitData> vctacceleration_limit_height;
        double current_limit_Temp = -1.0f;
        double current_limit_Acc = -1.0f;
        double current_limit_speed = -1.0f;
        double max_speed_limit_to_height;

        bool acceleration_limit_mess_enable;
        bool acceleration_limit_height_enable;
    };

}

#endif  //SMOOTHSPEEDACC_INFILL274874
