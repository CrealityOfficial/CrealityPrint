#include "deleteplateaction.h"
#include "kernel/kernelui.h"
#include "interface/spaceinterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"
#include "interface/printerinterface.h"
#include "internal/multi_printer/printer.h"

namespace creative_kernel
{
    DeletePlateAction::DeletePlateAction(QObject* parent):ActionCommand(parent)
    {
        m_actionname = tr("Delete Plate");
        m_actionNameWithout = "Delete Plate";
        m_source = "qrc:/UI/photo/menuImg/delete_plate_n.svg";
        m_icon = "qrc:/UI/photo/menuImg/delete_plate_p.svg";

        addUIVisualTracer(this,this);
    }

    DeletePlateAction::~DeletePlateAction()
    {
    }

    void DeletePlateAction::execute()
    {
        removePrinter(getSelectedPrinter()->index(), true);
    }


    void DeletePlateAction::onThemeChanged(ThemeCategory category)
    {
    }

    void DeletePlateAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Delete Plate");
    }

    bool DeletePlateAction::enabled()
    {
        return printersCount() > 1;
    }
}
