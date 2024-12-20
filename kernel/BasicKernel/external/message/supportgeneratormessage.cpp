#include "supportgeneratormessage.h"
#include "data/modeln.h"
#include "data/modelgroup.h"
#include "interface/selectorinterface.h"

namespace creative_kernel
{
    SupportGeneratorMessage::SupportGeneratorMessage(ModelN* supportGenerator, QObject* parent) : MessageObject(parent)
    {
        m_supportGenerator = supportGenerator;
    }

	SupportGeneratorMessage::~SupportGeneratorMessage()
    {

    }

    int SupportGeneratorMessage::codeImpl()
    {
        return SupportGeneratorWarn;
    }

    int SupportGeneratorMessage::levelImpl()
    {
        return Warning;
    }

    QString SupportGeneratorMessage::messageImpl()
    {
        return tr("Support generators are used but support is not enabled. Please enable support.");
    }

    QString SupportGeneratorMessage::linkStringImpl()
    {
        ModelGroup* group = m_supportGenerator->getModelGroup();
        if (group)
            return tr("Jump to") + QString(" [%1]").arg(group->groupName());
        else 
            return tr("Jump to") + QString(" [%1]").arg(m_supportGenerator->name());
    }

    void SupportGeneratorMessage::handleImpl()
    {
        if (!m_supportGenerator)
            return;

        ModelGroup* group = m_supportGenerator->getModelGroup();
        if (group)
        {
            selectGroup(group, false);
        }
        else
        {
            selectModelN(m_supportGenerator, false);
        }
    }

}