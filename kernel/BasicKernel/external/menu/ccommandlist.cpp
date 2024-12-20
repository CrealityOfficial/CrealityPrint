#include "ccommandlist.h"
#include "menu/actioncommand.h"

#include <QDebug>

CCommandList *CCommandList:: m_pComInstance = nullptr;
CCommandList *CCommandList:: getInstance()
{
    if (m_pComInstance == nullptr)
    {
        m_pComInstance = new CCommandList();
    }
    return m_pComInstance;
}

CCommandList::CCommandList(QObject *parent) : QObject(parent)
{
    m_varCommandList.clear();
    if(m_varCommandList.isEmpty())
    {
        startAddCommads();
    }
}
void CCommandList::addActionCommad(ActionCommand *pAction)
{
    if (pAction)
    {
        m_varCommandList.push_back(pAction);
    }
}

void CCommandList::addActionCommad(ActionCommand *pAction,int index)
{
    if(pAction == nullptr)return ;
    if(index > m_varCommandList.size())return;

    m_varCommandList.insert(index,pAction);
}
//
void CCommandList::addActionCommad(ActionCommand *pAction, QString strInsetKey)
{
    if(pAction == nullptr || m_varCommandList.isEmpty() )return ;
    int index = indexOfString(strInsetKey);
    m_varCommandList.insert(index+1,pAction);
}

int CCommandList::indexOfString(QString strInfo)
{
    int index = 0;
    if(m_varCommandList.isEmpty())return index;
    for(int i = 0 ; i < m_varCommandList.size(); i++)
    {
        if(m_varCommandList[i]->insertKey() == strInfo)
        {
            index = i;
            break;
        }
    }
    return  index;
}

void CCommandList::addActionCommads(QList< ActionCommand *>listAction)
{
    if(listAction.isEmpty())return;
    foreach (auto var, listAction)
    {
        m_varCommandList.push_back(var);
    }

}

void CCommandList::removeActionCommand(ActionCommand* command)
{
    int index = m_varCommandList.indexOf(command);
    if (index >= 0 && index < m_varCommandList.size())
    {
        m_varCommandList.removeAt(index);
    }
}

QList<ActionCommand *>CCommandList::getActionCommandList()
{
    return m_varCommandList;
}

void CCommandList::startAddCommads()
{
    
}
