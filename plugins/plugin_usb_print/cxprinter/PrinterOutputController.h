#ifndef _PRINTER_OUTPUT_CONTROLLER_H
#define _PRINTER_OUTPUT_CONTROLLER_H

#include <QtCore>

class PrinterOutputDevice;
class PrintJobOutputModel;
class ExtruderOutputModel;
class PrinterOutputModel;

class PrinterOutputController : public QObject
{
    Q_OBJECT
public:
    PrinterOutputController();
    PrinterOutputController(PrinterOutputDevice* output_device);

protected:
    bool can_pause = true;
    bool can_abort = true;
    bool can_pre_heat_bed = true;
    bool can_pre_heat_hotends = true;
    bool can_send_raw_gcode = true;
    bool can_control_manually = true;
    bool can_update_firmware = false;
    PrinterOutputDevice* _output_device = nullptr;

public:
    virtual void setTargetHotendTemperature(PrinterOutputModel* printer, int position, float temperature) = 0;
    virtual void setTargetBedTemperature(PrinterOutputModel* printer, float temperature) = 0;
    virtual void setJobState(PrintJobOutputModel* job, std::string state) = 0;
    virtual void cancelPreheatBed(PrinterOutputModel* printer) = 0;
    virtual void preheatBed(PrinterOutputModel* printer, float temperature, int duration) = 0;
    virtual void cancelPreheatHotend(ExtruderOutputModel* extruder) = 0;
    virtual void preheatHotend(ExtruderOutputModel* extruder, float temperature, int duration) = 0;
    virtual void setHeadPosition(PrinterOutputModel* printer, int x, int y, int z, int speed) = 0;
    virtual void moveHead(PrinterOutputModel* printer, int x, int y, int z, int speed) = 0;
    virtual void homeBed(PrinterOutputModel* printer) = 0;
    virtual void homeHead(PrinterOutputModel* printer) = 0;
    virtual void sendRawCommand(PrinterOutputModel* printer, std::string command) = 0;
};
#endif