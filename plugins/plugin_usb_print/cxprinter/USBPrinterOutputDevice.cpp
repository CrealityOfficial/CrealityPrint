
#include<exception>
#include <regex>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "USBPrinterOutputDevice.h"
#include "GenericOutputController.h"

USBPrinterOutputDevice::USBPrinterOutputDevice()
{
	setName("USB printing");
	setShortDescription("Print via USB");
	setDescription("Print via USB");
	setIconName("print");
}

void USBPrinterOutputDevice::requestWrite(const std::string& file_name, const time_t& printingTime)
{
	if (_is_printing)
	{
		return;
	}
	_print_estimated_time = printingTime;
	printGCode(file_name);
}

void USBPrinterOutputDevice::printGCode(const std::string& file_name)
{
	_gcode.clear();
	_paused = false;
	_gcode.push_back("M110");
	_gcode.push_back("M79 S1");
	_gcode.push_back("M72 filename");
	_gcode.push_back("M24");
	_gcode_position = 0;
	_print_start_time = time(nullptr);
	std::ifstream ifs(file_name);
	std::string line;
	while (std::getline(ifs, line))
	{
		if (line[0] != ';')
		{
			_gcode.push_back(line);
		}
	}
	ifs.close();
	auto index = file_name.find_last_of("/");
	_job_name = file_name.substr(index + 1);
	for (size_t i = 0; i < 4; i++)
	{
		sendNextGcodeLine();
	}
	_is_printing = true;
	//self.writeFinished.emit(self)
}

void USBPrinterOutputDevice::setBaudRate(const int& baud_rate)
{
	if (std::find(_all_baud_rates.cbegin(), _all_baud_rates.cend(), baud_rate) == _all_baud_rates.cend())
	{
		return;
	}
	_baud_rate = baud_rate;
}

void USBPrinterOutputDevice::setComName(const std::string& file_name)
{
	_serial_port = file_name;
}

void USBPrinterOutputDevice::connect()
{
	if (_serial != nullptr)
	{
		return;
	}
	if (_baud_rate == 0)
	{
		_autoDetectBaud = new AutoDetectBaudJob(_serial_port.c_str());
		QObject::connect(_autoDetectBaud, &AutoDetectBaudJob::sigDetectBaud, this, &USBPrinterOutputDevice::autoDetectFinished);
		_autoDetectBaud->start();
		return;
	}
	_serial = new itas109::CSerialPort(_serial_port);
	_serial->setBaudRate(_baud_rate);
	_serial->setDataBits(itas109::DataBits8);     //数据位
	_serial->setParity(itas109::ParityNone);    //校验位
	_serial->setStopBits(itas109::StopOne);   //停止位
	_serial->open();
	setConnectionState(ConnectionState::Connected);
	PrinterOutputController* controller = new GenericOutputController(this);
	PrinterOutputModel* printer = new PrinterOutputModel(controller, 1);
	addActivePrinter(printer);
	auto t = std::thread(&USBPrinterOutputDevice::update, this);
	t.detach();
}

void USBPrinterOutputDevice::close()
{
	if (_serial)
	{
		_serial->close();
	}
	delete _serial;
	_serial = nullptr;
}


void USBPrinterOutputDevice::sendCommand(const std::string& command)
{
	_command_write_mutex.lock();
	if (_write_flag == true)
	{
		_command_write_mutex.unlock();
		doSendCommand(command);
	}
	else
	{
		_command_queue.push_back(command);
		_command_write_mutex.unlock();
	}
}

void USBPrinterOutputDevice::doSendCommand(const std::string& cmd)
{
	{
		std::lock_guard<std::mutex> locker(_command_write_mutex);
		if (_is_printing)
		{
			_write_flag = false;
		}
	}
	if (_serial == nullptr || _connection_state != ConnectionState::Connected)
	{
		return;
	}
	std::string command = cmd;
	if (command[command.length()-1] != '\n')
	{
		command = command + '\n';
	}
	if (_serial->isOpened()) {
		auto res = _serial->writeData(command.c_str(), command.length());
	}
}

