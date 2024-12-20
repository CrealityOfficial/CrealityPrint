#ifndef _ADAPTIVE_LAYER_PLUGIN_H
#define _ADAPTIVE_LAYER_PLUGIN_H
#include "qtusercore/module/creativeinterface.h"
#include "data/interface.h"

class AdaptiveLayerCommand;
class AdaptiveLayerPlugin : public QObject, public CreativeInterface
	, public creative_kernel::UIVisualTracer
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "creative.AdaptiveLayerPlugin")
	Q_INTERFACES(CreativeInterface)

public:
	AdaptiveLayerPlugin(QObject* parent = nullptr);
	virtual ~AdaptiveLayerPlugin();

protected:
	QString name() override;
	QString info() override;

	void initialize() override;
	void uninitialize() override;
protected:
	void onThemeChanged(creative_kernel::ThemeCategory category) override;
	void onLanguageChanged(creative_kernel::MultiLanguage language) override;
protected:
	AdaptiveLayerCommand* m_command;
};

#endif // _ADAPTIVE_LAYER_PLUGIN_H