#ifndef CREATIVE_KERNEL_SENSORANALYTICS_1673681365358_H
#define CREATIVE_KERNEL_SENSORANALYTICS_1673681365358_H
#include "basickernelexport.h"

namespace creative_kernel
{
	class BASIC_KERNEL_API SensorAnalytics : public QObject
	{
		Q_OBJECT
	public:
		SensorAnalytics(QObject* parent = nullptr);
		virtual ~SensorAnalytics();

		Q_INVOKABLE void trace(const QString& eventType, const QString& eventAction);

	protected:
		bool initSDK();
		QString getPCMacAddress();
	};
}

#endif // CREATIVE_KERNEL_SENSORANALYTICS_1673681365358_H