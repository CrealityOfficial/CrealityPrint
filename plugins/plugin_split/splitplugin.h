#ifndef _NULLSPACE_SPLITPLUGIN_1591234859229_H
#define _NULLSPACE_SPLITPLUGIN_1591234859229_H
#include "qtusercore/module/creativeinterface.h"

class SplitCommand;
class SplitPlugin: public QObject, public CreativeInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "creative.SplitPlugin")
	Q_INTERFACES(CreativeInterface)
public:
	SplitPlugin(QObject* parent = nullptr);
	virtual ~SplitPlugin();

protected:
	QString name() override;
	QString info() override;

	void initialize() override;
	void uninitialize() override;

protected:
	SplitCommand* m_command;
};
#endif // _NULLSPACE_SPLITPLUGIN_1591234859229_H
