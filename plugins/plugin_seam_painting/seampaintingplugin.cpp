#include "seampaintingplugin.h"
#include "seampaintingcommand.h"
#include "kernel/translator.h"
#include "kernel/kernelui.h"
#include "interface/commandinterface.h"

using namespace creative_kernel;
SeamPaintingPlugin::SeamPaintingPlugin(QObject* parent) : QObject(parent), m_command(nullptr)
{

}

SeamPaintingPlugin::~SeamPaintingPlugin()
{

}

QString SeamPaintingPlugin::name()
{
	return "SeamPaintingPlugin";
}

QString SeamPaintingPlugin::info()
{
	return "Painting the seam of the model.";
}

void SeamPaintingPlugin::initialize()
{
	m_command = new SeamPaintingCommand(this);
	m_command->setName(tr("Seam Painting") + ": N");

	m_command->setSource("qrc:/paint/paint/SeamPaintingPanel.qml");

  getKernelUI()->addToolCommand(m_command,
    qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_OTHER,
    qtuser_qml::ToolCommandType::SEAM_PAINTING);
	addUIVisualTracer(this);
}

void SeamPaintingPlugin::uninitialize()
{
  getKernelUI()->removeToolCommand(m_command, qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_MAIN);
}

void SeamPaintingPlugin::onThemeChanged(ThemeCategory category)
{
	m_command->setDisabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/seam_painting_n.svg" : "qrc:/UI/photo/leftBar/seam_painting_light_n.svg");
	m_command->setEnabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/seam_painting_n.svg" : "qrc:/UI/photo/leftBar/seam_painting_light_n.svg");
	m_command->setHoveredIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/seam_painting_p.svg" : "qrc:/UI/photo/leftBar/seam_painting_p.svg");
	m_command->setPressedIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/seam_painting_p.svg" : "qrc:/UI/photo/leftBar/seam_painting_p.svg");
}

void SeamPaintingPlugin::onLanguageChanged(creative_kernel::MultiLanguage language)
{
	m_command->setName(tr("Seam Painting"));
}
