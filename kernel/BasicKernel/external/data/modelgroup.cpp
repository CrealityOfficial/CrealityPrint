#include "modelgroup.h"

#include "data/modeln.h"
#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/appsettinginterface.h"
#include "interface/spaceinterface.h"
#include "interface/parameterinterface.h"
#include "interface/machineinterface.h"

#include "us/usettings.h"
#include "qtuser3d/trimesh2/conv.h"
#include "internal/render/modelgroupentity.h"
#include "interface/spaceinterface.h"
#include "qtuser3d/trimesh2/trimeshrender.h"
#include "msbase/space/cloud.h"
#include "internal/render/modelnentity.h"

#include "kernel/kernel.h"
#include "internal/parameter/parametermanager.h"
#include "nestplacer/printpiece.h"
#include "qtuser3d/utils/texturecreator.h"

namespace creative_kernel
{
	ModelGroup::ModelGroup(QObject* parent)
		:QNode3D(parent)
		, m_objectid(-1)
		, m_modelGroupEntity(nullptr)
	{
		m_lastUsedSetting = new us::USettings(this);
		m_setting = new us::USettings(this);

		// init cache
		supportEnabled();
		supportAngle();
		supportStructure();
		supportFilament();
		supportInterfaceFilament();

		connect(m_setting, &us::USettings::settingsChanged, this, &ModelGroup::onSettingsChanged, Qt::UniqueConnection);
		connect(m_setting, &us::USettings::settingValueChanged, this, &ModelGroup::onSettingsChanged, Qt::UniqueConnection);
		connect(this, &ModelGroup::defaultColorIndexChanged, this, &ModelGroup::updateSetting);
		ParameterManager* parameterManager = getKernel()->parameterManager();
		connect(parameterManager, &ParameterManager::parameterEdited, this, &ModelGroup::onGlobalSettingsChanged);
		connect(parameterManager, &ParameterManager::extruderClearanceRadiusChanged, this, [=](float radius)
			{
				m_sweepPathDirty = true;
				updateSweepAreaPath();
			});
	}
	
	ModelGroup::~ModelGroup()
	{
		m_modelGroupEntity = nullptr;
	}

	void ModelGroup::setEntity(ModelGroupEntity* entity)
	{
		m_modelGroupEntity = entity;
	}

	ModelGroupEntity* ModelGroup::entity()
	{
		return m_modelGroupEntity;
	}

	Qt3DCore::QEntity* ModelGroup::qEntity()
	{
		return m_modelGroupEntity;
	}

	void ModelGroup::addModel(ModelN* model)
	{
		if (!model)
			return;

		QList<ModelN*> models;
		models << model;
		return addModels(models);
	}

	void ModelGroup::addModels(const QList<ModelN*>& models)
	{
		for (ModelN* model : models)
		{
			if (m_componentModels.contains(model))
			{
				qDebug() << QString("ModelGroup addModels already in.");
				continue;
			}

			model->onAttachModelGroup(this);

			if (m_componentModels.isEmpty() && m_extruderIndex < 0) // fix: [ID1028416], when loading 3mf, the group default index setting has no effect
			{
				setDefaultColorIndex(model->defaultColorIndex());
			}

			if (model->getPartIndex() >= 0)
			{
				m_componentModels.insert(model->getPartIndex(), model);
			}
			else
			{
				m_componentModels.append(model);
			}

			//crslice2
			if (m_crslice_object)
			{
				model->m_crslice_volume = m_crslice_object->add_volume();
			}

			emit groupNameChanged();

			connect(model, &ModelN::colorsDataChanged, this, &ModelGroup::colorsDataChanged, Qt::UniqueConnection);
			connect(model, &ModelN::seamsDataChanged, this, &ModelGroup::seamsDataChanged, Qt::UniqueConnection);
			connect(model, &ModelN::supportsDataChanged, this, &ModelGroup::supportsDataChanged, Qt::UniqueConnection);
			connect(model, &ModelN::adaptiveLayerDataChanged, this, &ModelGroup::adaptiveLayerDataChanged, Qt::UniqueConnection);
			connect(model, &ModelN::modelNTypeChanged, this, &ModelGroup::onModelNTypeChanged, Qt::UniqueConnection);
			connect(model, &ModelN::dirtyChanged, this, &ModelGroup::onModelNDirtyChanged, Qt::UniqueConnection);		
			connect(model, &ModelN::formModified, this, &ModelGroup::formModified, Qt::UniqueConnection);	
		}

		updateAllPartsIndex();

		dirty();
		colorsDataChanged();
		seamsDataChanged();
		supportsDataChanged();
		adaptiveLayerDataChanged();

		setOutlinePathDirty();
	}

