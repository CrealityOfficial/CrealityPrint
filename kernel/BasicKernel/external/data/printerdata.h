#ifndef _NULLSPACE_BMP_OBJECT_H
#define _NULLSPACE_BMP_OBJECT_H

#include "basickernelexport.h"
#include <QString>
#include <QHostAddress>
#include <QByteArray>


namespace creative_kernel
{


struct BASIC_KERNEL_API PrinterCommand
{
public:
	static const int CmdScanAsk = 10001;
	static const int CmdConnectAsk = 10010;

	static const int CmdPrinterOnLine = 20001;
	static const int CmdPrinterAcceptConnect = 20010;


	static const int PortDeviceUdp = 45454;
	static const int PortDeviceTcp = 40454;

	static const int PortDesktopUdp = 44454;

};


struct BASIC_KERNEL_API PrinterData
{

public:
	QHostAddress m_hostAddr;
	QString m_mac;
	QString m_printerName;//打印机名
	QString m_printerType;  //打印机类型  01 光固化，02 FDM
	int m_printerStatus;//0 准备好可发送 1繁忙不可发送
	

	QString toString() const;
	int m_status = 0;
	void receivePrinterMsg(QByteArray& datagram, QHostAddress& targetaddr);
};



}

#endif // _NULLSPACE_BMP_OBJECT_H
