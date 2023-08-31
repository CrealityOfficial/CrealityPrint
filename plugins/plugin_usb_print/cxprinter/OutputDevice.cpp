
#include "OutputDevice.h"

OutputDevice::OutputDevice()
{

}

OutputDevice::OutputDevice(const std::string& id)
	:_id(id)
{

}

void OutputDevice::requestWrite(const std::string& file_name)
{

}

void OutputDevice::sendCommand(const std::string& command)
{
}

void OutputDevice::pausePrint()
{
}

void OutputDevice::resumePrint()
{
}

void OutputDevice::cancelPrint()
{
}
