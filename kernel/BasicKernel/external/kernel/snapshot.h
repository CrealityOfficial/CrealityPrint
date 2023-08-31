#ifndef CREATIVE_KERNEL_SNAPSHOT_1679488630711_H
#define CREATIVE_KERNEL_SNAPSHOT_1679488630711_H
#include "basickernelexport.h"

namespace creative_kernel
{
	class BASIC_KERNEL_API SnapShot : public QObject
	{
		Q_OBJECT
	public:
		SnapShot(QObject* parent = nullptr);
		virtual ~SnapShot();

		Q_INVOKABLE void saveSnapShot();
		Q_INVOKABLE void loadLastSnapShot();
		Q_INVOKABLE void loadSnapShot(const QString& fileName);
	};
}

#endif // CREATIVE_KERNEL_SNAPSHOT_1679488630711_H