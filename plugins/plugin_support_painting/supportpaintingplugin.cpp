#include "supportpaintingplugin.h"
#include "supportpaintingcommand.h"
#include "kernel/translator.h"
#include "kernel/kernelui.h"
#include "interface/commandinterface.h"

using namespace creative_kernel;
SupportPaintingPlugin::SupportPaintingPlugin(QObject* parent) : QObject(parent), m_command(nullptr)
{

}

SupportPaintingPlugin::~SupportPaintingPlugin()
{

}

QString SupportPaintingPlugin::name()
{
	return "SupportPaintingPlugin";
}

QString SupportPaintingPlugin::info()
{
	return "Painting the support of the model.";
}

void SupportPaintingPlugin::initialize()
{
	m_command = new SupportPaintingCommand(this);
	m_command->setName(tr("Support Painting") + ": N");

	m_command->setSource("qrc:/paint/paint/SupportPaintingPanel.qml");

  getKernelUI()->addToolCommand(m_command,
    qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_MAIN,
    qtuser_qml::ToolCommandType::SUPPORT_PAINTING);
	addUIVisualTracer(this);
}

void SupportPaintingPlugin::uninitialize()
{
  getKernelUI()->removeToolCommand(m_command, qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_MAIN);
}

void SupportPaintingPlugin::onThemeChanged(ThemeCategory category)
{
	m_command->setDisabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/support_dark.svg" : "qrc:/UI/photo/leftBar/support_lite.svg");
	m_command->setEnabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/support_dark.svg" : "qrc:/UI/photo/leftBar/support_lite.svg");
	m_command->setHoveredIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/support_pressed.svg" : "qrc:/UI/photo/leftBar/support_lite.svg");
	m_command->setPressedIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/support_pressed.svg" : "qrc:/UI/photo/leftBar/support_pressed.svg");
}

void SupportPaintingPlugin::onLanguageChanged(creative_kernel::MultiLanguage language)
{
	m_command->setName(tr("Support Painting"));
}
