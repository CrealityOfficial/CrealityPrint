
#include "ExtruderOutputModel.h"

ExtruderOutputModel::ExtruderOutputModel()
{

}

ExtruderOutputModel::ExtruderOutputModel(PrinterOutputModel* printer, int position)
	:_printer(printer),
	_position(position)
{

}

void ExtruderOutputModel::preheatHotend(float temperature, float duration)
{
	_printer->getController()->preheatHotend(this, temperature, duration);
}

void ExtruderOutputModel::cancelPreheatHotend()
{
	_printer->getController()->cancelPreheatHotend(this);
}