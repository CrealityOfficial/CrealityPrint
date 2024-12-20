#include "stayonbordermessage.h"
#include "data/modelgroup.h"
#include "interface/selectorinterface.h"

namespace creative_kernel
{
    StayOnBorderMessage::StayOnBorderMessage(QList<ModelGroup*> groups, QObject* parent) : MessageObject(parent)
    {
        m_groups = groups;
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
        if (m_groups.size() > 0)
        {
            selectGroup(m_groups.first(), false);
        }
    }

}