#include "menu/actioncommand.h"

ActionCommand::ActionCommand(QObject* parent)
    :QObject(parent)
    , m_enabled(true)
    ,m_bVisible(true)
    ,m_bChecked(false)
{
    m_actionname = "";
    m_shortcut ="";
    m_source = "";
    m_icon = "";
    m_eParentMenu = eMenuType_File;
    m_bSubMenu = false;
    m_bSeparator = true;
    m_bCheckable = false;
    m_insertKey = "";
}

ActionCommand::~ActionCommand()
{

}

bool ActionCommand::enabled()
{
    return m_enabled;
}
void ActionCommand::setEnabled(bool enabled)
{
    if(m_enabled != enabled)
    {
        m_enabled = enabled;
        emit enabledChanged();
    }
}
void ActionCommand::update()
{
    emit enabledChanged();
}

QString ActionCommand::source() const
{
    return m_source;
}

QString ActionCommand::name() const
{
    return m_actionname;
}

QString ActionCommand::nameWithout() const
{
    return m_actionNameWithout;
}
QString ActionCommand::shortcut() const
{
    return m_shortcut;
}
eMenuType ActionCommand::parentMenu() const
{
    return m_eParentMenu;
}
QString ActionCommand::icon() const
{
    return m_icon;
}

bool ActionCommand::bSubMenu() const
{
    return m_bSubMenu;
}
void ActionCommand::setBSubMenu(const bool &subMenu)
{
    m_bSubMenu = subMenu;

}
void ActionCommand::setName(const QString& name)
{
    m_actionname = name;
}

void ActionCommand::setNameWithout(const QString& name)
{
    m_actionNameWithout = name;
}

void ActionCommand::setSource(const QString& source)
{
    m_source = source;
}
void ActionCommand::setShortcut(const QString &shortcut)
{
    m_shortcut = shortcut;
}
void ActionCommand::setParentMenu(const eMenuType &parentMenu)
{
    m_eParentMenu =  parentMenu;
}
void ActionCommand::setIcon(const QString& icon)
{
    m_icon = icon;
}
void ActionCommand::execute()
{

}
bool ActionCommand::bSeparator() const
{
    return m_bSeparator;
}

void ActionCommand::setBSeparator(const bool &bSeparator)
{
    m_bSeparator = bSeparator;
}
