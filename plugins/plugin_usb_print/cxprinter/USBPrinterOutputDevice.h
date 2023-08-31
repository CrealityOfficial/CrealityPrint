#ifndef _USB_PRINTER_H
#define _USB_PRINTER_H
#include<list>
#include<mutex>
#include<memory>
#include<string>
#include <array>
#include <queue>
#include "CSerialPort/SerialPort.h"
#include "PrinterOutputDevice.h"
#include "AutoDetectBaudJob.h"

class USBPrinterOutputDevice : public PrinterOutputDevice
{
	Q_OBJECT
public:
	USBPrinterOutputDevice();
	~USBPrinterOutputDevice() {}

public:
	void requestWrite(const std::string& file_name, const time_t& printingTime=0);
	void printGCode(const std::string& file_name);
	void setBaudRate(const int& baud_rate);
	void setComName(const std::string& file_name);
	void connect();
	void close();
	void sendCommand(const std::string& command);
	void doSendCommand(const std::string& command);
	void update();
	void pausePrint();
	void resumePrint();
	void cancelPrint();
	void sendNextGcodeLine();
	void updatePrinterName(const std::string& printerName);
private:
	void setConnectionState(const ConnectionState& state);
private:
	ConnectionState _connection_state;

	itas109::CSerialPort* _serial = nullptr;
	std::string _serial_port;
	int _timeout = 3;
	std::vector<std::string> _gcode;
	int _progress = 0;
	int _gcode_position = 0;
	bool _use_auto_detect = true;
	int _baud_rate = 0;
	std::vector<int> _all_baud_rates = { 115200, 250000, 500000, 230400, 76800, 57600, 38400, 19200, 9600 };
	time_t _last_temperature_request = 0;
	int _firmware_idle_count = 0;
	bool _is_printing = false;
	time_t _print_start_time = 0;
	int _print_estimated_time;
	bool _accepts_commands = true;
	bool _paused = false;
	bool _printer_busy = false;
	std::list<std::string> _command_queue;
	std::mutex _command_queue_mutex;
	std::mutex _command_write_mutex;
	bool _write_flag = true;
	std::thread* _update_thread;
	AutoDetectBaudJob* _autoDetectBaud = nullptr;
	std::string _job_name;
public slots:
	void autoDetectFinished(int baud);
};
#endif