	void ModelGroup::removeModel(ModelN* model)
	{
		if (!model)
			return;

		QList<ModelN*> models;
		return removeModels(models);
	}

	void ModelGroup::removeModels(const QList<ModelN*>& models)
	{
		for (ModelN* model : models)
		{
			if (!m_componentModels.contains(model))
			{
				qDebug() << QString("ModelGroup addModels not in.");
				continue;
			}

			model->setParent(nullptr);
			m_componentModels.removeOne(model);

			//crslice2
			if (m_crslice_object)
			{
				m_crslice_object->remove_volume(model->m_crslice_volume);
			}



			emit groupNameChanged();
			
			disconnect(model, &ModelN::colorsDataChanged, this, &ModelGroup::colorsDataChanged);
			disconnect(model, &ModelN::seamsDataChanged, this, &ModelGroup::seamsDataChanged);
			disconnect(model, &ModelN::supportsDataChanged, this, &ModelGroup::supportsDataChanged);
			disconnect(model, &ModelN::adaptiveLayerDataChanged, this, &ModelGroup::adaptiveLayerDataChanged);
			disconnect(model, &ModelN::modelNTypeChanged, this, &ModelGroup::onModelNTypeChanged);
			disconnect(model, &ModelN::dirtyChanged, this, &ModelGroup::onModelNDirtyChanged);	
			disconnect(model, &ModelN::formModified, this, &ModelGroup::formModified);	
		}

		updateAllPartsIndex();

		dirty();
		colorsDataChanged();
		seamsDataChanged();
		supportsDataChanged();
		adaptiveLayerDataChanged();
		setOutlinePathDirty();
	}

	void ModelGroup::setChildrenVisibility(bool visibility)
	{
		QList<ModelN*> modelns = models();
		for (ModelN* modeln : modelns)
		{
			modeln->setVisibility(visibility);
		}
	}

	void ModelGroup::dirty()
	{
		setDirty(true);
	}

	void ModelGroup::resetDirty()
	{
		setDirty(false);
	}

	void ModelGroup::setDirty(bool dirty)
	{
		for (ModelN* model : m_componentModels)
		{
			if (dirty)
				model->dirty();
			else 
				model->resetDirty();
		}

		if (m_isDirty != dirty)
		{
			m_isDirty = dirty;
			emit dirtyChanged();
		}
	}
	
	bool ModelGroup::isDirty()
	{
		return m_isDirty;
	}

	bool ModelGroup::needInit()
	{
		return false;
	}

	void ModelGroup::setInitPosition(QVector3D QPosition)
	{
	}

	void ModelGroup::setGroupName(const QString& groupName)
	{
		m_groupName = groupName;
		auto models = this->models();
		if (models.size() == 1)
		{
			models[0]->setName(groupName);
		}
		emit groupNameChanged();
	}

	QString ModelGroup::groupName()
	{
		return m_groupName;
	}

	void ModelGroup::setVisible(bool bVisible)
	{
		if(m_modelGroupEntity)
			m_modelGroupEntity->setEnabled(bVisible);
	}

	void ModelGroup::setObjectId(int64_t objId)
	{
		m_objectid = objId;
	}

	int64_t ModelGroup::getObjectId()
	{
		return m_objectid;
	}

	QList<QObject*> ModelGroup::modelObjects(int type)
	{
		QList<QObject*> ms;
		for (ModelN* model : m_componentModels)
		{
			if (model->modelNType() == (ModelNType)type)
				ms << model;
		}
		return ms;
	}

	QList<ModelN*> ModelGroup::models(int type)
	{
		QList<ModelN*> ms;
		for (ModelN* model : m_componentModels)
		{
			if (model->modelNType() == (ModelNType)type)
				ms << model;
		}
		return ms;
	}

