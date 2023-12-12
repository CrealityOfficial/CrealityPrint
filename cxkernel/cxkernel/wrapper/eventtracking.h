#ifndef CREATIVE_KERNEL_SENSORANALYTICS_1673681365358_H
#define CREATIVE_KERNEL_SENSORANALYTICS_1673681365358_H
#include "cxkernel/cxkernelinterface.h"

namespace cxkernel
{
	class CXKERNEL_API EventTracking : public QObject
	{
		Q_OBJECT
	public:
		EventTracking(QObject* parent = nullptr);
		virtual ~EventTracking();

		Q_INVOKABLE void setPath(const QString& path);
		Q_INVOKABLE void trace(const QString& eventType, const QString& eventAction);
	protected:
		bool initSDK();
		QString getPCMacAddress();

	protected:
		QString m_path;
	};
}

#endif // CREATIVE_KERNEL_SENSORANALYTICS_1673681365358_H