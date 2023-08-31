#include "modelspace.h"

#include "data/modeln.h"
#include "data/fdmsupportgroup.h"
#include "interface/modelinterface.h"

#include "internal/render/printerentity.h"
#include "qtusercore/string/resourcesfinder.h"

#include "stringutil/util.h"
#include "internal/menu/submenurecentfiles.h"

#include "interface/reuseableinterface.h"
#include "interface/spaceinterface.h"
#include "interface/uiinterface.h"
#include <QtCore>

using namespace qtuser_3d;
namespace creative_kernel
{
	ModelSpace::ModelSpace(QObject* parent)
		:QObject(parent)
		, m_root(nullptr)
		, m_currentModelGroup(nullptr)
		, m_spaceDirty(true)
	{
		m_baseBoundingBox += QVector3D(0.0f, 0.0f, 0.0f);
		m_baseBoundingBox += QVector3D(300.0f, 300.0f, 300.0f);

		m_root = new Qt3DCore::QEntity();
	}

	ModelSpace::~ModelSpace()
	{
		delete m_root;
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
		//判断模型是否在超出平台大小
		QVector3D bbOffset(0.1f, 0.1f, 0.1f);
		Box3D basebox = baseBoundingBox();
		basebox.min = basebox.min - bbOffset;
		basebox.max = basebox.max + bbOffset;
		QList<ModelN*> models = modelns();
		bool  bleft = false;
		bool  bfront = false;
		bool  bdown = false;
		bool  bright = false;
		bool  bback = false;
		bool  bup = false;
		int result = 0;
		for (size_t i = 0; i < models.size(); i++)
		{
			ModelN* model = models.at(i);
			qtuser_3d::Box3D LocalBox = model->globalSpaceBox();
			if (LocalBox.min.x() < basebox.min.x() ||
				LocalBox.min.y() < basebox.min.y() ||
				LocalBox.min.z() + 0.01 < basebox.min.z() ||
				LocalBox.max.x() > basebox.max.x() ||
				LocalBox.max.y() > basebox.max.y() ||
				LocalBox.max.z() > basebox.max.z())
			{
				model->setErrorState(true);
			}
			else
			{
				model->setErrorState(false);
			}

			//显示面
			if (LocalBox.min.x() < basebox.min.x())
			{
				bleft = true;
				result |= 1;
			}
			if (LocalBox.min.y() < basebox.min.y())
			{
				bfront = true;
				result |= 2;
			}
			if (LocalBox.min.z() + 0.01 < basebox.min.z())
			{
				bdown = true;
				result |= 4;
			}
			if (LocalBox.max.x() > basebox.max.x())
			{
				bright = true;
				result |= 8;
			}
			if (LocalBox.max.y() > basebox.max.y())
			{
				bback = true;
				result |= 16;
			}
			if (LocalBox.max.z() > basebox.max.z())
			{
				bup = true;
				result |= 32;
			}
		}
		PrinterEntity* entity = getCachedPrinterEntity();
		entity->onModelChanged(basebox, creative_kernel::haveModelN(), bleft, bright, bfront, bback, bup, bdown);
		return result;
	}

	int ModelSpace::checkBedRange()
	{
		QList<ModelN*> models = modelns();
		QList<Box3D> boxes;
		for (ModelN* amodel :models)
		{
			boxes.append(amodel->globalSpaceBox());
		}

		PrinterEntity* entity = getCachedPrinterEntity();
		entity->onCheckBed(boxes);
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
		uniformName(model);
		if (m_currentModelGroup && model)
		{
			m_currentModelGroup->addModel(model);
			emit modelNNumChanged();
		}
		emit sigAddModel();
	}

	void ModelSpace::removeModel(ModelN* model)
	{
		if (m_currentModelGroup && model)
		{
			m_currentModelGroup->removeModel(model);
			emit modelNNumChanged();
		}
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
		QList<ModelN*> models = modelns();
		ModelN* model = models[0];
		QFileInfo file(model->objectName());
		return file.baseName();
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
		constexpr auto ITEM_NAME_KEY{ "item_name" };
		constexpr auto ITEM_INDEX_KEY{ "item_index" };

		QString item_name = item->objectName();
		uint32_t item_index{ 0 };

		for (const auto& old_item : items) {
			const auto OLD_ITEM_NAME = old_item->property(ITEM_NAME_KEY).toString();
			const auto OLD_ITEM_INDEX = old_item->property(ITEM_INDEX_KEY).toInt();

			if (OLD_ITEM_NAME == item_name) {
				item_index = std::max<uint32_t>(OLD_ITEM_INDEX, item_index);
				item_index++;
			}
		}

		item->setProperty(ITEM_INDEX_KEY, item_index);
		item->setProperty(ITEM_NAME_KEY, item_name);

		if (item_index != 0) {
			const auto INSERT_INDEX = item_name.lastIndexOf(QStringLiteral("."));
			const auto INDEX_TEXT = QStringLiteral("#%1").arg(QString::number(item_index));
			item->setObjectName(item_name.insert(INSERT_INDEX, INDEX_TEXT));
		}
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

		//这里的条件加上体积判断
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
			addModelLayout(model);
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
			model->setParent(nullptr);
			addModelLayout(model);
		}

		m_needResizeModels.clear();
	}

	void ModelSpace::clearTempModels()
	{
		qDeleteAll(m_needResizeModels);
		m_needResizeModels.clear();
	}

	void ModelSpace::ignoreTempModels()
	{
		for (ModelN* model : m_needResizeModels)
		{
			model->setParent(nullptr);
			addModelLayout(model);
		}

		m_needResizeModels.clear();
	}

	bool ModelSpace::haveModelsOutPlatform()
	{
		QVector3D bbOffset(0.1f, 0.1f, 0.1f);
		Box3D baseBox = baseBoundingBox();
		baseBox.min = baseBox.min - bbOffset;
		baseBox.max = baseBox.max + bbOffset;
		QList<ModelN*> models = modelns();

		bool outPlatform = false;
		for (ModelN* model : models)
		{
			qtuser_3d::Box3D LocalBox = model->globalSpaceBox();
			if (!baseBox.contains(LocalBox))
			{
				outPlatform = true;
				break;
			}
		}

		return outPlatform;
	}

	bool ModelSpace::haveSupports(const QList<ModelN*>& models)
	{
		for (ModelN* model : models)
		{
			if (model->hasFDMSupport())
			{
				creative_kernel::FDMSupportGroup* p_support = model->fdmSupport();
				if (p_support->fdmSupportNum())
				{
					return true;
				}
			}
		}

		return false;
	}

	void ModelSpace::deleteSupports(QList<ModelN*>& models)
	{
		for (ModelN* model : models)
		{
			if (model->hasFDMSupport())
			{
				creative_kernel::FDMSupportGroup* p_support = model->fdmSupport();
				p_support->clearSupports();
			}
		}
	}
}
