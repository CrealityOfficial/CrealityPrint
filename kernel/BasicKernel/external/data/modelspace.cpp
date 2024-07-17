#include "modelspace.h"

#include "data/modeln.h"
#include "interface/modelinterface.h"
#include "qtusercore/module/systemutil.h"

#include "internal/render/printerentity.h"
#include "qtusercore/string/resourcesfinder.h"

#include "internal/menu/submenurecentfiles.h"

#include "interface/reuseableinterface.h"
#include "interface/spaceinterface.h"
#include "interface/uiinterface.h"
#include "interface/printerinterface.h"
#include "interface/camerainterface.h"
#include "interface/layoutinterface.h"
#include "interface/machineinterface.h"
#include "interface/renderinterface.h"
#include "interface/selectorinterface.h"

#include "qtuser3d/camera/cameracontroller.h"

#include "internal/multi_printer/printermanager.h"
#include "internal/data/_modelinterface.h"
#include "utils/modelpositioninitializer.h"

#include <QtCore>

static constexpr double EPSILON = 1e-3;

using namespace qtuser_3d;
namespace creative_kernel
{
	ModelSpace::ModelSpace(QObject* parent)
		:QObject(parent)
		, m_root(nullptr)
		, m_currentModelGroup(nullptr)
		, m_spaceDirty(true)
		, m_loadMeshCount(0)
		, m_layoutDoneFlag(false)
	{
		m_baseBoundingBox += QVector3D(0.0f, 0.0f, 0.0f);
		m_baseBoundingBox += QVector3D(300.0f, 300.0f, 300.0f);

		m_root = new Qt3DCore::QEntity();
	}

	ModelSpace::~ModelSpace()
	{
#ifndef Q515
		delete m_root;
#endif
	}

	void ModelSpace::initialize()
	{
		addModelGroup(new ModelGroup());
	}

	void ModelSpace::uninitialize()
	{
		m_root->setParent((Qt3DCore::QNode*)nullptr);
		QList<Qt3DCore::QEntity*> entities = m_root->findChildren<Qt3DCore::QEntity*>(QString(), Qt::FindDirectChildrenOnly);
		for(Qt3DCore::QEntity* entity : entities)
			entity->setParent((Qt3DCore::QNode*)nullptr);
	}

	void ModelSpace::addSpaceTracer(SpaceTracer* tracer)
	{
		if (tracer)
		{
			tracer->onBoxChanged(m_baseBoundingBox);
			m_spaceTracers.push_back(tracer);
		}
	}

	void ModelSpace::removeSpaceTracer(SpaceTracer* tracer)
	{
		if(tracer)
			m_spaceTracers.removeAt(m_spaceTracers.indexOf(tracer));
	}

	qtuser_3d::Box3D ModelSpace::baseBoundingBox()
	{
		return m_baseBoundingBox;
	}

	qtuser_3d::Box3D ModelSpace::sceneBoundingBox()
	{
		qtuser_3d::Box3D box;
		box += QVector3D(0.0f, 0.0f, 0.0f);

		QList<ModelN*> models = modelns();
		for (ModelN* m : models)
		{
			box += m->globalSpaceBox();
		}
		return box;
	}

	void ModelSpace::setBaseBoundingBox(const qtuser_3d::Box3D& box)
	{
		if (m_baseBoundingBox == box) return;

		m_baseBoundingBox = box;
		for (SpaceTracer* tracer : m_spaceTracers)
			tracer->onBoxChanged(m_baseBoundingBox);
	}

	void ModelSpace::onModelSceneChanged()
	{
		setModelEffectClipMaxZ(10000.0);
		qtuser_3d::Box3D box = sceneBoundingBox();
		for (SpaceTracer* tracer : m_spaceTracers)
			tracer->onSceneChanged(box);
	}

	void ModelSpace::checkCollide()
	{
	}

	int ModelSpace::checkModelRange()
	{
		return 0;
	}

	int ModelSpace::checkBedRange()
	{
		return true;
	}

	void ModelSpace::addModelGroup(ModelGroup* modelGroup)
	{
		modelGroup->setParent(this);
		modelGroup->setParent2Global(QMatrix4x4());
		modelGroup->setChildrenVisibility(true);
		m_currentModelGroup = modelGroup;
	}

	void ModelSpace::removeModelGroup(ModelGroup* modelGroup)
	{
		modelGroup->setParent(nullptr);
		modelGroup->setChildrenVisibility(false);
		if (m_currentModelGroup == modelGroup)
			m_currentModelGroup = nullptr;
	}

