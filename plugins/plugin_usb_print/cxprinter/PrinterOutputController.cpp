
#include "PrinterOutputController.h"

PrinterOutputController::PrinterOutputController()
{

}

PrinterOutputController::PrinterOutputController(PrinterOutputDevice* output_device)
{
	_output_device = output_device;
}