#include "cxopenandsavefilemanager.h"
#include "cxhandlerbase.h"

#include <QtGui/QDesktopServices>
#include <QtCore/QUrl>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtWidgets/QFileDialog>
#include <QtCore/QOperatingSystemVersion>
#include <QtCore/QSettings>
#include "ccglobal/platform.h"
#include <QtCore/QStandardPaths>
#include <QApplication>
#include "qtusercore/util/settings.h"
#include <regex>
namespace qtuser_core
{

	CXFileOpenAndSaveManager::CXFileOpenAndSaveManager(QObject* parent)
		: QObject(parent)
		, m_invokeObject(nullptr)
		, m_State(OpenSaveState::oss_none)
		, m_externalHandler(nullptr)
		, m_filterKey("base")
	{
	}

	CXFileOpenAndSaveManager::~CXFileOpenAndSaveManager()
	{
	}

	void CXFileOpenAndSaveManager::init(QObject* obj)
	{
#ifdef CXFILE_USE_INVOKE
		m_invokeObject = obj;
#endif
	}

	QString CXFileOpenAndSaveManager::title()
	{
		QString _title("Error");
		switch (m_State)
		{
		case OpenSaveState::oss_open:
		case OpenSaveState::oss_external_open:
			_title = QString("Open");
			break;
		case OpenSaveState::oss_external_save:
		case OpenSaveState::oss_save:
			_title = QString("Save");
			break;
		default:
			break;
		}
		return _title;
	}

	QStringList CXFileOpenAndSaveManager::nameFilters()
	{
		QStringList filterList;
		switch (m_State)
		{
		case OpenSaveState::oss_open:
			filterList = generateFiltersFromHandlers(false);
			break;
		case OpenSaveState::oss_save:
			filterList = generateFiltersFromHandlers(true);
			break;
		case OpenSaveState::oss_external_open:
		case OpenSaveState::oss_external_save:
			if(m_externalHandler)
				filterList = generateFilters(m_externalHandler->suffixesFromFilter());
			break;
		default:
			break;
		}

		return filterList;
	}

	QString CXFileOpenAndSaveManager::currOpenFile()
	{
	    return m_lastOpenFile;  // Halot
		//return m_lastOpenFile.mid(0, m_lastOpenFile.size() - 4);   // c3d
	}

	QStringList CXFileOpenAndSaveManager::generateFilters(const QStringList& extensions)
	{
		QStringList filters;
		QString filtersall;
		for (const QString& ext : extensions)
		{
			filters << QString("*.%1 ").arg(ext);
			filtersall += QString("*.%1 ").arg(ext);
		}
		if (-1 == filters.indexOf(filtersall))
		{
			filters.push_front(filtersall);
		}
		return filters;
	}

	QStringList CXFileOpenAndSaveManager::generateFiltersFromHandlers(bool saveState)
	{
		QMap<QString, QList<CXHandleBase*>>& handlers = saveState ? m_saveHandlers : m_openHandlers;
		QStringList filters;
		QString allSuffix;
		for (QMap<QString, QList<CXHandleBase*>>::iterator it = handlers.begin(); it != handlers.end(); ++it)
		{
			for (CXHandleBase* handler : it.value())
			{
				if (handler->filterKey() != m_filterKey)
					continue;

				QStringList enableFilters = handler->suffixesFromFilter();
				for (const QString& ext : enableFilters)
				{
					QString suffix = QString("*.%1 ").arg(ext);
					if (!filters.contains(suffix))
					{
						filters << suffix;
						allSuffix += suffix;
					}
				}
			}
		}
		filters.push_front(allSuffix);
		return filters;
	}

	QStringList CXFileOpenAndSaveManager::generateSuffixesFromHandlers(bool saveState)
	{
		QMap<QString, QList<CXHandleBase*>>& handlers = saveState ? m_saveHandlers : m_openHandlers;
		QStringList allSuffix;

		for (QMap<QString, QList<CXHandleBase*>>::iterator it = handlers.begin(); it != handlers.end(); ++it)
		{
			if (it.value().count() > 0)
				allSuffix << it.key();
		}

		return allSuffix;
	}

