#include "modelcollisionmessage.h"
#include "data/modelgroup.h"
#include "interface/selectorinterface.h"
#include <QCoreApplication>

namespace creative_kernel
{
    ModelNCollisionMessage::ModelNCollisionMessage(QList<ModelGroup*> groups, QObject* parent) : MessageObject(parent)
    {
        m_groups = groups;
    }

    ModelNCollisionMessage::~ModelNCollisionMessage()
    {
    }

    int ModelNCollisionMessage::codeImpl()
    {
        return MessageType::ModelCollision;
    }

    int ModelNCollisionMessage::levelImpl()
    {
        return m_level;
    }

    QString ModelNCollisionMessage::messageImpl()
    {
        if (m_groups.isEmpty())
        {
            return QString();
        }
        return QString("[")+ m_groups.first()->groupName() + QString("] ") + tr("is too close to others; there may be collisions when printing.");
    }

    QString ModelNCollisionMessage::linkStringImpl()
    {
        if (m_groups.isEmpty())
        {
            return QString();
        }
        QString jumpTo = QCoreApplication::translate("creative_kernel::StayOnBorderMessage", "Jump to ");
        return jumpTo + QString("[")+ m_groups.first()->groupName() + QString("]");
    }

    void ModelNCollisionMessage::handleImpl()
    {
        if (m_groups.size() > 0)
        {
            selectGroup(m_groups.first(), false);
        }
    }


    void ModelNCollisionMessage::setLevel(int level)
    {
        m_level = level;
    }
}