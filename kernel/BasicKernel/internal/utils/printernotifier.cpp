#include "printernotifier.h"

#include "interface/reuseableinterface.h"
#include "interface/renderinterface.h"
#include "interface/camerainterface.h"
#include "interface/spaceinterface.h"
#include "interface/selectorinterface.h"

#include "internal/render/printerentity.h"

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
		visCamera()->fittingBoundingBox(box);

		QVector3D machineCube = box.size();

		PrinterEntity* entity = getCachedPrinterEntity();
		entity->updateBox(box);

		renderOneFrame();
	}

	void PrinterNotifier::onCameraChanged(qtuser_3d::ScreenCamera* screenCamera)
	{
		PrinterEntity* entity = getCachedPrinterEntity();

		qtuser_3d::Box3D bbox = creative_kernel::baseBoundingBox();
		Qt3DRender::QCamera* camera = screenCamera->camera();

		QVector3D cameraPosition = camera->position();
		QVector3D viewCenter = camera->viewCenter();
		QVector3D viewUp = camera->upVector();

		bool enable = cameraPosition.z() > bbox.min.z();
		entity->enableSkirt(enable);

		//grid
		{
			bool visible = false;
			if (cameraPosition.z() < bbox.min.z())
			{
				visible = qAbs(cameraPosition.z() - bbox.min.z()) < 250.0f;
			}
			else
			{
				visible = (qAbs(cameraPosition.z() - bbox.min.z()) < 250.0f) || ((cameraPosition - viewCenter).length() < 100.0f);
			}

			entity->visibleSubGrid(true);
		}

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
		QList<ModelN*> selections = selectionms();
		PrinterEntity* entity = getCachedPrinterEntity();
		entity->setSkirtHighlight(selections.size() > 0);
	}
}