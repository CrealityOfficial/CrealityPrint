#include "supportpaintingplugin.h"
#include "kernel/translator.h"
#include "kernel/kernelui.h"
#include "interface/commandinterface.h"
#include "operation/paintoperatemode/paintcommand.h"
#include "supportpaintingoperatemode.h"

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
	m_command = new PaintCommand(this);
	m_command->setName(tr("Support Painting") + ": N");
	m_command->setSource("qrc:/paint/paint/SupportPaintingPanel.qml");
	m_command->setOperateMode(new SupportPaintingOperateMode(m_command));
	m_command->orderindex = 0;
  getKernelUI()->addToolCommand(m_command,
    qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_DRAW,
    qtuser_qml::ToolCommandType::SUPPORT_PAINTING);
	addUIVisualTracer(this,this);
}

void SupportPaintingPlugin::uninitialize()
{
  getKernelUI()->removeToolCommand(m_command, qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_DRAW);
}

void SupportPaintingPlugin::onThemeChanged(ThemeCategory category)
{
	m_command->setDisabledIcon("qrc:/UI/photo/cToolBar/support_dark_disable.svg");
	m_command->setEnabledIcon(category == ThemeCategory::tc_dark ?"qrc:/UI/photo/cToolBar/support_dark_default.svg" : "qrc:/UI/photo/cToolBar/support_light_default.svg");
	m_command->setHoveredIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/cToolBar/support_dark_default.svg" : "qrc:/UI/photo/cToolBar/support_light_default.svg");
	m_command->setPressedIcon("qrc:/UI/photo/cToolBar/support_dark_press.svg");
}

void SupportPaintingPlugin::onLanguageChanged(creative_kernel::MultiLanguage language)
{
	m_command->setName(tr("Support Painting"));
}
