#include "messageinterface.h"
#include "kernel/kernel.h"
#include "internal/utils/messagenotify.h"

namespace creative_kernel
{
	void invokeBriefMessage(MessageObject* object)
	{
		getKernel()->messageNotify()->invokeBriefMessage(object);
	}
}