	QString CXFileOpenAndSaveManager::generateFilterFromHandlers(bool saveState)
	{
		QMap<QString, QList<CXHandleBase*>>& handlers = saveState ? m_saveHandlers : m_openHandlers;
		QList<CXHandleBase*> uniqueHandlers;
		if (m_externalHandler)
		{
			uniqueHandlers.append(m_externalHandler);
		}
		else
		{
			for (QMap<QString, QList<CXHandleBase*>>::iterator it = handlers.begin();
				it != handlers.end(); ++it)
			{
				for (CXHandleBase* handler : it.value())
				{
					if (handler->filterKey() != m_filterKey)
						continue;

					if (!uniqueHandlers.contains(handler))
						uniqueHandlers.append(handler);
				}
			}
		}

		QString filter;
		//QString allFilter="All(%1);;";
		QStringList allFilter;
		for (CXHandleBase* handle : uniqueHandlers)
		{
			allFilter.append(handle->suffixesFromFilter());
			filter += handle->filter();
			if (handle != uniqueHandlers.back())
				filter += ";;";
		}
		filter = QString("All (*.%1);;").arg(allFilter.join(" *.")) + filter;
		return filter;
	}

	void CXFileOpenAndSaveManager::fileOpen(const QString& url)
	{
		openWithUrl(QUrl(url));
	};

	void CXFileOpenAndSaveManager::filesOpen(const QList<QUrl>& urls)
	{
		QStringList fileNames;
		for (const QUrl& url : urls)
			fileNames << url.toLocalFile();

		openWithNames(fileNames);
	};

	void CXFileOpenAndSaveManager::fileSave(const QString& url)
	{
		saveWithUrl(QUrl(url));
	}

	bool CXFileOpenAndSaveManager::cancelHandle()
	{
		if (m_externalHandler)
		{
			if (m_State == OpenSaveState::oss_save || m_State == OpenSaveState::oss_external_save)
			{
		           m_externalHandler->cancelHandle();
				//dlp save gcode canel back Mainscene
				//creative_kernel::AbstractKernelUI::getSelf()->backMainSceneShow();
			}
			m_externalHandler = nullptr;
			m_State = OpenSaveState::oss_none;

			return true;
		}
		return true;
	}

	void CXFileOpenAndSaveManager::qOpen()
	{
		open();
	}

	void CXFileOpenAndSaveManager::qOpen(QObject* receiver)
	{
		CXHandleBase* handle = dynamic_cast<CXHandleBase*>(receiver);
		open(handle);
	}

	void CXFileOpenAndSaveManager::qSave(QObject* receiver)
	{
		CXHandleBase* handle = dynamic_cast<CXHandleBase*>(receiver);
		save(handle);
	}

	void CXFileOpenAndSaveManager::open(CXHandleBase* receiver, const QString& title, bool isSingleFile)
	{
		m_State = OpenSaveState::oss_open;
		if (receiver)
		{
			m_externalHandler = receiver;
			m_State = OpenSaveState::oss_external_open;
		}

		if (m_invokeObject)
		{
			QMetaObject::invokeMethod(m_invokeObject,
				"requestOpenFileDialog", Q_ARG(QVariant, QVariant::fromValue(this)));
		}
		else
		{
			auto f = [this](const QStringList& files) {
				openWithNames(files);
			};
			QString filter = generateFilterFromHandlers(false);
			if (isSingleFile)
			{
				
				
				dialogOpenFile(filter,
					title.isEmpty() ? QObject::tr("OpenFile") : title
					, f);
			}
			else
			{
				dialogOpenFiles(filter,
					title.isEmpty() ? QObject::tr("OpenFile") : title
					, f);
			}
			m_externalHandler = nullptr;
		}
	}

