#include "stayonbordermessage.h"
#include "data/modeln.h"
#include "interface/selectorinterface.h"

namespace creative_kernel
{
    StayOnBorderMessage::StayOnBorderMessage(QList<ModelN*> models, QObject* parent) : MessageObject(parent)
    {
        m_models = models;
    }

	StayOnBorderMessage::~StayOnBorderMessage()
    {

    }

    int StayOnBorderMessage::codeImpl()
    {
        return 0;
    }

    int StayOnBorderMessage::levelImpl()
    {
        return 2;
    }

    QString StayOnBorderMessage::messageImpl()
    {
        return tr("An object is laid over the boundary of the plate or exceeds the height limit. Please solve the problem by moving it totally on or off the plate, and confirming that the height is within the build volume.");
    }

    QString StayOnBorderMessage::linkStringImpl()
    {
        return tr("Jump to ");
    }

    void StayOnBorderMessage::handleImpl()
    {
        QList<qtuser_3d::Pickable*> pickables;
        for (auto m : m_models)
            pickables << m;

        unselectAll();
        selectMore(pickables);
    }

}