	void ModelSpace::addModel(ModelN* model)
	{
		for (SpaceTracer* tracer : m_spaceTracers)
			tracer->onModelToAdded(model);

		uniformName(model);
		if (m_currentModelGroup && model)
		{
			m_currentModelGroup->addModel(model);
			emit modelNNumChanged();
		}

		for (SpaceTracer* tracer : m_spaceTracers)
			tracer->onModelAdded(model);
		emit sigAddModel();
	}

	void ModelSpace::removeModel(ModelN* model)
	{
		for (SpaceTracer* tracer : m_spaceTracers)
			tracer->onModelToRemoved(model);

		if (m_currentModelGroup && model)
		{
			m_currentModelGroup->removeModel(model);
			emit modelNNumChanged();
		}

		for (SpaceTracer* tracer : m_spaceTracers)
			tracer->onModelRemoved(model);
		emit sigRemoveModel();
	}

	QList<ModelN*> ModelSpace::modelns()
	{
		QList<ModelN*> models;
		QList<ModelGroup*> groups = modelGroups();
		for(ModelGroup* group : groups)
		{
			models << group->models();
		}
		return models;
	}

	QString ModelSpace::mainModelName()
	{
		if (!haveModelN())
		{
			return QString();
		}

		QList<ModelN*> models = selectionms();
		if (models.isEmpty())
			models = modelns();
		
		ModelN* model = models[0];
		QFileInfo file(model->objectName());
		return file.completeBaseName();
	}

	int ModelSpace::modelNNum()
	{
		return modelns().size();
	}

	bool ModelSpace::haveModelN()
	{
		return modelns().size() > 0;
	}

	void ModelSpace::uniformName(ModelN* item)
	{
		if (!item)
			return;

		QList<ModelN*> items = modelns();
		bool hasSame = false;
		for (auto itemData : items)
		{
			if (itemData->objectName() == item->objectName())
			{
				hasSame = true;
				break;
			}
		}

		if (!hasSame)
			return;
		
		QString objectName = item->objectName();
		QFileInfo file(objectName);
		QString suffix = file.suffix().isEmpty() ? "" : "." + file.suffix();
		QString baseName = file.baseName();

		item->setProperty("baseName", objectName);
		item->setProperty("baseNameID", objectName);

		int nameIndex = 1;
		QString name = QString("%1-%2").arg(baseName).arg(nameIndex) + suffix;
		//---                
		QList<ModelN*> models = modelns();
		QString sname;
		for (int k = 0; k < models.size(); ++k)
		{
			sname = models[k]->objectName();
			if (name == sname)
			{
				nameIndex++;
				name = QString("%1-%2").arg(baseName).arg(nameIndex) + suffix;
				k = -1;
			}
		}
		//---
		item->setObjectName(name);

	}

	void ModelSpace::process(cxkernel::ModelNDataPtr data)
	{
		if (data && !data->input.fileName.isEmpty())
		{
			QString fileName = data->input.fileName;
			QString shortName = fileName;
			QStringList stringList = fileName.split("/");
			if (stringList.size() > 0) {
				shortName = stringList.back();
			}
			data->input.name = shortName;

			QFile file(fileName);
			if (file.exists())
			{
				SubMenuRecentFiles::getInstance()->setMostRecentFile(fileName);
			}
		}

		creative_kernel::ModelN* model = new creative_kernel::ModelN();
		model->setData(data);

		qtuser_3d::Box3D baseBox = baseBoundingBox();
		qtuser_3d::Box3D modelBox = model->globalSpaceBox();

		//�����������������ж�
		static const float volume = 0.008;
		if (modelBox.size().x() > baseBox.size().x() ||
			modelBox.size().y() > baseBox.size().y() ||
			modelBox.size().z() > baseBox.size().z())
		{
			appendResizeModel(model);
			requestQmlDialog("idModelUnfitMessageDlg");
		}
		else if (modelBox.size().x() * modelBox.size().x() * modelBox.size().x() < volume)
		{
			model->setUnitType(creative_kernel::ModelN::UT_NOTMETRIC);
			appendResizeModel(model);
			requestQmlDialog("idModelUnfitMessageDlg_small");
		}
		else
		{
			layoutAddedModel(model);
		}
	}

	Qt3DCore::QEntity* ModelSpace::rootEntity()
	{
		return m_root;
	}

	ModelGroup* ModelSpace::currentGroup()
	{
		return m_currentModelGroup;
	}

	QList<ModelGroup*> ModelSpace::modelGroups()
	{
		QList<ModelGroup*> groups = findChildren<ModelGroup*>(QString(), Qt::FindDirectChildrenOnly);
		return groups;
	}

	void ModelSpace::setSpaceDirty(bool _spaceDirty)
	{
		m_spaceDirty = _spaceDirty;
	}

