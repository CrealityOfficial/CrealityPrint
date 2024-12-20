#include "splitobjectsaction.h"

#include "interface/selectorinterface.h"
#include "interface/modelgroupinterface.h"
#include "interface/commandinterface.h"

namespace creative_kernel
{
    SplitObjectsAction::SplitObjectsAction(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Split To Objects");
        m_actionNameWithout = "Split To Objects";
        m_source = "qrc:/UI/photo/menuImg/split_to_objects_n.svg";
        m_icon = "qrc:/UI/photo/menuImg/split_to_objects_p.svg";

        addUIVisualTracer(this,this);
    }

    SplitObjectsAction::~SplitObjectsAction()
    {
    }

    void SplitObjectsAction::execute()
    {
        splitSelectionsGroup2Objects();
    }

    bool SplitObjectsAction::enabled()
    {
        return selectedGroups().size() == 1;
    }

    void SplitObjectsAction::onThemeChanged(ThemeCategory category)
    {
    }

    void SplitObjectsAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Split To Objects");
    }
}
