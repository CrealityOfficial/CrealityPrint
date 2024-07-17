#include "modeltootallmessage.h"
#include "data/modeln.h"
#include "interface/selectorinterface.h"
#include <QCoreApplication>

namespace creative_kernel
{
    ModelTooTallMessage::ModelTooTallMessage(QList<ModelN*> models, QObject* parent) : MessageObject(parent)
    {
        m_models = models;
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
        if (m_models.isEmpty())
        {
            return QString();
        }
        return QString("[")+ m_models.first()->objectName() + QString("] ") + tr("is too tall, and collisions will be caused.");
    }

    QString ModelTooTallMessage::linkStringImpl()
    {
        if (m_models.isEmpty())
        {
            return QString();
        }
        QString jumpTo = QCoreApplication::translate("creative_kernel::StayOnBorderMessage", "Jump to ");
        return jumpTo + QString("[") + m_models.first()->objectName() + QString("]");
    }

    void ModelTooTallMessage::handleImpl()
    {
        if (m_models.isEmpty()) return;

        QList<qtuser_3d::Pickable*> pickables;
        pickables << m_models.first();
        /*for (auto m : m_models)
            pickables << m;*/

        unselectAll();
        selectMore(pickables);
    }
}