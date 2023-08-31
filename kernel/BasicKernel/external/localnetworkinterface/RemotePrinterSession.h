#ifndef REMOTE_PRINTER_SESSION_H
#define REMOTE_PRINTER_SESSION_H

#include <QObject>
#include "RemotePrinterConstant.h"

struct RemotePrinterSession
{
	std::string uniqueId;//设备唯一标识，有mac地址的用mac地址，对于octoprint和集群来说是服务器地址
	RemotePrinerType type;//远程打印机的类型
	std::string ipAddress;//一对多也是保存服务器地址
	std::string macAddress;
	std::string token;//token或api-key
	std::string previewImg;
	std::string printerName;
	int port;
	int printerId;
	int fluiddPort;
	int mainsailPort;
	bool connected;
	time_t tmLastActive;
	time_t tmLastGetFileList;

	RemotePrinterSession()
		:type(RemotePrinerType::REMOTE_PRINTER_TYPE_NONE),
		port(80),
		fluiddPort(0),
		mainsailPort(0),
		printerId(0),
		connected(false),
		tmLastActive(0),
		tmLastGetFileList(0)
	{
	}
};

#endif // REMOTE_PRINTER_SESSION_H


