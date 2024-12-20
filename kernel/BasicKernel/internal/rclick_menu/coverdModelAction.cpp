#include "coverdModelAction.h"
#include "interface/layoutinterface.h"
#include "interface/selectorinterface.h"
#include "interface/printerinterface.h"
#include "interface/commandinterface.h"
#include "internal/multi_printer/printer.h"

namespace creative_kernel
{

    CoverModelAction::CoverModelAction(QObject* parent) 
    {
        m_actionname = tr("Fill bed with copies");
        m_actionNameWithout = "Fill bed with copies";

        addUIVisualTracer(this,this);
    }

    CoverModelAction::~CoverModelAction()
    {

    }

    void CoverModelAction::execute()
    {
        QList<ModelGroup*> selects = creative_kernel::selectedGroups();
        if (selects.count() > 1 || selects.count() == 0)
            return;

        int printerIndex = getPrinterIndexByGroupId(selects.at(0)->getObjectId());
        selectPrinter(printerIndex);

        extendFillModelGroup(selects[0]->getObjectId(), printerIndex);
    }

    bool CoverModelAction::enabled()
    {
        QList<ModelGroup*> selects = creative_kernel::selectedGroups();
        if (selects.count() > 1 || selects.count() == 0)
            return false;

        int printerIdx = getPrinterIndexByGroupId(selects[0]->getObjectId());
        if (printerIdx < 0)
            return false;

        return true;
    }

    void CoverModelAction::onThemeChanged(ThemeCategory category)
    {
        
    }

    void CoverModelAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Fill bed with copies");
    }

}