	QList<ModelN*> ModelGroup::normalModels()
	{
		return models((int)ModelNType::normal_part);
	}

	QList<ModelN*> ModelGroup::models()
	{
		return m_componentModels;
	}
	
	QList<ModelN*> ModelGroup::selectedModels()
	{
		QList<ModelN*> list;
		for (ModelN* model : m_componentModels)
		{
			if (model->selected())
			{
				list << model;
			}
		}
		return list;
	}

	QList<ModelN*> ModelGroup::unselectedModels()
	{
		QList<ModelN*> list;
		for (ModelN* model : m_componentModels)
		{
			if (!model->selected())
			{
				list << model;
			}
		}
		return list;
	}

	bool ModelGroup::isVisible()
	{
		int visible = true;
		for (auto model : m_componentModels)
		{
			visible = model->isVisible();
			if (!visible)
				break;
		}

		return visible;
	}

	void ModelGroup::setIsVisible(bool isVisible)
	{
		for (auto model : m_componentModels)
		{
			model->setVisibility(isVisible);
		}

		emit isVisibleChanged();
	}

	bool ModelGroup::hasColors()
	{
		for (ModelN* model : m_componentModels)
		{
			if (model->hasColors())
				return true;
		}
		return false;
	}

	bool ModelGroup::hasSeamsData()
	{
		for (ModelN* model : m_componentModels)
		{
			if (model->hasSeamsData())
				return true;
		}
		return false;
	}

	bool ModelGroup::hasSupportsData()
	{
		for (ModelN* model : m_componentModels)
		{
			if (model->hasSupportsData())
				return true;
		}
		return false;
	}

	bool ModelGroup::adaptiveLayerData()
	{
		bool res = m_layerHeightProfile.size() > 0;
		return res;
	}

	int ModelGroup::defaultColorIndex()
	{
		return m_extruderIndex;
	}

	void ModelGroup::setDefaultColorIndex(int index)
	{
		if (index < 0)
			return;

		if (m_extruderIndex == index)
			return;

		for (auto model : m_componentModels)
		{
			model->setDefaultColorIndex(index);
		}

		m_extruderIndex = index;
		emit defaultColorIndexChanged();
	}

	void ModelGroup::discardDefaultColor()
	{
		if (m_extruderIndex == 0)
			return;

		for (auto model : m_componentModels)
		{
			if (model->defaultColorIndex() == m_extruderIndex)
				model->setDefaultColorIndex(0);
		}

		setDefaultColorIndex(0);
		emit defaultColorIndexChanged();
	}

	ModelN* ModelGroup::getModelNByObjectId(int id)
	{
		for (ModelN* model : m_componentModels)
		{
			if (model->getObjectId() == id)
				return model;
		}
		return nullptr;
	}

	ModelN* ModelGroup::getModelNByFixedId(int id)
	{
		if (id < 0)
			return nullptr;

		for (ModelN* model : m_componentModels)
		{
			if (model->getFixedId() == id)
				return model;
		}
		return nullptr;
	}

	bool ModelGroup::flushIntoInfill()
	{
		QString key = "flush_into_infill";
		QString value;
		if (!m_setting->hasKey(key))
		{
			value = getPrintProfileValue(key, "");
		}
		else
		{
			value = m_setting->value(key, "");
		}
		bool enabled = value == "1" ? true : false;
		return enabled;
	}

	void ModelGroup::setFlushIntoInfillEnabled(bool enabled)
	{
		QString key = "flush_into_infill";
		m_setting->add(key, enabled ? "1" : "0", true);
	}

	bool ModelGroup::flushIntoObjects()
	{
		QString key = "flush_into_objects";
		QString value;
		if (!m_setting->hasKey(key))
		{
			value = getPrintProfileValue(key, "");
		}
		else
		{
			value = m_setting->value(key, "");
		}
		bool enabled = value == "1" ? true : false;
		return enabled;
	}

	void ModelGroup::setFlushIntoObjectsEnabled(bool enabled)
	{
		QString key = "flush_into_objects";
		m_setting->add(key, enabled ? "1" : "0", true);
	}

	bool ModelGroup::flushIntoSupport()
	{
		QString key = "flush_into_support";
		QString value;
		if (!m_setting->hasKey(key))
		{
			value = getPrintProfileValue(key, "");
		}
		else
		{
			value = m_setting->value(key, "");
		}
		bool enabled = value == "1" ? true : false;
		return enabled;
	}

