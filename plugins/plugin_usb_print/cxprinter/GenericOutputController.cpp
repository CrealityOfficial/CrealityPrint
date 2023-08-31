
#include "GenericOutputController.h"
#include "PrinterOutputDevice.h"

GenericOutputController::GenericOutputController()
{

}

GenericOutputController::GenericOutputController(PrinterOutputDevice* output_device)
	:PrinterOutputController(output_device)
{
    _preheat_bed_timer.setSingleShot(true);
    QObject::connect(&_preheat_bed_timer, &QTimer::timeout, this, &GenericOutputController::onPreheatBedTimerFinished);
    _preheat_hotends_timer.setSingleShot(true);
    QObject::connect(&_preheat_hotends_timer, &QTimer::timeout, this, &GenericOutputController::onPreheatHotendsTimerFinished);
}

void GenericOutputController::setTargetHotendTemperature(PrinterOutputModel* printer, int position, float temperature)
{
    std::string cmd = "M104 S";
    cmd = cmd + std::to_string(temperature) + " T" + std::to_string(position);
    _output_device->sendCommand(cmd);
}

void GenericOutputController::setTargetBedTemperature(PrinterOutputModel* printer, float temperature)
{
    std::string cmd = "M140 S";
    cmd = cmd + std::to_string((int)std::round(temperature));
    _output_device->sendCommand(cmd);
}

void GenericOutputController::setJobState(PrintJobOutputModel* job, std::string state)
{
    if (state == "pause")
    {
        _output_device->pausePrint();
        job->updateState("paused");
    }
    else if (state == "print")
    {
        _output_device->resumePrint();
        job->updateState("printing");
    }
    else if (state == "abort")
    {
        job->updateState("abort");
        _output_device->cancelPrint();
    }
}

void GenericOutputController::cancelPreheatBed(PrinterOutputModel* printer)
{
    setTargetBedTemperature(printer, 0);
    _preheat_bed_timer.stop();
    printer->updateIsPreheating(false);
}

void GenericOutputController::preheatBed(PrinterOutputModel* printer, float temperature, int duration)
{
    setTargetBedTemperature(printer, temperature);
    _preheat_bed_timer.setInterval(duration * 1000);
    //_preheat_bed_timer.start();
    _preheat_printer = printer;
    //printer->updateIsPreheating(true);
}

void GenericOutputController::cancelPreheatHotend(ExtruderOutputModel* extruder)
{
    setTargetHotendTemperature(extruder->getPrinter(), extruder->getPosition(), 0);
    if (_preheat_hotends.find(extruder) != _preheat_hotends.end())
    {
        extruder->updateIsPreheating(false);
        _preheat_hotends.erase(extruder);
    }
    if (_preheat_hotends.empty() && _preheat_hotends_timer.isActive())
    {
        _preheat_hotends_timer.stop();
    }
}

void GenericOutputController::preheatHotend(ExtruderOutputModel* extruder, float temperature, int duration)
{
    auto position = extruder->getPosition();
    auto number_of_extruders = extruder->getPrinter()->extruders().length();
    if (position >= number_of_extruders)
    {
        return;
    }
    setTargetHotendTemperature(extruder->getPrinter(), position, temperature);
    _preheat_hotends_timer.setInterval(duration * 1000);
    //_preheat_hotends_timer.start();
    _preheat_hotends.insert(extruder);
    //extruder->updateIsPreheating(true);
}

void GenericOutputController::setHeadPosition(PrinterOutputModel* printer, int x, int y, int z, int speed)
{
}

void GenericOutputController::moveHead(PrinterOutputModel* printer, int x, int y, int z, int speed)
{
	_output_device->sendCommand("G91");
    std::string cmd = "G0 X";
    cmd = cmd + std::to_string(x) + " Y" + std::to_string(y) + " Z" + std::to_string(z) + " F" + std::to_string(speed);
    _output_device->sendCommand(cmd);
	_output_device->sendCommand("G90");
}

void GenericOutputController::homeBed(PrinterOutputModel* printer)
{
	_output_device->sendCommand("G28 Z");
}

void GenericOutputController::homeHead(PrinterOutputModel* printer)
{
	_output_device->sendCommand("G28 X Y");
}

void GenericOutputController::sendRawCommand(PrinterOutputModel* printer, std::string command)
{
    _output_device->sendCommand(command);
}

void GenericOutputController::onPreheatBedTimerFinished()
{
    if (_preheat_printer == nullptr)
        return;
    setTargetBedTemperature(_preheat_printer, 0);
    _preheat_printer->updateIsPreheating(false);
}

void GenericOutputController::onPreheatHotendsTimerFinished()
{
    for (const auto& extruder : _preheat_hotends)
    {
        setTargetHotendTemperature(extruder->getPrinter(), extruder->getPosition(), 0);
    }
    _preheat_hotends.clear();
}
