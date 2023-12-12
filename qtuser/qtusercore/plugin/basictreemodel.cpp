#include "qtusercore/plugin/basictreemodel.h"
#include "qtusercore/plugin/basictreecommand.h"

enum
{
    maction_name = Qt::UserRole + 1,
    maction_level,
    maction_subnode
};

BasicTreeModel::BasicTreeModel(QObject* parent)
    :QAbstractListModel(parent)
{
    m_rolesName[maction_name] = "name";
    m_rolesName[maction_level] = "level";
    m_rolesName[maction_subnode] = "subnodes";
}

BasicTreeModel::~BasicTreeModel()
{
}

void BasicTreeModel::addCommand(BasicTreeCommand* command)
{
    if (command)
    {
        m_actionCommands.push_back(command);
        int index = m_actionCommands.size()-1;
        beginInsertRows(QModelIndex(), index, index);
        endInsertRows();
    }
}
void BasicTreeModel::removeCommand(BasicTreeCommand* command)
{
    int index = m_actionCommands.indexOf(command);
    if (index >= 0 && index < m_actionCommands.size())
    {
        beginRemoveRows(QModelIndex(), index, index);
        m_actionCommands.removeAt(index);
        endRemoveRows();
    }
}

void BasicTreeModel::setSubLevel()
{
    for (int i =0;i < m_actionCommands.size(); i ++)
    {
        int level = m_actionCommands[i]->level();
        level += 1;
        m_actionCommands[i]->setLevel(level);
    }
}

void BasicTreeModel::removeAllCommand()
{
    for (int index = m_actionCommands.size() -2; index >= 0;index--)
    {
        beginRemoveRows(QModelIndex(), index, index);
        m_actionCommands.removeAt(index);
        endRemoveRows();
    }

}
int BasicTreeModel::rowCount(const QModelIndex& parent) const
{
    return m_actionCommands.size();
}

int BasicTreeModel::columnCount(const QModelIndex& parent) const
{
    return 1;
}
QVariant BasicTreeModel::data(const QModelIndex& index, int role) const
{
    if (index.row() >= 0 && index.row() < m_actionCommands.size())
    {
        BasicTreeCommand* command = m_actionCommands.at(index.row());
        switch (role)
        {
        case maction_name:
            return command->name();
        case maction_level:
            return command->level();
        case maction_subnode:
            return QVariant::fromValue(command->subNode());
        default:
            break;
        }
    }
    return QVariant(0);
}
bool BasicTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    return false;
}

QHash<int, QByteArray> BasicTreeModel::roleNames() const
{
    return m_rolesName;
}