	void CXFileOpenAndSaveManager::save(CXHandleBase* receiver, const QString& defaultName, const QString& title, const QString& filter)
	{
		m_State = OpenSaveState::oss_save;
		if (receiver)
		{
			m_externalHandler = receiver;
			m_State = OpenSaveState::oss_external_save;
		}

		if (m_invokeObject)
		{
			QMetaObject::invokeMethod(m_invokeObject,
				"requestSaveFileDialog", Q_ARG(QVariant, QVariant::fromValue(this)));
		}
		else
		{
			auto f = [this](const QString& file) {
				saveWithName(file);
				m_lastSaveFile = file;
			};

			dialogSave(filter.isEmpty() ? generateFilterFromHandlers(true) : filter,
				defaultName.isEmpty() ? m_lastOpenFile : defaultName,
				title.isEmpty() ? QObject::tr("Save") : title
				, f);

			m_externalHandler = nullptr;
		}
	}

	void CXFileOpenAndSaveManager::openWithParams(const QStringList& params)
	{
		if (params.size() > 1)
		{
			qDebug() << "params length > 1. maybe need open file";
			QString needOpenName;

			for (const QString& fileName : params)
			{
				QFile file(fileName);
				QFileInfo fileInfo(fileName);
				QString suffix = fileInfo.suffix();
				if (file.exists() && m_openFilterList.contains(suffix))
				{
					needOpenName = fileName;
					break;
				}
			}

			openWithName(needOpenName);
		}
	}

	void CXFileOpenAndSaveManager::setFilterKey(const QString& filterKey)
	{
		m_filterKey = filterKey;
	}

	bool CXFileOpenAndSaveManager::openWithName(const QString& fileName)
	{
		if (m_externalHandler)
		{
			m_externalHandler->handle(fileName);
			m_externalHandler = nullptr;
			m_State = OpenSaveState::oss_none;
			for (CXFileOpenSaveCallback* callback : m_callbacks)
				callback->onFileOpened(fileName);
			return true;
		}

		QFileInfo info(fileName);
		QString suffix = info.suffix();
		suffix = suffix.toLower();
		m_lastOpenFile = info.baseName();
		m_lastOpenFilePath = info.absoluteFilePath();
		return openWithNameSuffix(fileName, suffix);
	}

	void CXFileOpenAndSaveManager::openWithNames(const QStringList& fileNames)
	{
		if (m_externalHandler)
		{
			m_externalHandler->handle(fileNames);
			m_externalHandler = nullptr;
			m_State = OpenSaveState::oss_none;
			return;
		}

		if (fileNames.size() > 0)
		{
			QFileInfo info(fileNames.at(0));
			QString suffix = info.suffix();
			suffix = suffix.toLower();
			m_lastOpenFile = info.baseName();
			CXHandleBase* handler = findHandler(suffix, m_openHandlers);
			if (!handler)
			{
				qDebug() << "not register handler for this file type!";
				return;
			}

			handler->handle(fileNames);
			openWithNamesSuffix(fileNames);
		}
	}

	bool CXFileOpenAndSaveManager::openWithUrl(const QUrl& url)
	{
		return openWithName(url.toLocalFile());
	}

	bool CXFileOpenAndSaveManager::openWithString(const QString& commonName)
	{
		if (commonName.startsWith("file:///"))
		{
			QUrl url(commonName);
			openWithUrl(url);
		}
		else
		{
			openWithName(commonName);
		}
		return true;
	}

	bool CXFileOpenAndSaveManager::openWithUrls(const QList<QUrl>& urls)
	{
		if (urls.size() == 1)
		{
			return openWithName(urls[0].toLocalFile());
		}
		else if (urls.size() > 1)
		{
			QStringList fileNames;
			for (QUrl url : urls)
			{
				fileNames.push_back(url.toLocalFile());
			}
			openWithNames(fileNames);
			return true;
		}
		return false;
	}

	bool CXFileOpenAndSaveManager::saveWithName(const QString& fileName)
	{
		m_lastSaveFile = fileName;
		qDebug() << "saveWithName =" << m_lastSaveFile;
		if (m_externalHandler)
		{
			m_externalHandler->handle(fileName);
			m_externalHandler = nullptr;
			m_State = OpenSaveState::oss_none;
			return true;
		}

		QFileInfo info(fileName);
		QString suffix = info.suffix();
		suffix = suffix.toLower();
		return saveWithNameSuffix(fileName, suffix);
	}

