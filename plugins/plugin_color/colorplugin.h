#ifndef _NULLSPACE_COLOR_PLUGIN_1591235079966_H
#define _NULLSPACE_COLOR_PLUGIN_1591235079966_H
#include "qtusercore/module/creativeinterface.h"
#include "data/interface.h"

class PaintCommand;
class ColorPlugin : public QObject, public CreativeInterface
	, public creative_kernel::UIVisualTracer
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "creative.ColorPlugin")
	Q_INTERFACES(CreativeInterface)

public:
	ColorPlugin(QObject* parent = nullptr);
	virtual ~ColorPlugin();

protected:
	QString name() override;
	QString info() override;

	void initialize() override;
	void uninitialize() override;
protected:
	void onThemeChanged(creative_kernel::ThemeCategory category) override;
	void onLanguageChanged(creative_kernel::MultiLanguage language) override;
protected:
	PaintCommand* m_command;
};

#endif // _NULLSPACE_COLOR_PLUGIN_1591235079966_H