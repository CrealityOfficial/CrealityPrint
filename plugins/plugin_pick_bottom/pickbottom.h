#ifndef _NULLSPACE_PICKBOTTOM_1589849922902_H
#define _NULLSPACE_PICKBOTTOM_1589849922902_H
#include "qtusercore/module/creativeinterface.h"

class PickBottomCommand;
class PickBottom: public QObject, public CreativeInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "creative.PickBottom")
	Q_INTERFACES(CreativeInterface)
public:
	PickBottom(QObject* parent = nullptr);
	virtual ~PickBottom();

protected:
	QString name() override;
	QString info() override;

	void initialize() override;
	void uninitialize() override;

protected:
	PickBottomCommand* m_command;
};
#endif // _NULLSPACE_PICKBOTTOM_1589849922902_H