	bool ModelSpace::spaceDirty()
	{
		return m_spaceDirty;
	}

	void ModelSpace::appendResizeModel(ModelN* model)
	{
		if (model)
		{
			model->setParent(this);
			m_needResizeModels.append(model);
		}
	}

	void ModelSpace::adaptTempModels()
	{
		for (ModelN* model : m_needResizeModels)
		{
			if (model->unitType() == creative_kernel::ModelN::UT_NOTMETRIC)
			{
				model->adaptSmallBox(baseBoundingBox());
			}
			else
			{
				model->adaptBox(baseBoundingBox());
			}

			if (checkModelSizeForLayout(model))
			{
				if (1 == m_loadMeshCount)
				{
					creative_kernel::addModel(model, true);
					QVector3D newPos = getSelectedPrinter()->position();
					qtuser_3d::Box3D printerBox = getSelectedPrinter()->globalBox();
					float sizeX = printerBox.size().x();
					float sizeY = printerBox.size().y();
					newPos.setX(newPos.x() + sizeX / 2.0f);
					newPos.setY(newPos.y() + sizeY / 2.0f);
					_setModelPosition(model, newPos, true);
					bottomModel(model);
				}
				else
				{
					model->setParent(nullptr);
					layoutAddedModel(model);
				}

			}

		}

		m_needResizeModels.clear();

		// the "click event" could happen in the middle of loading  or  after all mesh loading
		reCheckLayoutWhenLoadingFinish();

	}

	void ModelSpace::clearTempModels()
	{
		qDeleteAll(m_needResizeModels);
		m_needResizeModels.clear();
	}

	void ModelSpace::ignoreTempModels()
	{
		qtuser_3d::Box3D baseBox = baseBoundingBox();

		for (ModelN* model : m_needResizeModels)
		{
			if (checkModelSizeForLayout(model))
			{
				model->setParent(nullptr);
				layoutAddedModel(model);
			}
			else
			{
				m_loadMeshCount -= 1;
			}

		}

		m_needResizeModels.clear();

		reCheckLayoutWhenLoadingFinish();

	}

	bool ModelSpace::haveModelsOutPlatform()
	{
		bool onPlatformBorder = false;
		for (int i = 0, count = creative_kernel::printersCount(); i < count; ++i)
		{
			auto p = creative_kernel::getPrinter(i);
			if (p->hasModelsOnBorder())
			{
				auto modelsOnBorder = p->modelsOnBorder();
				float minX, minY, minZ, maxX, maxY, maxZ;
				minX = minY = minZ = 100000;
				maxX = maxY = maxZ = -100000;
				for (auto m : modelsOnBorder)
				{
					// 计算预览区域
					auto box = m->boxWithSup();
					minX = box.min.x() < minX ? box.min.x() : minX;
					minY = box.min.y() < minY ? box.min.y() : minY;
					minZ = box.min.z() < minZ ? box.min.z() : minZ;
					maxX = box.max.x() > maxX ? box.max.x() : maxX;
					maxY = box.max.y() > maxY ? box.max.y() : maxY;
					maxZ = box.max.z() > maxZ ? box.max.z() : maxZ;
				}
				qtuser_3d::Box3D box(QVector3D(minX, minY, minZ), QVector3D(maxX, maxY, maxZ));
				QVector3D dir(0.0f, 1.0f, -1.0f);
				QVector3D right(1.0f, 0.0f, 0.0f);
				QVector3D center = box.center();
				center.setZ(0);
				creative_kernel::cameraController()->view(dir, right, &center);
				onPlatformBorder = true;
			}
		}

		return onPlatformBorder;
	}

	bool ModelSpace::modelOutPlatform(ModelN* amode)
	{
		QVector3D bbOffset(0.1f, 0.1f, 0.1f);
		Box3D baseBox = baseBoundingBox();
		baseBox.min = baseBox.min - bbOffset;
		baseBox.max = baseBox.max + bbOffset;

		qtuser_3d::Box3D LocalBox = amode->globalSpaceBox();
		LocalBox.min.setZ(0.0);
		LocalBox.max.setZ(0.0);
		bool outPlatform = false;
		if (!baseBox.contains(LocalBox))
		{
			outPlatform = true;
		}
		return outPlatform;
	}

	QList<ModelN*> ModelSpace::getModelnsBySerialName(const QStringList& names)
	{
		QList<ModelN*> theModels;

		for (QString sName : names)
		{
			ModelN* model = getModelNBySerialName(sName);
			if (model)
				theModels.append(model);
		}

		return theModels;
	}