	void ModelGroup::setFlushIntoSupportEnabled(bool enabled)
	{
		QString key = "flush_into_support";
		m_setting->add(key, enabled ? "1" : "0", true);
	}

	bool ModelGroup::supportEnabled()
	{
		QString key = "enable_support";
		QString value;
		if (!m_setting->hasKey(key))
		{
			value = getPrintProfileValue(key, "");
		}
		else
		{
			value = m_setting->value(key, "");
		}

		m_supportEnabled = (value == "true" || value == "1") ? true : false;
		return m_supportEnabled;
	}

	bool ModelGroup::cSupportEnabled()
	{
		return m_supportEnabled;
	}

	void ModelGroup::setSupportEnabled(bool enabled)
	{
		QString key = "enable_support";
		m_setting->add(key, enabled ? "1" : "0", true);
	}

	float ModelGroup::supportAngle()
	{
		QString key = "support_threshold_angle";
		QString value;
		if (!m_setting->hasKey(key))
		{
			value = getPrintProfileValue(key, "0");
		}
		else
		{
			value = m_setting->value(key, "0");
		}
		m_supportAngle = value.toFloat();
		return m_supportAngle;
	}

	float ModelGroup::cSupportAngle()
	{
		return m_supportAngle;
	}

	void ModelGroup::setSupportAngle(float angle)
	{
		QString key = "support_threshold_angle";
		QString valueStr = QString::number(angle);
		m_setting->add(key, valueStr, true);
	}

	QString ModelGroup::supportStructure()
	{
		QString key = "support_type";
		QString value;
		if (!m_setting->hasKey(key))
		{
			value = getPrintProfileValue(key, "normal(auto)");
		}
		else
		{
			value = m_setting->value(key, "normal(auto)");
		}

		m_supportStructure = (value == "" ? "normal(auto)" : value);
		return m_supportStructure;
	}

	QString ModelGroup::cSupportStructure()
	{
		return m_supportStructure;
	}

	void ModelGroup::setSupportStructure(QString structure)
	{
		QString key = "support_type";
		m_setting->add(key, structure, true);
	}

	int ModelGroup::supportFilament()
	{
		QString key = "support_filament";
		QString value;
		if (!m_setting->hasKey(key))
		{
			value = getPrintProfileValue(key, "0");
		}
		else
		{
			value = m_setting->value(key, "0");
		}

		m_supportFilament = value.toInt();
		return m_supportFilament;
	}

	int ModelGroup::cSupportFilament()
	{
		return m_supportFilament;
	}

	void ModelGroup::setSupportFilament(int index)
	{
		QString key = "support_filament";
		m_setting->add(key, QString::number(index), true);
	}

	int ModelGroup::supportInterfaceFilament()
	{
		QString key = "support_interface_filament";
		QString value;
		if (!m_setting->hasKey(key))
		{
			value = getPrintProfileValue(key, "0");
		}
		else
		{
			value = m_setting->value(key, "0");
		}

		m_supportInterfaceFilament = value.toInt();
		return m_supportInterfaceFilament;
	}

	int ModelGroup::cSupportInterfaceFilament()
	{
		return m_supportInterfaceFilament;
	}

	void ModelGroup::setSupportInterfaceFilament(int index)
	{
		QString key = "support_interface_filament";
		m_setting->add(key, QString::number(index), true);
	}

	us::USettings* ModelGroup::setting()
	{
		return m_setting;
	}

	bool ModelGroup::checkSettingDirty()
	{
		QSharedPointer<us::USettings> currentUsedSetting(new us::USettings());
		getUsedSetting(currentUsedSetting.get());

		if (*(currentUsedSetting.get()) == *m_lastUsedSetting)
			return false;
		else
			return true;
	}

