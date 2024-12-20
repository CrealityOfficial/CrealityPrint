#include "modeltootallmessage.h"
#include "data/modelgroup.h"
#include "interface/selectorinterface.h"
#include <QCoreApplication>

namespace creative_kernel
{
    ModelTooTallMessage::ModelTooTallMessage(QList<ModelGroup*> groups, QObject* parent) : MessageObject(parent)
    {
        m_groups = groups;
    }

    ModelTooTallMessage::~ModelTooTallMessage()
    {
    }

    int ModelTooTallMessage::codeImpl()
    {
        return MessageType::ModelTooTall;
    }

    int ModelTooTallMessage::levelImpl()
    {
        return m_level;
    }

    QString ModelTooTallMessage::messageImpl()
    {
        if (m_groups.isEmpty())
        {
            return QString();
        }
        return QString("[")+ m_groups.first()->groupName() + QString("] ") + tr("is too tall, and collisions will be caused.");
    }

    QString ModelTooTallMessage::linkStringImpl()
    {
        if (m_groups.isEmpty())
        {
            return QString();
        }
        QString jumpTo = QCoreApplication::translate("creative_kernel::StayOnBorderMessage", "Jump to ");
        return jumpTo + QString("[") + m_groups.first()->groupName() + QString("]");
    }

    void ModelTooTallMessage::handleImpl()
    {
        if (m_groups.isEmpty()) return;

        selectGroup(m_groups.first(), false);
    }
}