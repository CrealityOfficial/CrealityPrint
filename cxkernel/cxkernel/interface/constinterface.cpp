#include "constinterface.h"
#include "qtusercore/util/applicationconfiguration.h"
#include "cxkernel/kernel/cxkernel.h"
#include "cxkernel/kernel/const.h"

namespace cxkernel
{
	QString version()
	{
		return cxKernel->cxConst()->version();
	}

	bool isReleaseVersion()
	{
		return cxKernel->cxConst()->isReleaseVersion();
	}

	QString writableLocation(const QString& subDir, const QString& subSubDir)
	{
		return cxKernel->cxConst()->writableLocation(subDir, subSubDir);
	}
}