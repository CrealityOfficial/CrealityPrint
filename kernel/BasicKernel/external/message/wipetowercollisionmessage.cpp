#include "wipetowercollisionmessage.h"

#include "interface/selectorinterface.h"
#include "internal/render/wipetowerentity.h"

namespace creative_kernel
{
    WipeTowerCollisionMessage::WipeTowerCollisionMessage(WipeTowerEntity* wt, QObject* parent) 
        : MessageObject(parent)
        , m_wipeTower(wt)
    {
        
    }

    WipeTowerCollisionMessage::~WipeTowerCollisionMessage()
    {
    }

    int WipeTowerCollisionMessage::codeImpl()
    {
        return MessageType::WipeTowerCollision;
    }

    int WipeTowerCollisionMessage::levelImpl()
    {
        return 1;
    }

    QString WipeTowerCollisionMessage::messageImpl()
    {
        return tr("Prime Tower is too close to others, add collisions may be caused.");
    }

    QString WipeTowerCollisionMessage::linkStringImpl()
    {
        return "";
        //return QString("Jump to [Prime Tower]");
    }

    void WipeTowerCollisionMessage::handleImpl()
    {
    }
}