	bool ModelGroup::checkSettingDirty(const QString& key)
	{
		if (m_setting->hasKey(key))
		{
			if (m_lastUsedSetting->hasKey(key))
			{
				QString value = m_setting->value(key, "");
				QString lastValue = m_lastUsedSetting->value(key, "");
				return value != lastValue;
			}
			else
			{
				return true;
			}
		}
		else
		{
			QString value = getPrintProfileValue(key, "");
			if (value.isEmpty())
			{
				if (m_lastUsedSetting->hasKey(key))
					return true;
				else
					return false;
			}
			else
			{
				if (m_lastUsedSetting->hasKey(key))
				{
					QString lastValue = m_lastUsedSetting->value(key, "");
					return value != lastValue;
				}
				else
					return true;
			}
		}
	}

	bool ModelGroup::hasParameter(const QString& key)
	{
		return m_setting->hasKey(key);
	}

	void ModelGroup::getUsedSetting(us::USettings* setting)
	{
		QSharedPointer<us::USettings> globalSetting = creative_kernel::createCurrentGlobalSettings();
		setting->clear();
		setting->merge(globalSetting.get());
		setting->merge(m_setting);
	}

	void ModelGroup::recordSetting()
	{
		getUsedSetting(m_lastUsedSetting);
	}

	void ModelGroup::updateSetting()
	{
		if (!m_setting)
			return;

		int index = defaultColorIndex() + 1;
		m_setting->add("extruder", QString::number(index), true);
	}

	bool ModelGroup::selected()
	{
		return m_selected;
	}

	void ModelGroup::setSelected(bool selected)
	{
		if (m_selected != selected)
		{
			m_selected = selected;

			/*for (auto model : m_componentModels)
			{
				model->setSelected(selected);
			}*/
			emit selectedChanged();
		}
		
	}

	trimesh::dbox3 ModelGroup::localBoundingBox()
	{
		trimesh::dbox3 box;
		for (ModelN* model : m_componentModels)
		{
			if (model->isVisible())
			{
				box += model->_localBoundingBox();
			}
		}
		return box;
	}

	trimesh::dbox3 ModelGroup::globalBoundingBox()
	{
		trimesh::dbox3 box;
		for (ModelN* model : m_componentModels)
		{
			if (model->isVisible())
			{
				box += model->globalBoundingBox();
			}
		}
		return box;
	}

	qtuser_3d::Box3D ModelGroup::globalBox()
	{
		return convert(globalBoundingBox());
	}

	qtuser_3d::Box3D ModelGroup::globalSpaceBox()
	{
		trimesh::dbox3 box;
		for (ModelN* model : m_componentModels)
		{
			if (model->isVisible() && 
				model->modelNType() == ModelNType::normal_part)
			{
				box += model->globalBoundingBox();
			}
		}
		return convert(box);
	}

	QMatrix4x4 ModelGroup::globalMatrix()
	{
		return _globalMatrix();
	}

	QMatrix4x4 ModelGroup::localMatrix()
	{
		return _localMatrix();
	}

	std::vector<trimesh::vec3> ModelGroup::caculateOutlinePath()
	{
		std::vector<trimesh::vec3> paths;

		if (m_componentModels.size() <= 0)
			return paths;

		int firstNormalIdx = 0;
		for (int i = 0; i < m_componentModels.size(); i++)
		{
			if (m_componentModels[i]->modelNType() == ModelNType::normal_part)
			{
				firstNormalIdx = i;
			}
		}

		
		QVector3D groupBoxCenter = globalBox().center();
		
		cxkernel::NestDataPtr data = m_componentModels[firstNormalIdx]->nestData();
		
		TriMeshPtr mesh(new trimesh::TriMesh());
		
		cxkernel::MeshDataPtr meshData = m_componentModels[firstNormalIdx]->data();
		if (!meshData.get())
		{
			return paths;
		}

		*mesh = *(meshData->hull);
		
		QMatrix4x4 mat0 = m_componentModels[firstNormalIdx]->globalMatrix();
		trimesh::apply_xform(mesh.get(), trimesh::xform(qtuser_3d::qMatrix2Xform(mat0)));
		
		for (int i = 0; i < m_componentModels.size(); i++)
		{
			if (i == firstNormalIdx || m_componentModels[i]->modelNType() != ModelNType::normal_part)
				continue;

			TriMeshPtr tmpMesh(new trimesh::TriMesh());
		
			*tmpMesh = *(m_componentModels[i]->data()->hull);
		
			QMatrix4x4 mati = m_componentModels[i]->globalMatrix();
			trimesh::apply_xform(tmpMesh.get(), trimesh::xform(qtuser_3d::qMatrix2Xform(mati)));
		
			mesh->vertices.insert(mesh->vertices.end(), tmpMesh->vertices.begin(), tmpMesh->vertices.end());
			mesh->faces.insert(mesh->faces.end(), tmpMesh->faces.begin(), tmpMesh->faces.end());
		}
		
		trimesh::quaternion unit_rotation;
		trimesh::vec3 unit_scale(1.0f);
		paths = data->qPath(mesh, unit_rotation, unit_scale);

		// record the init position
		m_outlinePathInitPos = this->getNodeOffset();

		m_outlineIsCopy = false;

		return paths;
	}

