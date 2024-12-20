#include "higherthanbottommessage.h"
#include "data/modelgroup.h"
#include "interface/selectorinterface.h"
#include "interface/spaceinterface.h"

namespace creative_kernel
{
    HigherThanBottomMessage::HigherThanBottomMessage(int groupId, QObject* parent) 
        : MessageObject(parent)
    {
        m_groupId = groupId;
    }

	HigherThanBottomMessage::~HigherThanBottomMessage()
    {

    }

    int HigherThanBottomMessage::codeImpl()
    {
        return MessageType::ModelGroupHigherThanBottom;
    }

    int HigherThanBottomMessage::levelImpl()
    {
        return int(MessageLevel::Warning);  //warning
    }

    QString HigherThanBottomMessage::messageImpl()
    {
        return tr("There is an object present within the plate that is floating (not bottomed). Please add support or perform bottoming.");
    }

    QString HigherThanBottomMessage::linkStringImpl()
    {
        QString groupName = "";
        ModelGroup* mg = getModelGroupByObjectId(m_groupId);
        if (mg)
        {
            groupName = mg->groupName();
        }

        return tr("Jump to ") + "[" + groupName + QString("]");
    }

    void HigherThanBottomMessage::handleImpl()
    {
        ModelGroup* mg = getModelGroupByObjectId(m_groupId);
        if (mg)
        {
            selectGroup(mg);
        }
    }

}