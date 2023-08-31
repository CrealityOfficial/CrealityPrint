#ifndef _NULLSPACE_LETTER_PLUGIN_1591235079966_H
#define _NULLSPACE_LETTER_PLUGIN_1591235079966_H
#include "qtusercore/module/creativeinterface.h"
#include "data/interface.h"

class LetterCommand;
class LetterPlugin : public QObject, public CreativeInterface
	, public creative_kernel::UIVisualTracer
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "creative.LetterPlugin")
	Q_INTERFACES(CreativeInterface)

public:
	LetterPlugin(QObject* parent = nullptr);
	virtual ~LetterPlugin();

protected:
	QString name() override;
	QString info() override;

	void initialize() override;
	void uninitialize() override;
protected:
	void onThemeChanged(creative_kernel::ThemeCategory category) override;
	void onLanguageChanged(creative_kernel::MultiLanguage language) override;
protected:
	LetterCommand* m_command;
};

#endif // _NULLSPACE_LETTER_PLUGIN_1591235079966_H