	std::vector<trimesh::vec3> ModelGroup::concave_path()
	{
		// to do
		return std::vector<trimesh::vec3>();
	}

	trimesh::quaternion ModelGroup::nestRotation()
	{
		return trimesh::quaternion();
	}

	QVector3D ModelGroup::zeroLocation()
	{
		QVector3D loc = localPosition();
		qtuser_3d::Box3D box = globalSpaceBox();
		return loc - QVector3D(0.0f, 0.0f, box.min.z());
	}

	bool ModelGroup::isHigherThanBottom()
	{
		const float EPSILON = 0.001f;
		qtuser_3d::Box3D box = globalSpaceBox();
		return box.min.z() > (0.0f + EPSILON);
	}

	void ModelGroup::checkAndSetBottom()
	{
		trimesh::dbox3 box = _globalBoundingBox();
		trimesh::dvec3 boxCenter = box.center();
		double halfZSize = box.size().z / 2.0;
		if (boxCenter.z < halfZSize)
		{
			QVector3D pos = localPosition();
			float newZPos = halfZSize - boxCenter.z;
			setLocalPosition(QVector3D(pos.x(), pos.y(), newZPos));
		}
	}

	int ModelGroup::normalPartCount()
	{
		int count = 0; 
		for (int i = 0; i < m_componentModels.size(); i++)
		{
			if (m_componentModels[i]->modelNType() == ModelNType::normal_part)
				count++;
		}

		return count;
	}

	int ModelGroup::firstNormalPartObjectId()
	{
		for (int i = 0; i < m_componentModels.size(); i++)
		{
			if (m_componentModels[i]->modelNType() == ModelNType::normal_part)
				return m_componentModels[i]->getObjectId();
		}

		return -1;
	}

	void ModelGroup::updateNode()
	{
		if (m_modelGroupEntity)
		{
			m_modelGroupEntity->setPose(localMatrix());

			if (m_rsPath.size() == 0 || m_outlineIsCopy)
			{
				updateSweepAreaPath();
			}

			trimesh::dvec3 offset = this->getNodeOffset() - m_collishPathInitPos;
			offset.z = 0.0f;
			QMatrix4x4 m;
			m.translate(offset.x, offset.y, offset.z);
			m_modelGroupEntity->setLinesPose(m);
		}

		for (ModelN* model : m_componentModels)
		{
			model->updateNode();
		}

#ifdef DEBUG
		showOutlinePath();
#endif // DEBUG

	}

	cxkernel::MeshDataPtr ModelGroup::meshDataWithHullFaces(bool global)
	{
		cxkernel::MeshData* data = new cxkernel::MeshData;

		TriMeshPtr mesh(new trimesh::TriMesh);
		TriMeshPtr hull(new trimesh::TriMesh);
		QList<creative_kernel::ModelN*> groupModels = models();
		for (creative_kernel::ModelN* model : groupModels)
		{
			cxkernel::MeshDataPtr meshData = model->data(); 
			trimesh::xform xf;
			if (global)
				xf = trimesh::xform(qtuser_3d::qMatrix2Xform(model->globalMatrix()));
			else 
				xf = trimesh::xform(qtuser_3d::qMatrix2Xform(model->localMatrix()));
			trimesh::TriMesh* triMesh = new trimesh::TriMesh;
			*triMesh = *meshData->mesh;
			TriMeshPtr meshPtr(triMesh);
			trimesh::apply_xform(triMesh, xf);
			triMesh = new trimesh::TriMesh;
			*triMesh = *meshData->hull;
			TriMeshPtr hullPtr(triMesh);
			trimesh::apply_xform(triMesh, xf);

			auto& _vertices1 = meshPtr->vertices;
			mesh->vertices.insert(mesh->vertices.end(), _vertices1.begin(), _vertices1.end());
			auto& _face1 = meshPtr->faces;
			mesh->faces.insert(mesh->faces.end(), _face1.begin(), _face1.end());
			auto& _vertices2 = hullPtr->vertices;
			hull->vertices.insert(hull->vertices.end(), _vertices2.begin(), _vertices2.end());
			auto& _face2 = hullPtr->faces;
			hull->faces.insert(hull->faces.end(), _face2.begin(), _face2.end());
		}
		hull = cxkernel::MeshData::calculateHull(hull);
		const std::vector<cxkernel::KernelHullFace> &faces = cxkernel::MeshData::calculateFaces(mesh, hull);
		
		data->mesh = mesh;
		data->hull = hull;
		data->faces = faces;

		cxkernel::MeshDataPtr ptr(data);
		return ptr;
	}

