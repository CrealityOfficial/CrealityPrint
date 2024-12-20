#include "adaptivelayerplugin.h"
#include "kernel/translator.h"
#include "kernel/kernelui.h"
#include "interface/commandinterface.h"
#include "adaptivelayercommand.h"
#include "adaptivelayermode.h"
#include <QtCore/QCoreApplication>

using namespace creative_kernel;
AdaptiveLayerPlugin::AdaptiveLayerPlugin(QObject* parent) : QObject(parent), m_command(nullptr)
{

}

AdaptiveLayerPlugin::~AdaptiveLayerPlugin()
{

}

QString AdaptiveLayerPlugin::name()
{
	return "AdaptiveLayerPlugin";
}

QString AdaptiveLayerPlugin::info()
{
	return "Color the surface of the model.";
}

void AdaptiveLayerPlugin::initialize()
{
	m_command = new AdaptiveLayerCommand(this);
	m_command->setName(tr("Color") + ": N");

	m_command->setSource("qrc:/adaptive_layer/adaptive_layer/AdaptivePanel.qml");
	m_command->orderindex = 2;
    getKernelUI()->addToolCommand(m_command,
    qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_DRAW,
    qtuser_qml::ToolCommandType::ADAPTIVE_LAYER);
	addUIVisualTracer(this,this);
}

void AdaptiveLayerPlugin::uninitialize()
{
  getKernelUI()->removeToolCommand(m_command, qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_DRAW);
}

void AdaptiveLayerPlugin::onThemeChanged(ThemeCategory category)
{
	m_command->setDisabledIcon("qrc:/UI/photo/cToolBar/adaptive_dark_disable.svg");
	m_command->setEnabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/cToolBar/adaptive_dark_default.svg" : "qrc:/UI/photo/cToolBar/adaptive_light_default.svg");
	m_command->setHoveredIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/cToolBar/adaptive_dark_default.svg" : "qrc:/UI/photo/cToolBar/adaptive_light_default.svg");
	m_command->setPressedIcon("qrc:/UI/photo/cToolBar/adaptive_dark_press.svg");
}

void AdaptiveLayerPlugin::onLanguageChanged(creative_kernel::MultiLanguage language)
{
	m_command->setName(QCoreApplication::translate("CusAdaptiveLayerTs", "Adaptive"));
}
