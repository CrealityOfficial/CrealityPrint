#include "localnetworkinterface/devicephase.h"
#include "data/interface.h"
#include <interface/commandinterface.h>
namespace creative_kernel
{
	DevicePhase::DevicePhase(QObject* parent)
		:QObject(parent)
	{

	}

	DevicePhase::~DevicePhase()
	{

	}

	void DevicePhase::onStartPhase()
	{

	}

	void DevicePhase::onStopPhase()
	{

	}
	void DevicePhase::onModelAdded(ModelN* model)
	{
		setKernelPhase(KernelPhaseType::kpt_prepare);
	}
}