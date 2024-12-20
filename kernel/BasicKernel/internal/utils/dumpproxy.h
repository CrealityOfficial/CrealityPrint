#ifndef CREATIVE_KERNEL_DUMPPROXY_1672905000400_H
#define CREATIVE_KERNEL_DUMPPROXY_1672905000400_H
#include <QtCore/QObject>

namespace creative_kernel
{
	class DumpProxy : public QObject
	{
		Q_OBJECT
	public:
		DumpProxy(QObject* parent = nullptr);
		virtual ~DumpProxy();

		void initialize();
	protected:

	};
}

#endif // CREATIVE_KERNEL_DUMPPROXY_1672905000400_H