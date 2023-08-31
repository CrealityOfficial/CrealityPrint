#include "splitplugin.h"
#include "splitcommand.h"
#include "kernel/kernelui.h"

using namespace creative_kernel;
SplitPlugin::SplitPlugin(QObject* parent)
	:QObject(parent)
	, m_command(nullptr)
{
}

SplitPlugin::~SplitPlugin()
{
}

QString SplitPlugin::name()
{
	return "";
}

QString SplitPlugin::info()
{
	return "";
}

void SplitPlugin::initialize()
{
	if (!m_command)
		m_command = new SplitCommand(this);

		getKernelUI()->addToolCommand(m_command,
			qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_OTHER,
			qtuser_qml::ToolCommandType::SPLIT);
}

void SplitPlugin::uninitialize()
{
  getKernelUI()->removeToolCommand(m_command, qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_MAIN);
}