	bool CXFileOpenAndSaveManager::saveWithUrl(const QUrl& url)
	{
		return saveWithName(url.toLocalFile());
	}

	bool CXFileOpenAndSaveManager::openWithNameSuffix(const QString& fileName, const QString suffix)
	{
		CXHandleBase* handler = findHandler(suffix, m_openHandlers);
		if (!handler)
		{
			qDebug() << "not register handler for this file type!";
			return false;
		}

		handler->handle(fileName);
		for (CXFileOpenSaveCallback* callback : m_callbacks)
			callback->onFileOpened(fileName);
		return true;
	}

	bool CXFileOpenAndSaveManager::openWithNamesSuffix(const QStringList& fileNames)
	{
		for (CXFileOpenSaveCallback* callback : m_callbacks)
		{
			for (QString fileName : fileNames)
			{
				callback->onFileOpened(fileName);
			}
		}
		return true;
	}

	bool CXFileOpenAndSaveManager::saveWithNameSuffix(const QString& fileName, const QString suffix)
	{
		CXHandleBase* handler = findHandler(suffix, m_saveHandlers);
		if (!handler)
		{
			qDebug() << "not register handler for this file type!";
			return false;
		}

		handler->handle(fileName);
		return true;
	}

	CXHandleBase* CXFileOpenAndSaveManager::findHandler(const QString& suffix, QMap<QString, QList<CXHandleBase*>>& handlers)
	{
		QMap<QString, QList<CXHandleBase*>>::iterator it = handlers.find(suffix);
		if (it != handlers.end())
		{
			for (CXHandleBase* handler : it.value())
			{
				if (handler->filterKey() == m_filterKey)
					return handler;
			}
		}
		return nullptr;
	}

	void CXFileOpenAndSaveManager::registerOpenHandler(CXHandleBase* handler)
	{
		if (handler)
			registerOpenHandler(handler->suffixesFromFilter(), handler);
	}

	void CXFileOpenAndSaveManager::unRegisterSaveHandler(CXHandleBase* handler)
	{
		unRegisterHandler(handler, m_saveHandlers);
	}

	void CXFileOpenAndSaveManager::registerOpenHandler(const QStringList& suffixes, CXHandleBase* handler)
	{
		for (const QString& suffix : suffixes)
			registerOpenHandler(suffix, handler);
	}

	void CXFileOpenAndSaveManager::unRegisterOpenHandler(const QStringList& suffixes)
	{
		for (const QString& suffix : suffixes)
			unRegisterOpenHandler(suffix);
	}

	void CXFileOpenAndSaveManager::unRegisterOpenHandler(CXHandleBase* handler)
	{
		unRegisterHandler(handler, m_openHandlers);
	}

	void CXFileOpenAndSaveManager::registerOpenHandler(const QString& suffix, CXHandleBase* handler)
	{
		if (registerHandler(suffix, handler, m_openHandlers))
			m_openFilterList << suffix;
	}

	void CXFileOpenAndSaveManager::unRegisterOpenHandler(const QString& suffix)
	{
		unRegisterHandler(suffix, m_openHandlers);
		m_openFilterList.removeOne(suffix);
	}

	void CXFileOpenAndSaveManager::registerSaveHandler(CXHandleBase* handler)
	{
		if (handler)
			registerSaveHandler(handler->suffixesFromFilter(), handler);
	}

	void CXFileOpenAndSaveManager::registerSaveHandler(const QStringList& suffixes, CXHandleBase* handler)
	{
		for (const QString& suffix : suffixes)
			registerSaveHandler(suffix, handler);
	}

	void CXFileOpenAndSaveManager::unRegisterSaveHandler(const QStringList& suffixes)
	{
		for (const QString& suffix : suffixes)
			unRegisterSaveHandler(suffix);
	}

	void CXFileOpenAndSaveManager::registerSaveHandler(const QString& suffix, CXHandleBase* handler)
	{
		if (registerHandler(suffix, handler, m_saveHandlers))
			m_saveFilterList << suffix;
	}

	void CXFileOpenAndSaveManager::unRegisterSaveHandler(const QString& suffix)
	{
		unRegisterHandler(suffix, m_saveHandlers);
		m_saveFilterList.removeOne(suffix);
	}

