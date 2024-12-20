
#include "USBPrinterOutputDeviceManager.h"

#include<exception>
#include <regex>
#include <fstream>
#include <sstream>
#include <thread>
#include "CSerialPort/SerialPort.h"
#include "CSerialPort/SerialPortInfo.h"

USBPrinterOutputDeviceManager::USBPrinterOutputDeviceManager()
{
	connect(this, &USBPrinterOutputDeviceManager::addUSBOutputDeviceSignal, this, &USBPrinterOutputDeviceManager::addOutputDevice);
}

void USBPrinterOutputDeviceManager::start()
{
	_check_updates = true;
	auto t = std::thread(&USBPrinterOutputDeviceManager::updateThread, this);
	t.detach();
}

void USBPrinterOutputDeviceManager::stop()
{
	_check_updates = true;
}

void USBPrinterOutputDeviceManager::startUSBPrint(const QString& fileName, int printTime)
{
	startPrint("", fileName.toStdString(), printTime);
}

void USBPrinterOutputDeviceManager::addPrinter(const std::string& comName, const int& baudrate)
{
	if (_usb_output_devices.find(comName) != _usb_output_devices.end())
	{
		return;
	}
	_cur_com = comName;
	USBPrinterOutputDevice* printer = new USBPrinterOutputDevice();
	printer->setBaudRate(baudrate);
	printer->setComName(comName);
	printer->setName(comName);
	printer->setAddress(comName);
	_usb_output_devices[comName] = printer;
	emit outputDevicesChanged();
	printer->connect();
}

void USBPrinterOutputDeviceManager::startPrint(const std::string& comName, const std::string& fileName, const time_t& printingTime)
{
	if (!comName.empty())
	{
		_cur_com = comName;
	}
	if (_cur_com.empty())
	{
		return;
	}
	const auto printer = _usb_output_devices[_cur_com];
	//printer->connect();
	printer->requestWrite(fileName, printingTime);
}

void USBPrinterOutputDeviceManager::pausePrint(const std::string& comName)
{
	if (!comName.empty())
	{
		_cur_com = comName;
	}
	const auto printer = _usb_output_devices[_cur_com];
	printer->pausePrint();
}

void USBPrinterOutputDeviceManager::resumePrint(const std::string& comName)
{
	if (!comName.empty())
	{
		_cur_com = comName;
	}
	const auto printer = _usb_output_devices[_cur_com];
	printer->resumePrint();
}

void USBPrinterOutputDeviceManager::cancelPrint(const std::string& comName)
{
	if (!comName.empty())
	{
		_cur_com = comName;
	}
	const auto printer = _usb_output_devices[_cur_com];
	printer->cancelPrint();
}

QList<QObject*> USBPrinterOutputDeviceManager::printerOutputDevices()
{
	QList<QObject*> varList;
	varList.clear();
	for (const auto& item : _usb_output_devices)
	{
		varList.push_back(item.second);
	}
	return varList;
}

void USBPrinterOutputDeviceManager::updateThread()
{
	while (_check_updates)
	{
		auto port_list = getSerialPortList();
		addRemovePorts(port_list);
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
}

std::vector<std::string> USBPrinterOutputDeviceManager::getSerialPortList()
{
	std::vector<std::string> base_list;
	auto infos = itas109::CSerialPortInfo::availablePortInfos();
	for (const auto& info : infos)
	{
		if (info.description.substr(0, strlen("USB")) == "USB")
		{
			base_list.push_back(info.portName);
		}
	}
	return base_list;
}

void USBPrinterOutputDeviceManager::addRemovePorts(std::vector<std::string> serial_ports)
{
	for (const auto& serial_port : serial_ports)
	{
		if (std::find(_serial_port_list.cbegin(), _serial_port_list.cend(), serial_port) == _serial_port_list.cend())
		{
			emit addUSBOutputDeviceSignal(serial_port.c_str());  // Hack to ensure its created in main thread
			//addPrinter(serial_port);
			continue;
		}
	}
	_serial_port_list.clear();
	for (const auto& serial_port : serial_ports)
	{
		_serial_port_list.push_back(serial_port);
	}
	auto iter = _usb_output_devices.begin();
	for (; iter != _usb_output_devices.end();)
	{
		if (std::find(_serial_port_list.cbegin(), _serial_port_list.cend(), iter->first) == _serial_port_list.end())
		{
			iter->second->close();
			iter = _usb_output_devices.erase(iter);
			emit outputDevicesChanged();
		}
		else
		{
			iter++;
		}
	}
}

void USBPrinterOutputDeviceManager::addOutputDevice(QString comName)
{
	addPrinter(comName.toStdString());
}

void USBPrinterOutputDeviceManager::onMachineChanged(QString printerName)
{
	if (_usb_output_devices.empty())
	{
		return;
	}
	const auto printer = _usb_output_devices[_cur_com];
	if (printer != nullptr)
	{
		printer->updatePrinterName(printerName.toStdString());
	}
}