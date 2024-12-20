#include "PrinterOutputModel.h"
#include "interface/machineinterface.h"

PrinterOutputModel::PrinterOutputModel()
{
	_extruders.push_back(new ExtruderOutputModel());
}
PrinterOutputModel::PrinterOutputModel(PrinterOutputController* output_controller, int number_of_extruders)
{
	_controller = output_controller;
	for (auto i = 0; i < number_of_extruders; i++)
	{
		_extruders.push_back(new ExtruderOutputModel(this, i));
	}
	_name = creative_kernel::currentMachineName().toStdString();
}

QString PrinterOutputModel::name() const
{
	return _name.c_str();
}

QList<QObject*> PrinterOutputModel::extruders() const
{
	QList<QObject*> varList;
	varList.clear();
	for (const auto& item : _extruders)
	{
		varList.push_back(item);
	}
	return varList;
}

void PrinterOutputModel::updateActivePrintJob(PrintJobOutputModel* print_job)
{
	if (_active_print_job != print_job)
	{
		_active_print_job = print_job;
		emit activePrintJobChanged();
	}
}

void PrinterOutputModel::homeHead()
{
	_controller->homeHead(this);
}

void PrinterOutputModel::homeBed()
{
	_controller->homeBed(this);
}

void PrinterOutputModel::sendRawCommand(QString command)
{
	_controller->sendRawCommand(this, command.toUpper().toStdString());
}

void PrinterOutputModel::moveHead(float x, float y, float z, float speed)
{
	_controller->moveHead(this, x, y, z, speed);
}

void PrinterOutputModel::preheatBed(float temperature, float duration)
{
	_controller->preheatBed(this, temperature, duration);
}
