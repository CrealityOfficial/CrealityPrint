#include "printerdata.h"
#include <QString>
#include <QStringList>

namespace creative_kernel
{


QString PrinterData::toString() const
{
    QString statusStr = m_printerStatus == 0 ? "idle" : "busy";
    return m_mac + "\t" + m_hostAddr.toString() + "\t" + m_printerName+"\t" + statusStr;
}

void PrinterData::receivePrinterMsg(QByteArray& datagram, QHostAddress& targetaddr)
{
    QString data(datagram);
    QStringList strlist = data.split('\t');
    int command = strlist.at(0).toInt();
    if (command == PrinterCommand::CmdPrinterOnLine)
    {
        PrinterData pd;
        pd.m_mac = strlist.at(1);
        pd.m_printerName = strlist.at(2);
        pd.m_printerType = strlist.at(3);
        pd.m_printerStatus = strlist.at(4).toInt();
        pd.m_hostAddr = targetaddr;
    }
}

}