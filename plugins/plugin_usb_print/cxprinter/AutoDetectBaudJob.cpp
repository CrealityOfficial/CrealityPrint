#include "AutoDetectBaudJob.h"
#include "CSerialPort/SerialPort.h"

AutoDetectBaudJob::AutoDetectBaudJob()
{

}

AutoDetectBaudJob::AutoDetectBaudJob(const QString& serial_port)
	:_serial_port(serial_port)
{

}

AutoDetectBaudJob::~AutoDetectBaudJob()
{
}

void AutoDetectBaudJob::run()
{
	int tries = 2;
	int wait_response_timeouts[] = { 3, 15, 30 };
	itas109::CSerialPort* serial = nullptr;
	for (int retry = 0; retry < tries; retry++)
	{
		for (const auto& baud_rate : _all_baud_rates)
		{
			if (serial == nullptr)
			{
				serial = new itas109::CSerialPort(_serial_port.toStdString());
				serial->setDataBits(itas109::DataBits8);     //数据位
				serial->setParity(itas109::ParityNone);    //校验位
				serial->setStopBits(itas109::StopOne);   //停止位
				serial->setBaudRate(baud_rate);
			}
			else
			{
				serial->setBaudRate(baud_rate);
			}
			serial->open();
			std::string data = "\n";
			serial->writeData(data.c_str(), data.length());
			data = "M105\n";
			serial->writeData(data.c_str(), data.length());

			auto start_timeout_time = time(nullptr);
			auto timeout_time = start_timeout_time + wait_response_timeouts[retry];
			char dataRead[2] = { 0 };
			std::string line;
			while(timeout_time > time(nullptr))
			{
				if(serial->readData(dataRead, 1))
				{
					line = line + dataRead;
				}
			}
			if (line.find("ok") || line.find("T:"))
			{
				serial->close();
				emit sigDetectBaud(baud_rate);
				return;
			}
		}
	}
}
