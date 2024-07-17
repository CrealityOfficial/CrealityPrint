#ifndef CREATIVE_KERNEL_DEVICEPHASE_1672888329478_H
#define CREATIVE_KERNEL_DEVICEPHASE_1672888329478_H
#include "data/interface.h"

namespace creative_kernel
{
	class DevicePhase : public QObject,
		public KernelPhase, public SpaceTracer
	{
		Q_OBJECT
	public:
		DevicePhase(QObject* parent = nullptr);
		virtual ~DevicePhase();

	protected:
		void onStartPhase() override;
		void onStopPhase() override;

		void onBoxChanged(const qtuser_3d::Box3D& box) {};
		void onSceneChanged(const qtuser_3d::Box3D& box) {};

		void onModelAdded(ModelN* model) override;
	};
}

#endif // CREATIVE_KERNEL_COMMANDCENTER_1672888329478_H