	void ModelGroup::layerBottom()
	{
		trimesh::dbox3 box;
		for (ModelN* model : m_componentModels)
		{
			if (model->isVisible() &&
				model->modelNType() == ModelNType::normal_part)
			{
				box += model->globalBoundingBox();
			}
		}
		applyMatrix(trimesh::xform::trans(0.0, 0.0, -box.min.z));
	}

	void ModelGroup::setOutlinePathDirty()
	{
		// when copy modelGroup, the outline path NO need to recaculate
		if (m_outlineIsCopy)
			return;
		
		m_outlinePathIsDirty = true;
		m_sweepPathDirty = true;
	}

	void ModelGroup::setOutlinePathInitPos(const trimesh::dvec3& pos)
	{
		m_outlinePathInitPos = pos;
	}

	trimesh::dvec3 ModelGroup::getOutlinePathInitPos()
	{
		return m_outlinePathInitPos;
	}

	void ModelGroup::resetOutlineState()
	{
		// fix:[ID1028075] after the modelGroup is deleted, if undo, NO need to caculate outline-path
		m_outlineIsCopy = true;
	}

	void ModelGroup::showOutlinePath()
	{
		if (m_modelGroupEntity)
		{
			std::vector<trimesh::vec3> lines;
			qtuser_3d::loopPolygon2Lines(rsPath(), lines);
			m_modelGroupEntity->updateConvex(lines);
		}

	}

	void ModelGroup::setChildrenPartStateDirty()
	{
		for (ModelN* m : models())
		{
			if (m)
			{
				m->setCacheGlobalBoxDirty();
			}
		}
	}

	void ModelGroup::updateAllPartsIndex()
	{
		std::sort(m_componentModels.begin(), m_componentModels.end(), [](ModelN* m1, ModelN* m2)->bool {
			int t1 = (int)m1->modelNType(), t2 = (int)m2->modelNType();
			if (t1 == t2)
				return m1->getObjectId() < m2->getObjectId();
			else
				return t1 < t2;
			});

		for (int i = 0; i < m_componentModels.size(); i++)
		{
			ModelN* aModel = m_componentModels.at(i);
			if (aModel)
			{
				aModel->setPartIndex(i);
			}
		}
	}

	void ModelGroup::onModelNTypeChanged()
	{
		std::sort(m_componentModels.begin(), m_componentModels.end(), [](ModelN* m1, ModelN* m2)->bool {
			int t1 = (int)m1->modelNType(), t2 = (int)m2->modelNType();
			if (t1 == t2)
				return m1->getObjectId() < m2->getObjectId();
			else 
				return t1 < t2;
		});

		emit modelNTypeChanged(sender());
	}

	void ModelGroup::onModelNDirtyChanged()
	{
		ModelN* model = dynamic_cast<ModelN*>(sender());
		if (!model)
			return;

		bool isDirty = model->isDirty();
		if (m_isDirty != isDirty)
		{
			m_isDirty = isDirty;
			emit dirtyChanged();
		}

	}

