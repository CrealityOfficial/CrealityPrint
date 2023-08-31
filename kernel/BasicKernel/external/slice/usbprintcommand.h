#ifndef _USBPRINTCOMMAND_H
#define _USBPRINTCOMMAND_H
#include "menu/actioncommand.h"
#include "data/interface.h"

class UsbPrintCommand : public ActionCommand
    , public creative_kernel::UIVisualTracer
{
    Q_OBJECT
public:
    UsbPrintCommand(QObject* parent = nullptr);
    virtual ~UsbPrintCommand();

    Q_INVOKABLE void execute();
protected:
    void onThemeChanged(creative_kernel::ThemeCategory category) override;
    void onLanguageChanged(creative_kernel::MultiLanguage language) override;
private:
    int m_nShowType = 0;
    bool m_bFirstShow = true;

};

#endif //
