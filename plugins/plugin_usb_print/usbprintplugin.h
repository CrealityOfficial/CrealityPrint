#ifndef _UsbPrintPlugin_H
#define _UsbPrintPlugin_H

#include <QtQml/QQmlComponent>

#include "qtusercore/module/creativeinterface.h"
#include "cxprinter/USBPrinterOutputDeviceManager.h"

class UsbPrintPlugin: public QObject, public CreativeInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "creative.UsbPrintPlugin")
    Q_INTERFACES(CreativeInterface)
public:
    UsbPrintPlugin(QObject* parent = nullptr);
    virtual ~UsbPrintPlugin();

    QString name() override;
    QString info() override;

    void initialize() override;
    void uninitialize() override;

protected:
	QObject* m_usbPrintUI;
    USBPrinterOutputDeviceManager* m_usbPrinterModel = nullptr;
};
#endif // _UsbPrintPlugin_H
