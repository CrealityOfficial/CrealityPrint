#include "adaptivelayermessage.h"

#include "interface/selectorinterface.h"

namespace creative_kernel
{
    AdaptiveLayerMessage::AdaptiveLayerMessage(bool treeSupport ,QObject* parent)
        : MessageObject(parent)
        , m_treesupport(treeSupport)
    {
        
    }

    AdaptiveLayerMessage::~AdaptiveLayerMessage()
    {
    }

    int AdaptiveLayerMessage::codeImpl()
    {
        return MessageType::AdaptiveLayer;
    }

    int AdaptiveLayerMessage::levelImpl()
    {
        return 2;
    }

    QString AdaptiveLayerMessage::messageImpl()
    {
        if(m_treesupport)
            return QString("Error:Variable layer height is not supported withOrganic supports.");
        return QString("Error:The prime tower is only supported if all objectshave the same variable layer height");
    }

    QString AdaptiveLayerMessage::linkStringImpl()
    {
        return "";
        //return QString("Jump to [Prime Tower]");
    }

    void AdaptiveLayerMessage::handleImpl()
    {
    }
}