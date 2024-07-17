#ifndef _NULLSPACE_SUPPORT_PAINTING_PLUGIN_1591235079966_H
#define _NULLSPACE_SUPPORT_PAINTING_PLUGIN_1591235079966_H
#include "qtusercore/module/creativeinterface.h"
#include "data/interface.h"

class PaintCommand;
class SupportPaintingPlugin : public QObject, public CreativeInterface
	, public creative_kernel::UIVisualTracer
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "creative.SupportPaintingPlugin")
	Q_INTERFACES(CreativeInterface)

public:
	SupportPaintingPlugin(QObject* parent = nullptr);
	virtual ~SupportPaintingPlugin();

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

#endif // _NULLSPACE_SUPPORT_PAINTING_PLUGIN_1591235079966_H