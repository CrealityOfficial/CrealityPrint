#include "deviceutil.h"
#include "cxkernel/utils/glcompatibility.h"

namespace cxkernel
{
	DeviceUtil::DeviceUtil(QObject* parent)
		: QObject(parent)
	{

	}

	DeviceUtil::~DeviceUtil()
	{

	}

	bool DeviceUtil::isDesktopGL()
	{
		return !isGles() && !isSoftwareGL();
	}

	bool DeviceUtil::isGles()
	{
		return cxkernel::isGles();
	}

	bool DeviceUtil::isSoftwareGL()
	{
		return cxkernel::isSoftwareGL();
	}
}