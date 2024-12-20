#include "pickbottomaction.h"
#include "kernel/kernelui.h"
#include "interface/spaceinterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"
#include "interface/selectorinterface.h"
#include "interface/layoutinterface.h"
#include "interface/printerinterface.h"
#include "internal/multi_printer/printer.h"
#include "data/modelgroup.h"
#include "data/modeln.h"

namespace creative_kernel
{
    PickBottomAction::PickBottomAction(QObject* parent):ActionCommand(parent)
    {
        m_actionname = tr("Pick Bottom");
        m_actionNameWithout = "Pick Bottom";
        m_source = "qrc:/UI/photo/menuImg/pick_bottom_n.svg";
        m_icon = "qrc:/UI/photo/menuImg/pick_bottom_p.svg";

        addUIVisualTracer(this,this);
    }

    PickBottomAction::~PickBottomAction()
    {
    }

    void PickBottomAction::execute()
    {
        Printer* p = getSelectedPrinter();
        QList<ModelGroup*> modelGroups = selectedGroups();
		if (modelGroups.empty())
		{
			creative_kernel::setModelGroupsMaxFaceBottom(p->modelGroupsInside());
		}
		else
		{
			creative_kernel::setModelGroupsMaxFaceBottom(modelGroups);
		}
    }


    void PickBottomAction::onThemeChanged(ThemeCategory category)
    {
    }

    void PickBottomAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Pick Bottom");
    }

    bool PickBottomAction::enabled()
    {
        Printer* printer = getSelectedPrinter();
        if (!printer)
            return false;

        auto insideModels = printer->modelsInside();
        auto onBorderModels = printer->modelsOnBorder();

        return insideModels.size() + onBorderModels.size() > 0;
    }
}
