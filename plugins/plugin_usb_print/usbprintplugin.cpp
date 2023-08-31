#include "usbprintplugin.h"
#include "interface/uiinterface.h"

using namespace creative_kernel;

UsbPrintPlugin::UsbPrintPlugin(QObject* parent)
	:QObject(parent)
    , m_usbPrintUI(nullptr)
{
     m_usbPrinterModel = new USBPrinterOutputDeviceManager();
     //m_usbPrinterModel->start();
}

UsbPrintPlugin::~UsbPrintPlugin()
{
}

QString UsbPrintPlugin::name()
{
	return "UsbPrintPlugin";
}

QString UsbPrintPlugin::info()
{
	return "";
}

void UsbPrintPlugin::initialize()
{
    registerContextObject("plugin_usb_printer", m_usbPrinterModel);
    m_usbPrintUI = createQmlObjectFromQrc(":/usbprint/UsbControlDlg.qml");
}

void UsbPrintPlugin::uninitialize()
{

}

