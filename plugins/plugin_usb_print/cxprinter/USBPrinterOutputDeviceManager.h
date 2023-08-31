#ifndef _USB_PRINTER_MANAGER_H
#define _USB_PRINTER_MANAGER_H
#include<list>
#include<mutex>
#include<memory>
#include<string>
#include "USBPrinterOutputDevice.h"
#include <QAbstractListModel>

class USBPrinterOutputDeviceManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QObject*> printerOutputDevices READ printerOutputDevices NOTIFY outputDevicesChanged)
public:
    USBPrinterOutputDeviceManager();

    Q_INVOKABLE void startUSBPrint(const QString& fileName, int printTime);  //printTime s
public:
    void start();
    void stop();
    void addPrinter(const std::string& comName, const int& baudrate=0);
    void startPrint(const std::string& comName, const std::string& fileName, const time_t& printingTime=0);
    void pausePrint(const std::string& comName);
    void resumePrint(const std::string& comName);
    void cancelPrint(const std::string& comName);
    QList<QObject*> printerOutputDevices();
private:
    void updateThread();
    std::vector<std::string> getSerialPortList();
    void addRemovePorts(std::vector<std::string>);

signals:
    void outputDevicesChanged();
    void addUSBOutputDeviceSignal(QString);

public slots:
    void addOutputDevice(QString comName);
    void onMachineChanged(QString printerName);

private:
    std::vector<std::string> _serial_port_list;
    std::map<std::string, USBPrinterOutputDevice*> _usb_output_devices;
    std::string _cur_com;
    bool _check_updates = true;
};
#endif