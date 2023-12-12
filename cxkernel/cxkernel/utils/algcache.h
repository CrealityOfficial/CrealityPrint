#ifndef CXKERNEL_UTILS_ALGCACHE_1692424902291_H
#define CXKERNEL_UTILS_ALGCACHE_1692424902291_H
#include "cxkernel/cxkernelinterface.h"
#include "cxkernel/data/header.h"

namespace cxkernel
{
	class AlgCache : public QObject
	{
	public:
		AlgCache(QObject* parent = nullptr);
		virtual ~AlgCache();

		QString createNewAlgCache(const QString& alg, bool removeOld = true);
	};
}

#endif // CXKERNEL_UTILS_ALGCACHE_1692424902291_H