#include "repair.h"
#include "repaircommand.h"

#include "menu/ccommandlist.h"

using namespace creative_kernel;
Repair::Repair(QObject* parent)
    :QObject(parent)
    , m_command(nullptr)
{
}

Repair::~Repair()
{
}

QString Repair::name()
{
    return "repair";
}

QString Repair::info()
{
    return "repair model";
}

void Repair::initialize()
{
    m_command = new RepairCommand();
    m_command->setParent(this);
    CCommandList::getInstance()->addActionCommad(m_command, "perfer");
}

void Repair::uninitialize()
{
    CCommandList::getInstance()->removeActionCommand(m_command);
}
