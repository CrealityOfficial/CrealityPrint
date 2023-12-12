#ifndef _QTUSER_CORE_RESOURCESFINDER_1590131561766_H
#define _QTUSER_CORE_RESOURCESFINDER_1590131561766_H
#include "qtusercore/qtusercoreexport.h"
#include <QtCore/QStringList>

namespace qtuser_core
{
	class QTUSER_CORE_API ResourcesFinder: public QObject
	{
		ResourcesFinder(QObject* parent = nullptr);
	public:
		virtual ~ResourcesFinder();

		static ResourcesFinder& instance();

		void addSearchPath(const QString& path);
	protected:
		static ResourcesFinder m_resourcesFinder;
	};
	
	QTUSER_CORE_API QStringList dynamicLoadFilters(const QString& prefix = "plugin");
	QTUSER_CORE_API QString getOrCreateAppDataLocation(const QString& folder = "");
	QTUSER_CORE_API QString getResourcesFolder(const QString& folder);
	QTUSER_CORE_API bool copyDir(const QString& source, const QString& destination, bool override);
	QTUSER_CORE_API bool copyFileToPath(const QString& source, const QString& destination, bool deleteFileIfExist = true);
	QTUSER_CORE_API bool copyFile(const QString& source, const QString& destination, bool cover = true);

	QTUSER_CORE_API QStringList allFiles(const QString& directory, const QStringList& filters = QStringList());
}

#define APP_RESOURCES qtuser_core::ResourcesFinder::instance()
#endif // _QTUSER_CORE_RESOURCESFINDER_1590131561766_H
