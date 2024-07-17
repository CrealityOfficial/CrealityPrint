#include "modelcollisionmessage.h"
#include "data/modeln.h"
#include "interface/selectorinterface.h"
#include <QCoreApplication>

namespace creative_kernel
{
    ModelNCollisionMessage::ModelNCollisionMessage(QList<ModelN*> models, QObject* parent) : MessageObject(parent)
    {
        m_models = models;
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
        if (m_models.isEmpty())
        {
            return QString();
        }
        return QString("[")+ m_models.first()->objectName() + QString("] ") + tr("is too close to others; there may be collisions when printing.");
    }

    QString ModelNCollisionMessage::linkStringImpl()
    {
        if (m_models.isEmpty())
        {
            return QString();
        }
        QString jumpTo = QCoreApplication::translate("creative_kernel::StayOnBorderMessage", "Jump to ");
        return jumpTo + QString("[")+ m_models.first()->objectName() + QString("]");
    }

    void ModelNCollisionMessage::handleImpl()
    {
        if (m_models.isEmpty()) return;

        QList<qtuser_3d::Pickable*> pickables;
        pickables << m_models.first();
        /*for (auto m : m_models)
            pickables << m;*/

        unselectAll();
        selectMore(pickables);
    }


    void ModelNCollisionMessage::setLevel(int level)
    {
        m_level = level;
    }
}