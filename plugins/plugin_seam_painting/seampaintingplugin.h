#ifndef _NULLSPACE_SEAM_PAINTING_PLUGIN_1591235079966_H
#define _NULLSPACE_SEAM_PAINTING_PLUGIN_1591235079966_H
#include "qtusercore/module/creativeinterface.h"
#include "data/interface.h"

class PaintCommand;
class SeamPaintingPlugin : public QObject, public CreativeInterface
	, public creative_kernel::UIVisualTracer
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "creative.SeamPaintingPlugin")
	Q_INTERFACES(CreativeInterface)

public:
	SeamPaintingPlugin(QObject* parent = nullptr);
	virtual ~SeamPaintingPlugin();

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

#endif // _NULLSPACE_SEAM_PAINTING_PLUGIN_1591235079966_H