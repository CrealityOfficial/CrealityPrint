#include "layoutplateaction.h"
#include "interface/layoutinterface.h"
#include "interface/printerinterface.h"
#include "internal/multi_printer/printer.h"
#include "interface/modelinterface.h"
#include "interface/selectorinterface.h"
#include "interface/spaceinterface.h"
#include "interface/commandinterface.h"

using namespace qtuser_core;
namespace creative_kernel
{
    LayoutPlateAction::LayoutPlateAction(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("Auto arrange");
        m_actionNameWithout = "Auto arrange";
        m_source = "qrc:/UI/photo/menuImg/auto_layout_n.svg";
        m_icon = "qrc:/UI/photo/menuImg/auto_layout_p.svg";

        addUIVisualTracer(this,this);
    }

    LayoutPlateAction::~LayoutPlateAction()
    {
    }

    void LayoutPlateAction::execute()
    {
        Printer* printer = getSelectedPrinter();

        auto insideModels = printer->modelsInside();
        auto onBorderModels = printer->modelsOnBorder();
        
		layoutModels(insideModels + onBorderModels, printer->index(), false, false);
    }

    bool LayoutPlateAction::enabled()
    {
        Printer* printer = getSelectedPrinter();
        if (!printer)
            return false;

        auto insideModels = printer->modelsInside();
        auto onBorderModels = printer->modelsOnBorder();

        return insideModels.size() + onBorderModels.size() > 0;
    }

    void LayoutPlateAction::onThemeChanged(ThemeCategory category)
    {
    }

    void LayoutPlateAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Auto arrange");
    }
}