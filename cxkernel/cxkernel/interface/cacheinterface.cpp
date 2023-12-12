#include "cacheinterface.h"
#include "cxkernel/kernel/cxkernel.h"
#include "cxkernel/utils/algcache.h"

namespace cxkernel
{
	QString createNewAlgCache(const QString& alg, bool removeOld)
	{
		return cxKernel->algCache()->createNewAlgCache(alg, removeOld);
	}
}