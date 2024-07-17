#include "colorplugin.h"
#include "kernel/translator.h"
#include "kernel/kernelui.h"
#include "interface/commandinterface.h"
#include "operation/paintoperatemode/paintcommand.h"
#include "coloroperatemode.h"

using namespace creative_kernel;
ColorPlugin::ColorPlugin(QObject* parent) : QObject(parent), m_command(nullptr)
{

}

ColorPlugin::~ColorPlugin()
{

}

QString ColorPlugin::name()
{
	return "ColorPlugin";
}

QString ColorPlugin::info()
{
	return "Color the surface of the model.";
}

void ColorPlugin::initialize()
{
	m_command = new PaintCommand(this);
	m_command->setName(tr("Color") + ": N");
	m_command->setOperateMode(new ColorOperateMode(m_command));

	m_command->setSource("qrc:/paint/paint/ColorPanel.qml");
	m_command->orderindex = 3;
  getKernelUI()->addToolCommand(m_command,
    qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_DRAW,
    qtuser_qml::ToolCommandType::COLOR);
	addUIVisualTracer(this,this);
}

void ColorPlugin::uninitialize()
{
  getKernelUI()->removeToolCommand(m_command, qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_DRAW);
}

void ColorPlugin::onThemeChanged(ThemeCategory category)
{
	m_command->setDisabledIcon("qrc:/UI/photo/cToolBar/color_dark_disable.svg");
	m_command->setEnabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/cToolBar/color_dark_default.svg" : "qrc:/UI/photo/cToolBar/color_light_default.svg");
	m_command->setHoveredIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/cToolBar/color_dark_default.svg" : "qrc:/UI/photo/cToolBar/color_light_default.svg");
	m_command->setPressedIcon("qrc:/UI/photo/cToolBar/color_dark_press.svg");
	
}

void ColorPlugin::onLanguageChanged(creative_kernel::MultiLanguage language)
{
	m_command->setName(tr("Color"));
}
