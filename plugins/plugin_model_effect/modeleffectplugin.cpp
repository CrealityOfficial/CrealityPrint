#include "modeleffectplugin.h"
#include "modeleffectcommand.h"

#include "kernel/kernelui.h"

using namespace creative_kernel;
ModelEffectPlugin::ModelEffectPlugin(QObject* parent)
	:QObject(parent)
	, m_command(nullptr)
{
}

ModelEffectPlugin::~ModelEffectPlugin()
{
}

QString ModelEffectPlugin::name()
{
	return "";
}

QString ModelEffectPlugin::info()
{
	return "";
}

void ModelEffectPlugin::initialize()
{
	if (!m_command)
		m_command = new ModelEffectCommand(this);
		/*getKernelUI()->addToolCommand(m_command,
			qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_MAIN,
			qtuser_qml::ToolCommandType::MODEL_EFFECT);*/
}

void ModelEffectPlugin::uninitialize()
{
  getKernelUI()->removeToolCommand(m_command, qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_MAIN);
}
