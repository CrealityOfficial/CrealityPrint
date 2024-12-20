//Copyright (c) 2022 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#include <assert.h>
#include "communication/gcodeExport.h"
#include "NozzleTempInsert.h"

namespace cura52
{

    NozzleTempInsert::NozzleTempInsert(unsigned int path_idx, int extruder, double temperature, bool wait, double time_after_path_start)
        : path_idx(path_idx)
        , time_after_path_start(time_after_path_start)
        , extruder(extruder)
        , temperature(temperature)
        , wait(wait)
    {
        assert(temperature != 0 && temperature != -1 && "Temperature command must be set!");
    }

    void NozzleTempInsert::write(GCodeExport& gcode)
    {
        //���������л�ʱ����ǰ���ȣ������������Զ�����Ԥ�ȹ���
        //if (gcode.getExtruderNum() == 1 )
        {
            gcode.writeTemperatureCommand(extruder, temperature, wait);
        }
    }

    FanInsert::FanInsert(unsigned int path_idx, int fan_idx, double fan_speed, double time_after_path_start)
        : path_idx(path_idx)
        , fan_speed(fan_speed)
        , fan_idx(fan_idx)
        , time_after_path_start(time_after_path_start)
    {

    }

    void FanInsert::write(GCodeExport& gcode)
    {
        if (fan_idx == 0)
        {
            if (gcode.getCurrentFanSpeed() < fan_speed)
                gcode.writeFanCommand(fan_speed);
        }
        else
        {
            if (gcode.getCurrentCdsFanSpeed() < fan_speed)
                gcode.writeCdsFanCommand(fan_speed);
        }
    }

}