#ifndef _NULLSPACE_Repair_1589849922902_H
#define _NULLSPACE_Repair_1589849922902_H
#include "qtusercore/module/creativeinterface.h"

class RepairCommand;
class Repair: public QObject, public CreativeInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "creative.Repair")
	Q_INTERFACES(CreativeInterface)
public:
	Repair(QObject* parent = nullptr);
	virtual ~Repair();

protected:
	QString name() override;
	QString info() override;

	void initialize() override;
	void uninitialize() override;

protected:
	RepairCommand* m_command;
};
#endif // _NULLSPACE_Repair_1589849922902_H