	bool CXFileOpenAndSaveManager::registerHandler(const QString& suffix, CXHandleBase* handler, QMap<QString, QList<CXHandleBase*>>& handlers)
	{
		if (!handler)
			return false;

		QMap<QString, QList<CXHandleBase*>>::iterator it = handlers.find(suffix);
		if(it == handlers.end())
			it = handlers.insert(suffix, QList<CXHandleBase*>());

		if(!it.value().contains(handler))
			it.value().append(handler);
		return true;
	}

	void CXFileOpenAndSaveManager::unRegisterHandler(const QString& suffix, QMap<QString, QList<CXHandleBase*>>& handlers)
	{
		handlers.remove(suffix);
	}

	void CXFileOpenAndSaveManager::unRegisterHandler(CXHandleBase* handler, QMap<QString, QList<CXHandleBase*>>& handlers)
	{
		if (!handler)
			return;

		for (QMap<QString, QList<CXHandleBase*>>::Iterator it = handlers.begin();
			it != handlers.end(); ++it)
			it.value().removeOne(handler);
	}

	QString CXFileOpenAndSaveManager::lastOpenFileName()
	{
		return m_lastOpenFile;
	}

	QString CXFileOpenAndSaveManager::lastOpenFilePath()
	{
		return m_lastOpenFilePath;
	}

	QString CXFileOpenAndSaveManager::lastSaveFileName()
	{
		return m_lastSaveFile;
	}

	void CXFileOpenAndSaveManager::setLastOpenFileName(QString filePath)
	{
	    m_lastOpenFile = filePath;
	}

	void CXFileOpenAndSaveManager::setLastSaveFileName(QString filePath)
	{
		m_lastSaveFile = filePath;
	}

	bool CXFileOpenAndSaveManager::isSupportSuffix(const QString& suffix)
	{
		return m_openFilterList.contains(suffix);
	}

	void CXFileOpenAndSaveManager::openDesktopFolder()
	{
		QFileInfo info(m_lastSaveFile);
		QDesktopServices::openUrl(QUrl::fromLocalFile(info.path()));
	}

	void CXFileOpenAndSaveManager::openLastSaveFolder()
	{
		qDebug() << "openLastSaveFolder =" <<m_lastSaveFile;
		QFileInfo fileInfo(m_lastSaveFile);
		openFolder(fileInfo.absolutePath());
	}

	void CXFileOpenAndSaveManager::openFolder(const QString& folder)
	{
		QOperatingSystemVersion version = QOperatingSystemVersion::current();
		QProcess process;
		if(version.type() == QOperatingSystemVersion::Windows)
		{
			QString dir = folder;
			dir.replace("/", "\\");
		    process.startDetached(QStringLiteral("explorer ") + dir);
		}
		else if(version.type() == QOperatingSystemVersion::MacOS)
		{
		    m_lastSaveFile.replace("\\","/");
		    process.startDetached("/usr/bin/open",QStringList() << folder);
		}

		qDebug() << QString("CXFileOpenAndSaveManager::openFolder : [%1]").arg(folder);
	}

	void CXFileOpenAndSaveManager::addCXFileOpenSaveCallback(CXFileOpenSaveCallback* callback)
	{
		if (callback && !m_callbacks.contains(callback))
			m_callbacks.append(callback);
	}

	void CXFileOpenAndSaveManager::removeCXFileOpenSaveCallback(CXFileOpenSaveCallback* callback)
	{
		if (callback)
			m_callbacks.removeOne(callback);
	}

