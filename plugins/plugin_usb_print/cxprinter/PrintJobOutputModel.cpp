
#include "PrintJobOutputModel.h"

PrintJobOutputModel::PrintJobOutputModel()
{

}

PrintJobOutputModel::PrintJobOutputModel(PrinterOutputController* output_controller, std::string name)
	:_output_controller(output_controller),
	_name(name)
{

}

void PrintJobOutputModel::setState(QString state)
{
	_output_controller->setJobState(this, state.toStdString());
}