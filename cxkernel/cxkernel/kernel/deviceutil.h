#ifndef CXKERNEL_DEVICEUTIL_1686035882039_H
#define CXKERNEL_DEVICEUTIL_1686035882039_H
#include "cxkernel/cxkernelinterface.h"
#include <QtCore/QObject>

namespace cxkernel
{
	class CXKERNEL_API DeviceUtil : public QObject
	{
		Q_OBJECT
	public:
		DeviceUtil(QObject* parent = nullptr);
		virtual ~DeviceUtil();

		Q_INVOKABLE bool isDesktopGL();
		Q_INVOKABLE bool isGles();
		Q_INVOKABLE bool isSoftwareGL();
	};
}

#endif // CXKERNEL_DEVICEUTIL_1686035882039_H