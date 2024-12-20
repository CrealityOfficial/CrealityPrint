#ifndef _NULLSPACE_MODEL_EFFECT_PLUGIN_202312111728_H
#define _NULLSPACE_MODEL_EFFECT_PLUGIN_202312111728_H
#include "qtusercore/module/creativeinterface.h"

class ModelEffectCommand;
class ModelEffectPlugin: public QObject, public CreativeInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "creative.ModelEffectPlugin")
	Q_INTERFACES(CreativeInterface)
public:
	ModelEffectPlugin(QObject* parent = nullptr);
	virtual ~ModelEffectPlugin();

protected:
	QString name() override;
	QString info() override;

	void initialize() override;
	void uninitialize() override;

protected:
	ModelEffectCommand* m_command;
};
#endif // _NULLSPACE_MODEL_EFFECT_PLUGIN_202312111728_H
