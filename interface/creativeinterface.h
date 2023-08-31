#ifndef _NULLSPACE_CREATIVEINTERFACE_1589956011258_H
#define _NULLSPACE_CREATIVEINTERFACE_1589956011258_H
#include <QtCore/QObject>

class CreativeInterface
{
public:
	virtual ~CreativeInterface() {}

	virtual QString name() = 0;
	virtual QString info() = 0;

	virtual void initialize() = 0;
	virtual void uninitialize() = 0;
};

Q_DECLARE_INTERFACE(CreativeInterface, "creative.CreativeInterface")
#endif // _NULLSPACE_CREATIVEINTERFACE_1589956011258_H
