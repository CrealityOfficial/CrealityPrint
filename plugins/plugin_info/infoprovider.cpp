#include "infoprovider.h"

#include "kernel/modelnselector.h"
#include "interface/machineinterface.h"
//#include "kernel/spacemanager.h"
#include "interface/selectorinterface.h"
#include "data/modeln.h"
#include "data/modelgroup.h"
#include "data/spaceutils.h"
#include "qtuser3d/trimesh2/conv.h"
#include "interface/appsettinginterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "job/modelrepairob.h"
#include "kernel/kernel.h"
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
	}

	QString InfoProvider::buildTime()
	{
		return "";
	}

	QString InfoProvider::currentPrinter()
	{
		return currentMachineShowName();
	}

	int InfoProvider::selectCount()
	{
		int groupCount = selectedGroupsNum();
		int partCount = selectedPartsNum();
		return groupCount ? groupCount : partCount;
	}

	int InfoProvider::layerHeight()
	{
		return m_layerCount;
	}

	bool InfoProvider::isNeedRepair()
	{
		bool needRepair = false;
		QList<creative_kernel::ModelN*> models = selectionms();
		for (creative_kernel::ModelN* model : models)
		{
			if (model->needRepair())
			{
				needRepair = true;
				break;
			}
		}
		return needRepair;
	}

	void InfoProvider::repairModel()
	{
		creative_kernel::ModelRepairWinJob* job = new creative_kernel::ModelRepairWinJob();

		cxkernel::executeJob(qtuser_core::JobPtr(job));
		disconnect(getKernel()->jobExecutor(), SIGNAL(jobsEnd()), this, SIGNAL(repairSuccess()));
		connect(getKernel()->jobExecutor(), SIGNAL(jobsEnd()), this, SIGNAL(repairSuccess()));

		connect(job, &ModelRepairWinJob::repairSuccessed, this, &InfoProvider::repairSuccess);
	}

	void InfoProvider::onSelectionsChanged()
	{
		emit infoChanged();
	}

	void InfoProvider::onBoxChanged(const trimesh::dbox3& box)
	{
		emit infoChanged();
	}

	void InfoProvider::onSceneChanged(const trimesh::dbox3& box)
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
		QList<ModelGroup*> groups = selectedGroups();
		if (!groups.isEmpty())
		{
			for (ModelGroup* group : groups)
				name += group->groupName() + QString(" ");
		}
		if (!name.isEmpty())
			return name;

		QList<ModelN*> parts = selectedParts();
		if (!parts.isEmpty())
		{
			for (ModelN* part : parts)
				name += part->name() + QString(" ");
		}

		return name;
	}

	QVector3D InfoProvider::modelSize()
	{
		trimesh::dbox3 box;
		QList<ModelGroup*> groups = selectedGroups();
		if (!groups.isEmpty())
		{
			box = creative_kernel::groupsGlobalBoundingBox(groups);
		}
		else {
			box = creative_kernel::modelsBox(selectedParts());
		}

		trimesh::dvec3 size = box.size();
		return QVector3D(size.x, size.y, size.z);
	}
}