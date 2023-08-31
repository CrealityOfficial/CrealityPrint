#include "supportcommand.h"
#include "simplesupportop.h"

#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
using namespace creative_kernel;
SupportCommand::SupportCommand(QObject *parent) : ToolCommand(parent),m_pOp(nullptr)
{
    orderindex = 99;
}
void SupportCommand::addMode()
{
    if (m_pOp)
    {
        m_pOp->setAddMode();
    }
}
void SupportCommand::deleteMode()
{
    if (m_pOp)
    {
        m_pOp->setDeleteMode();
    }
}
void SupportCommand::execute()
{
    if (!m_pOp)
    {
        m_pOp = new SimpleSupportOp(this);
    }

    if (!m_pOp->getShowPop())
    {
        setVisOperationMode(m_pOp);
    }
    else
    {
        setVisOperationMode(nullptr);
    }

}
void SupportCommand::removeAll()
{
    if (m_pOp)
    {
        m_pOp->deleteAllSupport();
    }

}
void SupportCommand::autoAddSupport(qreal size, qreal angle, bool platform)
{
    qDebug() << "support autoAddSupport";

    if (m_pOp)
    {
        m_pOp->setSupportAngle(angle);
        m_pOp->setSupportSize(size);
        m_pOp->autoSupport(platform);
    }
}
bool SupportCommand::checkSelect()
{
    qDebug() << "support checkselect";

    QList<ModelN*> selections = selectionms();
    if(selections.size()>0)
    {
        return true;
    }
    return false;
}
void SupportCommand::changeSize(qreal size)
{
    if (m_pOp)
    {
        m_pOp->setSupportSize(size);
    }
}
