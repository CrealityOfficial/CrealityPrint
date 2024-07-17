#include "printernotifier.h"

#include "interface/reuseableinterface.h"
#include "interface/renderinterface.h"
#include "interface/camerainterface.h"
#include "interface/spaceinterface.h"
#include "interface/selectorinterface.h"

#include "internal/render/printerentity.h"
#include "internal/multi_printer/printermanager.h"

namespace creative_kernel
{
	PrinterNotifier::PrinterNotifier(QObject* parent)
		: QObject(parent)
	{
	}
	
	PrinterNotifier::~PrinterNotifier()
	{
	}

	void PrinterNotifier::onBoxChanged(const qtuser_3d::Box3D& box)
	{
		getPrinterMangager()->allPrinterUpdateBox(box);

	//	cameraController()->setRotateCenter(visCamera()->orignCenter());

		visCamera()->fittingBoundingBox(getPrinterMangager()->boundingBox());

		renderOneFrame();
	}

	void PrinterNotifier::onCameraChanged(qtuser_3d::ScreenCamera* screenCamera)
	{
		Qt3DRender::QCamera* camera = screenCamera->camera();
		if (camera)
		{
			QVector3D viewDir = camera->viewVector();
			viewDir.normalize();
			QVector3D up = QVector3D(0.0f, 0.0f, 1.0f);
			float a = 0.5f - 0.625f * QVector3D::dotProduct(viewDir, up);
			if (a > 0.5f)
				a = 0.5f;
			else if (a < 0.0f)
				a = 0.0f;

			QVector4D color = QVector4D(0.0f, 0.0f, 0.0f, a);
			setModelZProjectColor(color);
		}
	}

	void PrinterNotifier::onSelectionsChanged()
	{
	}
}