#ifndef CREATIVE_KERNEL_PRINTERENTITYUPDATER_1594459630592_H
#define CREATIVE_KERNEL_PRINTERENTITYUPDATER_1594459630592_H
#include "basickernelexport.h"
#include "data/interface.h"
#include "qtuser3d/camera/screencamera.h"
#include "qtuser3d/module/pickableselecttracer.h"

namespace creative_kernel
{
	class BASIC_KERNEL_API PrinterNotifier : public QObject
		, public SpaceTracer
		, public qtuser_3d::ScreenCameraObserver
		, public qtuser_3d::SelectorTracer
	{
		Q_OBJECT
	public:
		PrinterNotifier(QObject* parent = nullptr);
		virtual ~PrinterNotifier();

	protected:
		void onBoxChanged(const qtuser_3d::Box3D& box) override;
        void onSceneChanged(const qtuser_3d::Box3D &box) override {}
		void onCameraChanged(qtuser_3d::ScreenCamera* camera) override;

		void onSelectionsChanged() override;
		void selectChanged(qtuser_3d::Pickable* pickable) {};
	};
}
#endif // CREATIVE_KERNEL_PRINTERENTITYUPDATER_1594459630592_H
