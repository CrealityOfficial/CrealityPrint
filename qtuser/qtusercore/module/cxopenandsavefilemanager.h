#ifndef QTUSER_CORE_CXFileOpenAndSaveManager_H
#define QTUSER_CORE_CXFileOpenAndSaveManager_H
#include "qtusercore/qtusercoreexport.h"

#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QUrl>

namespace qtuser_core
{
	enum class OpenSaveState
	{
		oss_none,
		oss_open,
		oss_save,
		oss_external_open,
		oss_external_save
	};

	class CXFileOpenSaveCallback
	{
	public:
		virtual ~CXFileOpenSaveCallback() {}

		virtual void onFileOpened(const QString& name) = 0;
		//virtual void onFileOpened(const QStringList& names) = 0;
	};

	class CXHandleBase;
	class QTUSER_CORE_API CXFileOpenAndSaveManager : public QObject
	{
		Q_OBJECT
	public:
		CXFileOpenAndSaveManager(QObject* parent = nullptr);
		virtual ~CXFileOpenAndSaveManager();

		void init(QObject* obj);

		Q_INVOKABLE QString title();
		Q_INVOKABLE QStringList nameFilters();
		Q_INVOKABLE QString currOpenFile();
		Q_INVOKABLE void fileOpen(const QString& url);
		Q_INVOKABLE void fileSave(const QString& url);
		Q_INVOKABLE void filesOpen(const QList<QUrl>& urls);
		Q_INVOKABLE bool cancelHandle();
		void addCXFileOpenSaveCallback(CXFileOpenSaveCallback* callback);
		void removeCXFileOpenSaveCallback(CXFileOpenSaveCallback* callback);
	public:
		void open(CXHandleBase* receiver = nullptr, const QString& title = QString(""));
		void save(CXHandleBase* receiver = nullptr, const QString& defaultName = QString(""), const QString& title = QString(""));

		Q_INVOKABLE void qOpen(QObject* receiver);
		Q_INVOKABLE void qSave(QObject* receiver);
		Q_INVOKABLE void qOpen();

		Q_INVOKABLE void setFilterKey(const QString& filterKey);
		void openWithParams(const QStringList& fileNames);
		bool openWithName(const QString& fileName);
		void openWithNames(const QStringList& fileNames);
		Q_INVOKABLE bool openWithUrl(const QUrl& url);
		Q_INVOKABLE bool openWithUrls(const QList<QUrl>& urls);
		bool openWithString(const QString& commonName);

		bool saveWithName(const QString& fileName);
		bool saveWithUrl(const QUrl& url);

		void registerOpenHandler(CXHandleBase* handler);
		void unRegisterOpenHandler(CXHandleBase* handler);
		void registerOpenHandler(const QStringList& suffixes, CXHandleBase* handler);
		void unRegisterOpenHandler(const QStringList& suffixes);
		void registerOpenHandler(const QString& suffix, CXHandleBase* handler);
		void unRegisterOpenHandler(const QString& suffix);

		void registerSaveHandler(CXHandleBase* handler);
		void unRegisterSaveHandler(CXHandleBase* handler);
		void registerSaveHandler(const QStringList& suffixes, CXHandleBase* handler);
		void unRegisterSaveHandler(const QStringList& suffixes);
		void registerSaveHandler(const QString& suffix, CXHandleBase* handler);
		void unRegisterSaveHandler(const QString& suffix);

		QString lastOpenFileName();
		QString lastOpenFilePath();
		QString lastSaveFileName();
		void setLastOpenFileName(QString filePath);
		void setLastSaveFileName(QString filePath);

		bool isSupportSuffix(const QString& suffix);

		Q_INVOKABLE void openDesktopFolder();
		Q_INVOKABLE void openLastSaveFolder();
		void openFolder(const QString& folder);

		size_t getFileSize(const QString& fileName);
	protected:
		bool openWithNameSuffix(const QString& fileName, const QString suffix);
		bool openWithNamesSuffix(const QStringList& fileNames);
		bool saveWithNameSuffix(const QString& fileName, const QString suffix);

		bool registerHandler(const QString& suffix, CXHandleBase* handler, QMap<QString, QList<CXHandleBase*>>& handlers);
		void unRegisterHandler(const QString& suffix, QMap<QString, QList<CXHandleBase*>>& handlers);
		void unRegisterHandler(CXHandleBase* handler, QMap<QString, QList<CXHandleBase*>>& handlers);
		CXHandleBase* findHandler(const QString& suffix, QMap<QString, QList<CXHandleBase*>>& handlers);

		QStringList generateFilters(const QStringList& extensions);
		QStringList generateFiltersFromHandlers(bool saveState);
		QStringList generateSuffixesFromHandlers(bool saveState);
		QString generateFilterFromHandlers(bool saveState);
	protected:
		QObject* m_invokeObject;
		OpenSaveState m_State;

		QString m_filterKey;
		QMap<QString, QList<CXHandleBase*>> m_openHandlers;
		QStringList m_openFilterList;

		QMap<QString, QList<CXHandleBase*>> m_saveHandlers;
		QStringList m_saveFilterList;

		CXHandleBase* m_externalHandler;

		QString m_lastSaveFile;
		QString m_lastOpenFile;
		QString m_lastOpenFilePath;

		QList<CXFileOpenSaveCallback*> m_callbacks;
	};

	typedef std::function<void(const QStringList& fileNames)> loadFunc;
	QTUSER_CORE_API void dialogOpenFiles(const QString& filter, const QString& title,loadFunc func);

	typedef std::function<void(const QString& fileName)> saveFunc;
	QTUSER_CORE_API void dialogSave(const QString& filter, const QString& defaultName, const QString& title, saveFunc func);
}

#endif // !QTUSER_CORE_CXFileOpenAndSaveManager_H
