#include "letterplugin.h"
#include "lettercommand.h"
#include "kernel/translator.h"
#include "kernel/kernelui.h"
#include "interface/commandinterface.h"

using namespace creative_kernel;
LetterPlugin::LetterPlugin(QObject* parent) : QObject(parent), m_command(nullptr)
{

}

LetterPlugin::~LetterPlugin()
{

}

QString LetterPlugin::name()
{
	return "LetterPlugin";
}

QString LetterPlugin::info()
{
	return "Lettering texts on the model surface.";
}

void LetterPlugin::initialize()
{
	m_command = new LetterCommand(this);
	m_command->setName(tr("Letter") + ": T");

	m_command->setSource("qrc:/letter/letter/info.qml");

  getKernelUI()->addToolCommand(m_command,
    qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_OTHER,
    qtuser_qml::ToolCommandType::LETTER);
	addUIVisualTracer(this);
}

void LetterPlugin::uninitialize()
{
  getKernelUI()->removeToolCommand(m_command, qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_MAIN);
}

void LetterPlugin::onThemeChanged(ThemeCategory category)
{
	m_command->setDisabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/letter_dark.svg" : "qrc:/UI/photo/leftBar/letter_lite.svg");
	m_command->setEnabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/letter_dark.svg" : "qrc:/UI/photo/leftBar/letter_lite.svg");
	m_command->setHoveredIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/letter_pressed.svg" : "qrc:/UI/photo/leftBar/letter_lite.svg");
	m_command->setPressedIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/letter_pressed.svg" : "qrc:/UI/photo/leftBar/letter_pressed.svg");
}

void LetterPlugin::onLanguageChanged(creative_kernel::MultiLanguage language)
{
	m_command->setName(tr("Letter"));
}
