#ifndef CX_KERNEL_DUMPPROXY_1672905000400_H
#define CX_KERNEL_DUMPPROXY_1672905000400_H
#include <QtCore/QObject>

namespace cxkernel
{
	class DumpProxy : public QObject
	{
		Q_OBJECT
	public:
		DumpProxy(QObject* parent = nullptr);
		virtual ~DumpProxy();

		void initialize(const QString& version, 
			const QString& cloudId, const QString& path);

		QString cloudId();
		QString dumpPath();
		QString version();
	protected:
		QString m_version;
		QString m_cloudId;
		QString m_path;
	};
}

#endif // CX_KERNEL_DUMPPROXY_1672905000400_H