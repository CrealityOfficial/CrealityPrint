#include "splitpartsaction.h"

#include "interface/selectorinterface.h"
#include "interface/modelgroupinterface.h"
#include "interface/commandinterface.h"

namespace creative_kernel
{
    SplitPartsAction::SplitPartsAction(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Split To Parts");
        m_actionNameWithout = "Split To Parts";
        m_source = "qrc:/UI/photo/menuImg/split_to_parts_n.svg";
        m_icon = "qrc:/UI/photo/menuImg/split_to_parts_p.svg";

        addUIVisualTracer(this,this);
    }

    SplitPartsAction::~SplitPartsAction()
    {
    }

    void SplitPartsAction::execute()
    {
        splitSelectionsModels2Parts();
    }

    bool SplitPartsAction::enabled()
    {
        return selectedGroups().size() == 1 && selectionms().size() == 1;
    }

    void SplitPartsAction::onThemeChanged(ThemeCategory category)
    {
    }

    void SplitPartsAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Split To Parts");
    }
}