	size_t CXFileOpenAndSaveManager::getFileSize(const QString& fileName)
	{
		if (fileName.isEmpty())
			return 0;

		FILE* file = fopen(fileName.toLocal8Bit().constData(), "rb+");
		if (nullptr == file)
		{
			return 0;
		}

		fseek(file, 0, SEEK_END);
		size_t filesize = _cc_ftelli64(file);
		fclose(file);

		return filesize;
	}
	void dialogOpenFile(const QString& filter, const QString& title, loadFunc func)
	{
		if (!func)
			return;
		qtuser_core::VersionSettings setting;
		QString lastPath = setting.value("dialogLastPath", "").toString();
#ifdef Q_OS_LINUX
		QString fileName = QFileDialog::getOpenFileName(
			nullptr, title,
			lastPath, filter, nullptr, QFileDialog::DontUseNativeDialog);
#else
		QString fileName = QFileDialog::getOpenFileName(
			nullptr, title,
			lastPath, filter);
#endif
		if (fileName.isEmpty())
			return;
		QFileInfo fileinfo = QFileInfo(fileName);
		lastPath = fileinfo.path();
		setting.setValue("dialogLastPath", lastPath);
		QStringList files;
		files << fileName;
		func(files);
	}
	void dialogOpenFiles(const QString& filter, const QString& title,loadFunc func)
	{
		if (!func)
			return;
		qtuser_core::VersionSettings setting;
		QString lastPath = setting.value("dialogLastPath", "").toString();
#ifdef Q_OS_LINUX
		QStringList fileName = QFileDialog::getOpenFileNames(
			nullptr, title,
			lastPath, filter,nullptr,QFileDialog::DontUseNativeDialog);
#else
		QStringList fileName = QFileDialog::getOpenFileNames(
			nullptr, title,
			lastPath, filter);
#endif
		if (fileName.isEmpty())
			return;
		QFileInfo fileinfo = QFileInfo(fileName.first());
		lastPath = fileinfo.path();
		setting.setValue("dialogLastPath", lastPath);
		func(fileName);
	}
//	void dialogOpenFile(const QString& filter, const QString& title, loadFunc func)
//	{
//		if (!func)
//			return;
//		qtuser_core::VersionSettings setting;
//		QString lastPath = setting.value("dialogLastPath", "").toString();
//#ifdef Q_OS_LINUX
//		QString fileName = QFileDialog::getOpenFileNames(
//			nullptr, title,
//			lastPath, filter, nullptr, QFileDialog::DontUseNativeDialog);
//#else
//		QString fileName = QFileDialog::getOpenFileName(
//			nullptr, title,
//			lastPath, filter);
//#endif
//		if (fileName.isEmpty())
//			return;
//		QFileInfo fileinfo = QFileInfo(fileName);
//		lastPath = fileinfo.path();
//		setting.setValue("dialogLastPath", lastPath);
//		func(fileName);
//	}
	void dialogSave(const QString& filter, const QString& defaultName, const QString& title,saveFunc func)
	{
		if (!func)
			return;
		qtuser_core::VersionSettings setting;
		QString lastPath = setting.value("dialogLastPath", "").toString() + "/" + defaultName;
#ifdef Q_OS_OSX
		lastPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)+ "/" + defaultName;
#endif
		qDebug() <<  "lastPath =" << lastPath;
		QString selectedFilter;
#ifdef Q_OS_LINUX
		QString fileName = QFileDialog::getSaveFileName(
			nullptr, title,
			lastPath, filter,&selectedFilter,QFileDialog::DontUseNativeDialog);
#else
		
		QString fileName = QFileDialog::getSaveFileName(
			nullptr, title,
			lastPath, filter, &selectedFilter);
#endif
		if (fileName.isEmpty())
			return;

		auto match = std::smatch{};
		std::string test = selectedFilter.toStdString();
		std::regex pattern(R"(\*\.(\w+))");
		std::smatch matches;

		auto begin = std::sregex_iterator(test.begin(), test.end(), pattern);
		auto end = std::sregex_iterator();

		bool found = false;
		std::string suffix;
		for (std::sregex_iterator i = begin; i != end; ++i)
		{
			std::smatch match = *i;
			if (QFileInfo(fileName).suffix() == QString::fromStdString(match[1].str()))
			{
				found = true;
				break;
			}
			if (suffix.empty())
			{
				suffix = match[1].str();
			}
		}
		if (!found)
		{
			fileName = QString("%1.%2").arg(fileName).arg(suffix.c_str());
		}

		lastPath = QFileInfo(fileName).path();
		setting.setValue("dialogLastPath", lastPath);
		func(fileName);
	}
}