	std::vector<trimesh::vec3> ModelGroup::rsPath()
	{
		if (m_rsPath.size() <= 0 || m_outlinePathIsDirty)
		{
			// need to recaculate the outlinePath if necessary
			m_rsPath = caculateOutlinePath();
			m_outlinePathIsDirty = false;
		}

		// do the outlinePath offset
		std::vector<trimesh::vec3> paths = m_rsPath;

		trimesh::dvec3 offset = this->getNodeOffset() - m_outlinePathInitPos;
		offset.z = 0.0f;

		trimesh::xform offsetXf;
		offsetXf = offsetXf.trans(offset.x, offset.y, offset.z);

		msbase::applyMatrix2Points(paths, trimesh::fxform(offsetXf));

		// when copy group, no need to recaculate the outlinePath
		m_outlineIsCopy = false;

		return paths;
	}

	const std::vector<trimesh::vec3>& ModelGroup::sweepAreaPath()
	{
		std::vector<trimesh::vec3> convexData = rsPath();

		std::vector<trimesh::vec3> orbit = getKernel()->parameterManager()->extruderClearanceContour();
		m_sweepPath = nestplacer::sweepAreaProfile(convexData, orbit, trimesh::vec3(0.0f));

		return m_sweepPath;
	}

	void ModelGroup::copyOutlinePath(const std::vector<trimesh::vec3>& copyPath)
	{
		if (m_rsPath.size() <= 0)
		{
			m_rsPath = copyPath;
			m_outlinePathIsDirty = false;
			m_outlineIsCopy = true;

		}
			
	}

	void ModelGroup::onLocalPositionChanged(const trimesh::dvec3& newPosition)
	{
	}

	void ModelGroup::onLocalScaleChanged(const trimesh::dvec3& newScale)
	{
		setOutlinePathDirty();
		setChildrenPartStateDirty();
	}

	void ModelGroup::onLocalQuaternionChanged(const trimesh::dvec3& newQ)
	{
		setOutlinePathDirty();
		setChildrenPartStateDirty();
	}


	void ModelGroup::setOuterLinesColor(const QVector4D& color)
	{
		if (m_modelGroupEntity)
			m_modelGroupEntity->setOuterLinesColor(color);
	}

	void ModelGroup::setOuterLinesVisibility(bool visible)
	{
		if (m_modelGroupEntity)
			m_modelGroupEntity->setOuterLinesVisibility(visible);
	}

	void ModelGroup::setNozzleRegionVisibility(bool visible)
	{
		if (m_modelGroupEntity)
			m_modelGroupEntity->setNozzleRegionVisibility(visible);
	}

	void ModelGroup::updateSweepAreaPath()
	{
		if (m_modelGroupEntity == nullptr) return;

		if (!m_sweepPathDirty) {
			return;
		}

		m_modelGroupEntity->updateLines(sweepAreaPath());

		m_collishPathInitPos = this->getNodeOffset();
		m_sweepPathDirty = false;
	}

	void ModelGroup::setLayerHeightProfile(const std::vector<double>& layerHeightProfile)
	{
		if (m_layerHeightProfile == layerHeightProfile)
			return;

		m_layerHeightProfile = layerHeightProfile;
		QList<ModelN*> ms = normalModels();

		for (size_t i = 0; i < ms.size(); i++)
		{
			ModelN* m = ms.at(i);
			m->setLayerHeightProfile(layerHeightProfile);
		}

		dirty();
		Q_EMIT adaptiveLayerDataChanged();
	}

	const std::vector<double>& ModelGroup::layerHeightProfile()
	{
		return m_layerHeightProfile;
	}

	void ModelGroup::setLayersTexture(const QImage& image)
	{
		Qt3DRender::QTexture2D* tex = qtuser_3d::createFromImage(image);
		tex->setGenerateMipMaps(true);
		tex->setParent(m_modelGroupEntity);

		qtuser_3d::Box3D box = globalSpaceBox();
		QList<ModelN*> ms = normalModels();

		for (size_t i = 0; i < ms.size(); i++)
		{
			ModelN* m = ms.at(i);
			ModelNEntity* entity = m->entity();
			if (entity)
			{
				entity->setLayersTexture(tex);
				
				QMatrix4x4 parentMatrix;
				entity->updateBoxLocal(box, parentMatrix);
			}
		}

		if (m_layerHeightTexture)
		{
			delete m_layerHeightTexture;
		}
		m_layerHeightTexture = tex;
	}
}