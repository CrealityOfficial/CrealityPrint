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
	QString m_printerName;//��ӡ����
	QString m_printerType;  //��ӡ������  01 ��̻���02 FDM
	int m_printerStatus;//0 ׼���ÿɷ��� 1��æ���ɷ���
	

	QString toString() const;
	int m_status = 0;
	void receivePrinterMsg(QByteArray& datagram, QHostAddress& targetaddr);
};



}

#endif // _NULLSPACE_BMP_OBJECT_H
