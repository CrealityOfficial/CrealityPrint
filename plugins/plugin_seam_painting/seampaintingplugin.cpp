#include "seampaintingplugin.h"
#include "kernel/translator.h"
#include "kernel/kernelui.h"
#include "interface/commandinterface.h"
#include "seampaintingoperatemode.h"
#include "operation/paintoperatemode/paintcommand.h"

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
	m_command = new PaintCommand(this);
	m_command->setName(tr("Seam Painting") + ": N");
	m_command->setSource("qrc:/paint/paint/SeamPaintingPanel.qml");
	m_command->setOperateMode(new SeamPaintingOperateMode(m_command));
	m_command->orderindex = 1;
  getKernelUI()->addToolCommand(m_command,
    qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_DRAW,
    qtuser_qml::ToolCommandType::SEAM_PAINTING);
	addUIVisualTracer(this,this);
}

void SeamPaintingPlugin::uninitialize()
{
  getKernelUI()->removeToolCommand(m_command, qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_DRAW);
}

void SeamPaintingPlugin::onThemeChanged(ThemeCategory category)
{
	m_command->setDisabledIcon("qrc:/UI/photo/cToolBar/zDraw_dark_disable.svg");
	m_command->setEnabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/cToolBar/zDraw_dark_default.svg" : "qrc:/UI/photo/cToolBar/zDraw_light_default.svg");
	m_command->setHoveredIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/cToolBar/zDraw_dark_default.svg" : "qrc:/UI/photo/cToolBar/zDraw_light_default.svg");
	m_command->setPressedIcon("qrc:/UI/photo/cToolBar/zDraw_dark_press.svg");
}

void SeamPaintingPlugin::onLanguageChanged(creative_kernel::MultiLanguage language)
{
	m_command->setName(tr("Seam Painting"));
}