void USBPrinterOutputDevice::update()
{
	std::string line;
	time_t lastReadTime = 0;
	while (_connection_state == ConnectionState::Connected && _serial != nullptr && _serial->isOpened())
	{
		if (_is_printing && abs(time(nullptr) - lastReadTime) > 3)
		{
			doSendCommand("M105");
			lastReadTime = time(nullptr);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}

		char data[2] = { 0 };
		bool hasData = false;
		bool hasLine = false;

		int numRead = 0;
		while (numRead = _serial->readData(data, 1))
		{
			if (numRead == -1)
			{
				return;
			}
			hasData = true;
			line = line + data;
			if (data[0] == '\n')
			{
				hasLine = true;
				break;
			}
		}

		if (!hasData)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}
		std::string receivedMsg;// = line;
		if (!hasLine)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}
		else
		{
			receivedMsg = line;
			line.clear();
		}
		auto timeNow = time(nullptr);
		lastReadTime = timeNow;
		if (_last_temperature_request == 0 || abs(timeNow - _last_temperature_request) > _timeout)
		{
			if (!_printer_busy)
			{
				sendCommand("M105");
			}
			_last_temperature_request = timeNow;
		}
		std::smatch match;
		if (std::regex_search(receivedMsg.cbegin(), receivedMsg.cend(), match, std::regex("[B|T\\d*]: ?\\d+\\.?\\d*")))
		{
			std::smatch extruder_temperature_matches;
			auto it = receivedMsg.cbegin();
			auto itEnd = receivedMsg.cend();
			while (std::regex_search(it, itEnd, extruder_temperature_matches, std::regex("T(\\d*): ?(\\d+\\.?\\d*)\\s*\\/?(\\d+\\.?\\d*)?")))
			{
				auto extruder_nr = 0;
				if (!extruder_temperature_matches[0].str().empty())
				{
					extruder_nr = atoi(extruder_temperature_matches[0].str().c_str());
				}
				auto extruder = _printers[0]->_extruders[extruder_nr];
				auto HotendTemperature = std::atof(extruder_temperature_matches[2].str().c_str());
				auto TargetHotendTemperature = std::atof(extruder_temperature_matches[3].str().c_str());

				extruder->updateHotendTemperature(HotendTemperature);
				extruder->updateTargetHotendTemperature(TargetHotendTemperature);
				it = extruder_temperature_matches[0].second;
			}

			std::smatch bed_temperature_matches;
			std::regex_search(receivedMsg.cbegin(), receivedMsg.cend(), bed_temperature_matches, std::regex("B: ?(\\d+\\.?\\d*)\\s*\\/?(\\d+\\.?\\d*)?"));
			if (bed_temperature_matches.size() == 3)
			{
				auto BedTemperature = std::atof(bed_temperature_matches[1].str().c_str());
				auto TargetBedTemperature = std::atof(bed_temperature_matches[2].str().c_str());
				_printers[0]->updateBedTemperature(BedTemperature);
				_printers[0]->updateTargetBedTemperature(TargetBedTemperature);
			}
		}
		if (receivedMsg.empty())
		{
			_firmware_idle_count += 1;
		}
		else
		{
			_firmware_idle_count = 0;
		}
		if (receivedMsg.substr(0,2) == "ok" || _firmware_idle_count > 1)
		{
			_printer_busy = false;
			{
				std::lock_guard<std::mutex> locker(_command_write_mutex);
				_write_flag = true;
			}
			if (!_command_queue.empty())
			{
				doSendCommand(_command_queue.front());
				_command_queue.pop_front();
			}
			else if(_is_printing)
			{
				if (!_paused)
				{
					sendNextGcodeLine();
				}
			}
		}
		if (receivedMsg.substr(0, strlen("echo:busy:")) == "echo:busy:")
		{
			_printer_busy = true;
		}
		if (_is_printing)
		{
			if (receivedMsg.substr(0, strlen("!!")) == "!!")
			{
				cancelPrint();
			}
			else
			{
				auto strLower = receivedMsg;
				std::transform(strLower.begin(), strLower.end(), strLower.begin(), tolower);
				if (strLower.substr(0, strlen("resend")) == "resend" || strLower.substr(0, strlen("rs")) == "rs")
				{
					auto position = std::regex_replace(std::regex_replace(std::regex_replace(receivedMsg, std::regex("N:"), " "), std::regex("N"), " "), std::regex(":"), " ");
					auto index = position.find_last_of(" ");
					_gcode_position = std::stoi(position.substr(index));
				}
			}
		}
	}
}

