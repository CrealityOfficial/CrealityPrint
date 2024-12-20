#include "lockplateaction.h"
#include "kernel/kernelui.h"
#include "interface/spaceinterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"
#include "interface/printerinterface.h"
#include "internal/multi_printer/printer.h"

namespace creative_kernel
{
    LockPlateAction::LockPlateAction(QObject* parent):ActionCommand(parent)
    {
        m_actionname = tr("Lock Plate / Unlock Plate");
        m_actionNameWithout = "Lock Plate / Unlock Plate";
        m_source = "qrc:/UI/photo/menuImg/lock_plate_n.svg";
        m_icon = "qrc:/UI/photo/menuImg/lock_plate_p.svg";

        addUIVisualTracer(this,this);
    }

    LockPlateAction::~LockPlateAction()
    {
    }

    void LockPlateAction::execute()
    {
        Printer* printer = getSelectedPrinter();
        printer->setLock(!printer->lock());

    }


    void LockPlateAction::onThemeChanged(ThemeCategory category)
    {
    }

    void LockPlateAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Lock Plate / Unlock Plate");
    }

    bool LockPlateAction::enabled()
    {
        return true;
    }
}
