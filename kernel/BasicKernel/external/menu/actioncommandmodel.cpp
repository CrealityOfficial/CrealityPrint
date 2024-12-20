#include "menu/actioncommandmodel.h"
#include "menu/actioncommand.h"
#include <QList>
#include <QVariant>
#include <QDebug>

enum
{
    maction_name = Qt::UserRole + 1,
    maction_shortcut,
    maction_parentmenu,
    maction_source,
    maction_icon,
    maction_item,
    maction_enable,
    maction_separator,
    maction_checked
};

ActionCommandModel::ActionCommandModel(QObject* parent)
    :QAbstractListModel(parent)
{
    m_rolesName[maction_name] = "actionNameRole";
    m_rolesName[maction_shortcut] = "actionShortcut";
    m_rolesName[maction_parentmenu] = "parentMenu";
    m_rolesName[maction_source] = "actionSource";
    m_rolesName[maction_icon] = "actionIcon";
    m_rolesName[maction_item] = "actionItem";
    m_rolesName[maction_enable] = "actionEnable";
    m_rolesName[maction_separator] = "actionSeparator";
    m_rolesName[maction_checked] = "actionChecked";
}

ActionCommandModel::~ActionCommandModel()
{

}

void ActionCommandModel::addCommand(ActionCommand* command)
{
    if (command)
    {
        
        int index = m_actionCommands.size();
        beginInsertRows(QModelIndex(), index, index);
        m_actionCommands.push_back(command);
        endInsertRows();
    }
}

//Add in reverse order, insert first
void ActionCommandModel::addCommandFront(ActionCommand* command)
{
    if (command)
    {
        //        qDebug() << "addCommandFront";
        m_actionCommands.push_front(command);
        int index = 0;//m_actionCommands.size()-1;

        beginInsertRows(QModelIndex(), index, index);
        endInsertRows();
    }
}

void ActionCommandModel::removeCommandFromQString(QString strName)
{
    int index=-1;
     for(int i =0; i < m_actionCommands.size(); i++)
     {
        if(strName == m_actionCommands[i]->name())
        {
            index = i;
            break;
        }
     }
     if (index >= 0 && index < m_actionCommands.size())
     {
         beginRemoveRows(QModelIndex(), index, index);
         m_actionCommands.removeAt(index);
         endRemoveRows();
     }
}

void ActionCommandModel::removeCommand(ActionCommand* command)
{
    int index = m_actionCommands.indexOf(command);
    if (index >= 0 && index < m_actionCommands.size())
    {
        beginRemoveRows(QModelIndex(), index, index);
        m_actionCommands.removeAt(index);
        endRemoveRows();
        qDebug() << "remove command " << index << " command";
    }
}

void ActionCommandModel::removeAllCommand()
{
    for (int index = m_actionCommands.size() -1; index >= 0;index--)
    {
        beginRemoveRows(QModelIndex(), index, index);
        m_actionCommands.at(index)->deleteLater();
        m_actionCommands.removeAt(index);
        endRemoveRows();
    }
}

void ActionCommandModel::removeCommandButLastIndex()
{
    for (int index = m_actionCommands.size() -2; index >= 0;index--)
    {
        beginRemoveRows(QModelIndex(), index, index);
        m_actionCommands.removeAt(index);
        endRemoveRows();
    }
}

int ActionCommandModel::rowCount(const QModelIndex& parent) const
{
    return m_actionCommands.size();
}

int ActionCommandModel::columnCount(const QModelIndex& parent) const
{
    return 1;
}

QVariant ActionCommandModel::data(const QModelIndex& index, int role) const
{
    if (index.row() >= 0 && index.row() < m_actionCommands.size())
    {
        ActionCommand* command = m_actionCommands.at(index.row());
        switch (role)
        {
        case maction_name:
            return command->name();
        case maction_shortcut:
            return command->shortcut();
        case maction_parentmenu:
            return command->parentMenu();
        case maction_source:
            return command->source();
        case  maction_icon:
            return command->icon();
        case maction_item:
            return QVariant::fromValue(command);
        case maction_enable:
            return command->enabled();
        case maction_separator:
            return command->bSeparator();
        case maction_checked:
            return command->bChecked();
        default:
            break;
        }
    }
    return QVariant(0);
}

void ActionCommandModel::changeCommand(ActionCommand* command)
{
    for (int i = 0; i < m_actionCommands.size(); ++i) {
          if (m_actionCommands.at(i) == command)
          {
              //m_actionCommands[i] = command;
              emit dataChanged(createIndex(i, 0), createIndex(i, 0));
              qDebug() << "change command " << i;
          }
      }
}

bool ActionCommandModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    return false;
}
ActionCommand* ActionCommandModel::getData(int index)
{
    return m_actionCommands.at(index);
}

QList<ActionCommand*> ActionCommandModel::actionCommands()
{
    return m_actionCommands;
}

QHash<int, QByteArray> ActionCommandModel::roleNames() const
{
    return m_rolesName;
}