void USBPrinterOutputDevice::pausePrint()
{
	_paused = true;
}

void USBPrinterOutputDevice::resumePrint()
{
	_paused = false;
	sendNextGcodeLine();
}

void USBPrinterOutputDevice::cancelPrint()
{
	_gcode_position = -1;
	_gcode.clear();
	_printers[0]->updateActivePrintJob(nullptr);
	_is_printing = false;
	_paused = false;

	//Turn off temperatures, fanand steppers
	doSendCommand("M140 S0");
	doSendCommand("M104 S0");
	doSendCommand("M107");

	//Home XY to prevent nozzle resting on aborted print
	//Don't home bed because it may crash the printhead into the print on printers that home on the bottom
	_printers[0]->homeHead();
	doSendCommand("M84");
	doSendCommand("M79 S5");
}

void USBPrinterOutputDevice::sendNextGcodeLine()
{
	if (_gcode.empty() || _gcode_position == -1 || _gcode_position == _gcode.size())
	{
		_printers[0]->updateActivePrintJob(nullptr);
		_is_printing = false;
		return;
	}
	auto progress = (double)_gcode_position / _gcode.size();
	int elapsed_time = time(nullptr) - _print_start_time;
	auto estimated_time = _print_estimated_time;
	if (progress > 0.000001)
	{
		//estimated_time = (int)(elapsed_time / progress);
		estimated_time = _print_estimated_time * (1 - progress) + elapsed_time;
	}

	auto result = elapsed_time * 100 / std::max(estimated_time, 1);
	int tempProgress = std::min(result, 100);

	std::string line = "";
	if (tempProgress > _progress)
	{
		_progress = tempProgress;
		line = "M79 T" + std::to_string(_progress);
		doSendCommand(line);
		line = "M79 C" + std::to_string(elapsed_time);
		doSendCommand(line);
		line = "M79 D" + std::to_string(estimated_time);
		doSendCommand(line);
		return;
	}
	else
	{
		line = _gcode[_gcode_position];
		auto index = line.find(";");
		if (index != -1)
		{
			line = line.substr(0, index);
		}

		if (line == "" || line == "M0" || line == "M1")
		{
			line = "M105";
		}
		auto lastIndex = line.length() - 1;
		while (std::isspace(line[lastIndex]))
		{
			line.erase(lastIndex);
			lastIndex--;
			if (lastIndex == -1)
			{
				line = "M105";
				break;
			}
		}
	}

	char checkSum = 0;
	char cmd[256] = { 0 };
	sprintf(cmd, "N%d%s", _gcode_position, line.c_str());
	for (size_t i = strlen(cmd); i > 0; i--)
	{
		checkSum ^= cmd[i - 1];
	}
	sprintf(cmd, "N%d%s*%d", _gcode_position, line.c_str(), checkSum);
	doSendCommand(cmd);
	PrintJobOutputModel* print_job = nullptr;
	auto activePrintJob = _printers[0]->activePrintJob();
	if (activePrintJob != nullptr)
	{
		print_job = dynamic_cast<PrintJobOutputModel*>(_printers[0]->activePrintJob());
	}
	else
	{
		auto controller = new GenericOutputController(this);
		print_job = new PrintJobOutputModel(controller, _job_name);
	}
	print_job->updateState("printing");
	_printers[0]->updateActivePrintJob(print_job);

	print_job->updateTimeElapsed(elapsed_time);
	print_job->updateTimeTotal(estimated_time);
	_gcode_position++;
}

void USBPrinterOutputDevice::updatePrinterName(const std::string& printerName)
{
	QObject* printer = activePrinter();
	if (printer != nullptr)
	{
		dynamic_cast<PrinterOutputModel*>(printer)->updateName(printerName);
	}
}

void USBPrinterOutputDevice::setConnectionState(const ConnectionState& state)
{
	if (_connection_state != state)
		_connection_state = state;
}

void USBPrinterOutputDevice::autoDetectFinished(int baud)
{
	setBaudRate(baud);
	connect();
}