	ModelN* ModelSpace::getModelNBySerialName(const QString& name)
	{
		ModelN* model = nullptr;
		QList<ModelN*> ms = modelns();
		for (ModelN* m : ms)
		{
			if (name == m->getSerialName())
			{
				model = m;
				break;
			}
		}

		if (!model)
		{
			qDebug() << QString("getModelNBySerialName error. [%1]").arg(name);
		}
		return model;
	}

	ModelN* ModelSpace::getModelNByObjectName(const QString& objectName)
	{
		ModelN* model = nullptr;
		QList<ModelN*> ms = modelns();
		for (ModelN* m : ms)
		{
			if (objectName == m->objectName())
			{
				model = m;
				break;
			}
		}

		if (!model)
		{
			qDebug() << QString("getModelNByObjectName error. [%1]").arg(objectName);
		}
		return model;
	}

	void ModelSpace::modelMeshLoadStarted(int iNum)
	{
		setContinousRender();
		m_loadedModels.clear();
		m_loadMeshCount = iNum;
		m_layoutDoneFlag = false;
	}

	void ModelSpace::onMeshLoadFail()
	{
		m_loadMeshCount -= 1;
	}

	void ModelSpace::layoutAddedModel(ModelN* aModel)
	{
		if (!aModel)
			return;

		if (currentMachineIsBelt())
		{
			ModelPositionInitializer::layoutBelt(aModel, nullptr);
			bottomModel(aModel);
			creative_kernel::addModel(aModel, true);
			aModel->updateMatrix();
			aModel->dirty();

			m_loadMeshCount -= 1;
		}
		else
		{
			m_loadedModels.append(aModel);
			if (m_loadMeshCount == m_loadedModels.count())
			{
				creative_kernel::layoutModels(m_loadedModels, currentPrinterIndex(), false);
				m_layoutDoneFlag = true;
			}
		}
	}

	bool ModelSpace::checkModelSizeForLayout(ModelN* aModel)
	{
		if (!aModel)
			return false;

		qtuser_3d::Box3D baseBox = baseBoundingBox();

		qtuser_3d::Box3D modelBox = aModel->globalSpaceBox();

		// very big model DO NOT need to do layout algorithm
		if (modelBox.size().x() > baseBox.size().x() ||
			modelBox.size().y() > baseBox.size().y() ||
			modelBox.size().z() > baseBox.size().z())
		{
			creative_kernel::addModel(aModel, true);
			QVector3D newPos = getPrinter(0)->position();
			newPos.setX(newPos.x() - modelBox.size().x() / 2);
			newPos.setY(newPos.y() + modelBox.size().y() / 2);
			_setModelPosition(aModel, newPos, true);
			bottomModel(aModel);

			return false;
		}
		else
		{
			return true;
		}

	}

	void ModelSpace::reCheckLayoutWhenLoadingFinish()
	{
		if (!m_layoutDoneFlag && m_loadedModels.size() > 0 && m_loadMeshCount == m_loadedModels.size())
		{
			creative_kernel::layoutModels(m_loadedModels, currentPrinterIndex(), false);
		}
	}

	bool ModelSpace::checkLayerHeightEqual(const std::vector<std::vector<double>>& objectsLayers)
	{
		/* 全部固定层高 */
		if (std::all_of(objectsLayers.begin(), objectsLayers.end(),
			[](const std::vector<double>& layer) {
				return layer.empty();
			})) {
			return true;
		}

		/* 自适应层高和固定层高同时存在，返回false */
		if (std::any_of(objectsLayers.begin(), objectsLayers.end(), [](const std::vector<double>& layer) {
			return layer.empty();
			})) {
			return false;
		}

		auto areClose = [](double a, double b) {
			return std::abs(a - b) <= EPSILON;
			};

		/* 获取最大的层数 */
		auto maxIt = std::max_element(objectsLayers.begin(), objectsLayers.end(),
			[](const std::vector<double>& a, const std::vector<double>& b) {
				return a.size() < b.size();
			});
		size_t maxLayerSize = (*maxIt).size();
		size_t maxLayerIndex = std::distance(objectsLayers.begin(), maxIt);

		/* 逐层比较层高, 如果不相等则返回false */
		for (size_t heightIdx = 0; heightIdx < maxLayerSize; heightIdx++) {
			const double layerHeight = (*maxIt)[heightIdx];
			for (size_t layerIdx = 0; layerIdx < objectsLayers.size(); layerIdx++) {
				if (layerIdx != maxLayerIndex && heightIdx < objectsLayers[layerIdx].size() &&
					!areClose(layerHeight, objectsLayers[layerIdx][heightIdx])) {
					return false;
				}
			}
		}
		return true;
	}
}
