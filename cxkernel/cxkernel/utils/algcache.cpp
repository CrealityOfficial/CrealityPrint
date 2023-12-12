#include "algcache.h"
#include "qtusercore/string/resourcesfinder.h"
#include "qtusercore/module/systemutil.h"
#include <QtCore/QUuid>

namespace cxkernel
{
	AlgCache::AlgCache(QObject* parent)
		: QObject(parent)
	{

	}

	AlgCache::~AlgCache()
	{

	}

	QString AlgCache::createNewAlgCache(const QString& alg, bool removeOld)
	{
		if (alg.isEmpty())
			return QString();

		QString folder = QString("AlgrithmCache/%2").arg(alg);
		QString directory = qtuser_core::getOrCreateAppDataLocation(folder);
		if (removeOld)
		{
			clearPath(directory);
		}

		QString fileName = QString("%1/%2.%3").arg(directory)
			.arg(QUuid::createUuid().toString()).arg(alg);
		return fileName;
	}
}