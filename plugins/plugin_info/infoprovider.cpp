#include "infoprovider.h"

#include "kernel/modelnselector.h"
#include "interface/machineinterface.h"
//#include "kernel/spacemanager.h"
#include "interface/selectorinterface.h"
#include "data/modeln.h"
#include "qtuser3d/trimesh2/conv.h"
#include "interface/appsettinginterface.h"
namespace creative_kernel
{
	InfoProvider::InfoProvider(QObject* parent)
		:QObject(parent)
	{
	}

	InfoProvider::~InfoProvider()
	{
	}

	QString InfoProvider::softVersion()
	{
		return version();
		//return gContext->buildInfo->getCurrentVersion();
	}

	QString InfoProvider::buildTime()
	{
		return "";//return gContext->buildInfo->getBuildDate();
	}

	QString InfoProvider::currentPrinter()
	{
		return currentMachineShowName();
		//return gContext->printerManager->currentPrinterName();
	}

	int InfoProvider::selectCount()
	{
		return selectionms().size();
	}

	int InfoProvider::layerHeight()
	{
		return m_layerCount;
	}

	void InfoProvider::onSelectionsChanged()
	{
		emit infoChanged();
	}

	void InfoProvider::selectChanged(qtuser_3d::Pickable* pickable)
	{
	}

	void InfoProvider::onBoxChanged(const qtuser_3d::Box3D& box)
	{
		emit infoChanged();
	}

	void InfoProvider::onSceneChanged(const qtuser_3d::Box3D& box)
	{
		//float maxZ = box.max.z;
		//creative_kernel::CommonSettingPtr setting = gContext->printerManager->createCurrentSetting();
		//float thickness = setting->floatParam("layer_height");
		//maxZ = ceil(maxZ * 10) / 10.0;
		//m_layerCount = (ceil)(maxZ / thickness);
		emit infoChanged();
	}

	void InfoProvider::updateMachineInfo()
	{
		emit infoChanged();
	}

	QString InfoProvider::modelName()
	{
		QString name;
		QList<creative_kernel::ModelN*> models =selectionms();
		for (creative_kernel::ModelN* model : models)
			name += model->objectName() + QString(" ");
		return name;
	}

	QVector3D InfoProvider::modelSize()
	{
		qtuser_3d::Box3D box;
		QList<creative_kernel::ModelN*> models = selectionms();
		for (creative_kernel::ModelN* model : models)
			box += model->globalSpaceBox();

		return box.size();
	}
}