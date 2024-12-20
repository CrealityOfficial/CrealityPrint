#include "usbprintcommand.h"
#include "kernel/kernelui.h"
#include <QDebug>

#include "interface/commandinterface.h"

using namespace creative_kernel;
UsbPrintCommand::UsbPrintCommand(QObject* parent)
    :ActionCommand(parent)
{
    m_actionname = tr("USB Printing");
    m_actionNameWithout = "USB Printing";
    m_eParentMenu = eMenuType_PrinterControl;

    addUIVisualTracer(this,this);
}

UsbPrintCommand::~UsbPrintCommand()
{
}

void UsbPrintCommand::execute()
{
    QObject* pmainWinbj = getKernelUI()->getUI("UIAppWindow");
    QMetaObject::invokeMethod(pmainWinbj, "showFdmPmDlg", Q_ARG(QVariant, QVariant::fromValue(QString())));
}

void UsbPrintCommand::onThemeChanged(creative_kernel::ThemeCategory category)
{
}

void UsbPrintCommand::onLanguageChanged(creative_kernel::MultiLanguage language)
{
    m_actionname = tr("USB Printing");
}
