#ifndef QTUSER_CORE_SETTINGCACHE_1616496145215_H
#define QTUSER_CORE_SETTINGCACHE_1616496145215_H
#include "qtusercore/qtusercoreexport.h"
#include <QtCore/QString>

namespace qtuser_core
{
	QTUSER_CORE_API void cacheString(const QString& groupName, const QString& key, const QString& value);
	QTUSER_CORE_API QString traitString(const QString& groupName, const QString& key, const QString& defaultStr = QString(""));

	QTUSER_CORE_API void cacheStrings(const QString& key, const QStringList& values);
	QTUSER_CORE_API QStringList traitStrings(const QString& key);

	QTUSER_CORE_API void cacheInt(const QString& groupName, const QString& key, int value);
	QTUSER_CORE_API int traitInt(const QString& groupName, const QString& key, int value = -1);
}

#endif // QTUSER_CORE_SETTINGCACHE_1616496145215_H