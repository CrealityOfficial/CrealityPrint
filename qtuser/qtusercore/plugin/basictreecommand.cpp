#include "qtusercore/plugin/basictreemodel.h"
#include "qtusercore/plugin/basictreecommand.h"

BasicTreeCommand::BasicTreeCommand()
{
    m_name = "";
    m_level = 0;
    m_subModel =  nullptr;
}
BasicTreeCommand::~BasicTreeCommand()
{

}
void BasicTreeCommand::setName(const QString& name)
{
    m_name = name;
}
QString BasicTreeCommand::name()const
{
    return m_name;
}

void BasicTreeCommand::setLevel(const int &level)
{

    m_level = level;
}
void BasicTreeCommand::setSuNode(BasicTreeModel *subnode)
{
    m_subModel = subnode;
    m_subModel->setSubLevel();
}
int BasicTreeCommand::level() const
{
    return m_level;
}
BasicTreeModel *BasicTreeCommand:: subNode()const
{
    return m_subModel;
}

void BasicTreeCommand:: execute()
{}
