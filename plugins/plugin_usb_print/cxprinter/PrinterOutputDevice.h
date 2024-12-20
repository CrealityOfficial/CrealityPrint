#ifndef _PRINTER_OUTPUT_DEVICE_H
#define _PRINTER_OUTPUT_DEVICE_H

#include <QtCore>
#include "OutputDevice.h"
#include "PrinterOutputModel.h"

enum class ConnectionState : short
{
    Closed = 0,
    Connecting = 1,
    Connected = 2,
    Busy = 3,
    Error = 4
};

enum class ConnectionType : short
{
    NotConnected = 0,
    UsbConnection = 1,
    NetworkConnection = 2,
    CloudConnection = 3
};

class PrinterOutputDevice : public OutputDevice
{
    Q_OBJECT
    Q_PROPERTY(QString address READ address NOTIFY connectionTextChanged)
    Q_PROPERTY(QString connectionText READ connectionText)
    Q_PROPERTY(QObject* activePrinter READ activePrinter NOTIFY printersChanged)
    Q_PROPERTY(bool acceptsCommands READ acceptsCommands NOTIFY acceptsCommandsChanged)
public:
    PrinterOutputDevice();
public:
    QString address() const { return _address.c_str(); }

    void setAddress(const std::string& address) {
        _address = address.c_str();
    }

    QString connectionText() {
        return _connection_text.c_str(); 
    }

    QObject* activePrinter() {
        if (_printers.empty())
        {
            return nullptr;
        }
        return _printers[0];
    }

    bool acceptsCommands()
    {
        return true;
    }

    void addActivePrinter(PrinterOutputModel* printer) {
        _printers.push_back(printer);
        emit printersChanged();
    }

protected:
    bool can_pause = true;
    bool can_abort = true;
    bool can_pre_heat_bed = true;
    bool can_pre_heat_hotends = true;
    bool can_send_raw_gcode = true;
    bool can_control_manually = true;
    bool can_update_firmware = false;
    std::string _address;
    std::string _connection_text;
    std::vector<PrinterOutputModel*> _printers;
signals:
    void printersChanged();
    void connectionStateChanged(QString);
    void acceptsCommandsChanged();
    void materialIdChanged();
    void hotendIdChanged();
    void connectionTextChanged();
    void uniqueConfigurationsChanged();
};
#endif