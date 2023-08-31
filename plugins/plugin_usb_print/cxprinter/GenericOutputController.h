#ifndef _GENERIC_OUTPUT_CONTROLLER_H
#define _GENERIC_OUTPUT_CONTROLLER_H

#include <QtCore>
#include "PrinterOutputController.h"
#include <set>

class GenericOutputController : public PrinterOutputController
{
    Q_OBJECT
public:
    GenericOutputController();
    GenericOutputController(PrinterOutputDevice* output_device);

public:
    void setTargetHotendTemperature(PrinterOutputModel* printer, int position, float temperature);
    void setTargetBedTemperature(PrinterOutputModel* printer, float temperature);
    void setJobState(PrintJobOutputModel* job, std::string state);
    void cancelPreheatBed(PrinterOutputModel* printer);
    void preheatBed(PrinterOutputModel* printer, float temperature, int duration);
    void cancelPreheatHotend(ExtruderOutputModel* extruder);
    void preheatHotend(ExtruderOutputModel* extruder, float temperature, int duration);
    void setHeadPosition(PrinterOutputModel* printer, int x, int y, int z, int speed);
    void moveHead(PrinterOutputModel* printer, int x, int y, int z, int speed);
    void homeBed(PrinterOutputModel* printer);
    void homeHead(PrinterOutputModel* printer);
    void sendRawCommand(PrinterOutputModel* printer, std::string command);
    void onPreheatBedTimerFinished();
    void onPreheatHotendsTimerFinished();
private:
    QTimer _preheat_bed_timer;
    QTimer _preheat_hotends_timer;
    PrinterOutputModel* _preheat_printer = nullptr;
    std::set<ExtruderOutputModel*> _preheat_hotends;
};
#endif