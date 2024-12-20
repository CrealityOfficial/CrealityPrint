#include "modeln.h"

#include "qtuser3d/utils/texturecreator.h"

#include "interface/reuseableinterface.h"
#include "interface/spaceinterface.h"
#include "interface/selectorinterface.h"
#include "interface/appsettinginterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/parameterinterface.h"
#include "interface/machineinterface.h"
#include "interface/shareddatainterface.h"
#include "interface/projectinterface.h"
#include "us/usettings.h"

#include "qtuser3d/math/angles.h"
#include "qtuser3d/geometry/geometrycreatehelper.h"
#include "cxkernel/data/trimeshutils.h"

#include "qtuser3d/math/space3d.h"

#include "qtuser3d/trimesh2/conv.h"

#include "cxkernel/data/raymesh.h"

#include "internal/render/modelnentity.h"
#include "internal/render/modelgroupentity.h"
#include "internal/project_cx3d/cx3dmanager.h"
#include <QColor>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include "qtuser3d/refactor/xeffect.h"
#include "msbase/mesh/deserializecolor.h"

#include "kernel/kernel.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/printmachine.h"

#include "us/usettingwrapper.h"
#include "nestplacer/printpiece.h"
#include "slice/adaptiveslice.h"
#include "data/modelgroup.h"

#include "interface/renderinterface.h"
#include "external/data/shareddatamanager/shareddatamanager.h"
#include "external/data/shareddatamanager/meshdatawrapper.h"
#include "kernel/kernel.h"
#include "external/data/spaceutils.h"

#include "operation/fontmesh/fontmeshwrapper.h"

namespace creative_kernel
{
	ModelN::ModelN(QObject* parent)
		:QNode3D(parent)
		, SharedDataTracer()
		, m_nestRotation(0)
		, m_modelGroup(nullptr)
		, m_objectId(-1)
		, m_entity(nullptr)
		, m_renderType(ModelNEntity::ViewRender)
	{
		m_lastUsedSetting = new us::USettings(this);
		setsetting(new us::USettings(this));

		m_visibility = true;
		setNozzle(0);

		ParameterManager* parameterManager = getKernel()->parameterManager();
		connect(parameterManager, &ParameterManager::parameterEdited, this, &ModelN::onGlobalSettingsChanged);
		connect(this, &ModelN::defaultColorIndexChanged, this, &ModelN::updateSetting);

	}

	ModelN::~ModelN()
	{
		m_entity = nullptr;
		if (m_fontMesh)
			delete m_fontMesh;
	}

	void ModelN::setEntity(ModelNEntity* entity)
	{
		m_entity = entity;
		if (!m_entity)
			return;

		m_entity->setEnabled(true);
		connect(m_entity, &QObject::destroyed, this, [=]() { m_entity = NULL; });

		m_entity->setEntityType(modelNType() != ModelNType::normal_part, false);
		setRenderData();
		updateEntityRender();
		updateNode();
	}

	ModelNEntity* ModelN::entity()
	{
		return m_entity;
	}

	Qt3DCore::QEntity* ModelN::qEntity()
	{
		return m_entity;
	}

	//void ModelN::onGlobalMatrixChanged(const QMatrix4x4& globalMatrix)
	//{
	//	qtuser_3d::Box3D box = globalSpaceBox();
	//
	//	QMatrix4x4 matrix = globalMatrix;
	//	m_entity->updateBoxLocal(box, matrix);
	//}
	//
	//void ModelN::onLocalMatrixChanged(const QMatrix4x4& localMatrix)
	//{
	//	m_entity->setPose(localMatrix);
	//}

	void ModelN::onStateChanged(qtuser_3d::ControlState state)
	{
		setState((int)state);
		//m_entity->setBoxVisibility(selected() ? true : false);
	}

	void ModelN::meshChanged()
	{

	}

	void ModelN::faceBaseChanged(int faceBase)
	{
		QPoint vertexBase(0, 0);
		vertexBase.setX(faceBase * 3);
		if (m_entity)
			m_entity->setVertexBase(vertexBase);
	}

	us::USettings* ModelN::setting()
	{
		return m_setting;
	}

	void ModelN::setsetting(us::USettings* modelsetting)
	{
		if (m_setting)
		{
			disconnect(m_setting, &us::USettings::settingsChanged, this, &ModelN::onSettingsChanged);
			disconnect(m_setting, &us::USettings::settingValueChanged, this, &ModelN::onSettingsChanged);
		}

		m_setting = modelsetting;
		connect(m_setting, &us::USettings::settingsChanged, this, &ModelN::onSettingsChanged, Qt::UniqueConnection);
		connect(m_setting, &us::USettings::settingValueChanged, this, &ModelN::onSettingsChanged, Qt::UniqueConnection);
	}

	bool ModelN::hasParameter(const QString& key)
	{
		return m_setting->hasKey(key);
	}

	void ModelN::getUsedSetting(us::USettings* setting)
	{
		QSharedPointer<us::USettings> globalSetting = creative_kernel::createCurrentGlobalSettings();
		setting->clear();
		setting->merge(globalSetting.get());
		setting->merge(m_setting);
	}

	void ModelN::recordSetting()
	{
		getUsedSetting(m_lastUsedSetting);
	}

	bool ModelN::checkSettingDirty()
	{
		QSharedPointer<us::USettings> currentUsedSetting(new us::USettings());
		getUsedSetting(currentUsedSetting.get());

		if (*(currentUsedSetting.get()) == *m_lastUsedSetting)
			return false;
		else
			return true;
	}

	bool ModelN::checkSettingDirty(const QString& key)
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
				return false;
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

	void ModelN::setVisibility(bool visibility)
	{
		int p = getKernel()->currentPhase();
		if (p == 0)
			m_visibility = visibility;

		if (m_entity)
			m_entity->setVisible(m_visibility);

		emit isVisibleChanged();
		dirty();

		requestVisUpdate(true);
	}

	bool ModelN::isVisible()
	{
		return m_visibility;
	}

	void ModelN::useVariableLayerHeightEffect()
	{
		if (m_entity)
			m_entity->useVariableLayerHeightEffect();
	}

	void ModelN::useDefaultModelEffect()
	{
		if (m_entity)
			m_entity->useDefaultModelEffect();
	}

	void ModelN::setNozzle(int nozzle)
	{
		if (m_entity)
			m_entity->setNozzle(nozzle);

		set_nozzle(m_setting, nozzle);
	}

	int ModelN::nozzle()
	{
		return get_nozzle(m_setting);
	}

	bool ModelN::needInit()
	{
		return m_needInit;
	}

	QVector3D& ModelN::GetInitPosition()
	{
		return m_initPosition;
	}

	void ModelN::SetInitPosition(QVector3D QPosition)
	{
		m_needInit = false;
		m_initPosition = QPosition;
	}

	void ModelN::rotateNormal2Bottom(const QVector3D& normal, QVector3D& position, QQuaternion& rotate)
	{
		//QVector3D z = QVector3D(0.0f, 0.0f, -1.0f);
		//QMatrix4x4 qmatrix = globalMatrix();
		//QVector3D qNormal = normal.normalized();
		//qNormal.normalize();
		//QQuaternion qq = qtuser_3d::quaternionFromVector3D2(qNormal, z, true);
		//
		//QQuaternion oldRotate = localQuaternion();
		//QQuaternion q = oldRotate;
		//QVector3D oldCenter = globalSpaceBox().center();
		//
		//q = qq * q;
		//setLocalQuaternion(q);
		//
		//qtuser_3d::Box3D box = globalSpaceBox();
		//QVector3D offset = oldCenter - box.center();
		//QVector3D zoffset = QVector3D(offset.x(), offset.y(), -box.min.z());
		//QVector3D _lPosition = localPosition();
		//
		//position = _lPosition + zoffset;
		//rotate = q;
	}

	void ModelN::setState(int state)
	{
		if (m_entity)
			m_entity->setState(state);
	}

	int ModelN::getState()
	{
		return m_entity ? m_entity->getState() : 0;
	}

	void ModelN::SetModelViewClrState(qtuser_3d::ControlState statevalue, bool boxshow)
	{
		setState((float)statevalue);
	}

	trimesh::TriMesh* ModelN::mesh()
	{
		cxkernel::MeshDataPtr meshDataPtr = getMeshData(sharedDataID());
		return meshDataPtr ? meshDataPtr->mesh.get() : nullptr;
	}

	TriMeshPtr ModelN::meshptr()
	{
		cxkernel::MeshDataPtr meshDataPtr = getMeshData(sharedDataID());
		return meshDataPtr ? meshDataPtr->mesh : nullptr;
	}

	TriMeshPtr ModelN::globalMesh(bool needMergeColorMesh)
	{
		trimesh::TriMesh* mesh = nullptr;

		SharedDataID dataID = sharedDataID();
		cxkernel::MeshDataPtr meshDataPtr = getMeshData(dataID);
		auto colorsData = getColors(dataID);

		EngineType engine_type = getEngineType();
		//解析颜色
		if (needMergeColorMesh && hasColors() && engine_type == EngineType::ET_CURA)
		{
			std::vector<int> facet2Facets;
			mesh = msbase::mergeColorMeshes(meshDataPtr->mesh.get(), colorsData, facet2Facets);
			mesh->need_normals();
		}
		else
		{
			mesh = new trimesh::TriMesh();
			*mesh = *meshDataPtr->mesh;

			//add default color
			if (mesh->flags.size() != mesh->faces.size())
			{
				mesh->flags.clear();
				mesh->flags.resize(mesh->faces.size(), 0);
			}
		}

		trimesh::apply_xform(mesh, trimesh::xform(qtuser_3d::qMatrix2Xform(_globalMatrix())));
		return TriMeshPtr(mesh);
	}

	TriMeshPtr ModelN::localMesh(bool needMergeColorMesh)
	{
		trimesh::TriMesh* mesh = nullptr;

		SharedDataID dataID = sharedDataID();
		cxkernel::MeshDataPtr meshDataPtr = getMeshData(dataID);
		auto colorsData = getColors(dataID);

		EngineType engine_type = getEngineType();
		//解析颜色
		if (needMergeColorMesh && hasColors() && engine_type == EngineType::ET_CURA)
		{
			std::vector<int> facet2Facets;
			mesh = msbase::mergeColorMeshes(meshDataPtr->mesh.get(), colorsData, facet2Facets);
			mesh->need_normals();
		}
		else
		{
			mesh = new trimesh::TriMesh();
			*mesh = *meshDataPtr->mesh;

			//add default color
			if (mesh->flags.size() != mesh->faces.size())
			{
				mesh->flags.clear();
				mesh->flags.resize(mesh->faces.size(), 0);
			}
		}

		trimesh::apply_xform(mesh, trimesh::xform(qtuser_3d::qMatrix2Xform(_localMatrix())));
		return TriMeshPtr(mesh);
	}

	void ModelN::setModelNType(ModelNType type)
	{
		if (type == modelType())
			return;

		setModelType(type);
		setRenderData();
		setEntityType(type != ModelNType::normal_part);
		emit modelNTypeChanged();

		dirty();
	}

	ModelNType ModelN::modelNType()
	{
		return modelType();
	}

	int ModelN::modelNTypeInt()
	{
		return (int)modelType();
	}

	bool ModelN::needRepair()
	{
		return getMeshNeedRepair(meshID());
	}

	void ModelN::setSharedData(SharedDataID id)
	{
		ModelNType type = modelType();
		SharedDataID oldID = sharedDataID();
		setSharedDataID(id);
		setRenderData();
		setEntityType(id(ModelType) != (int)ModelNType::normal_part);

		if (oldID != id)
		{
			if (id(ModelType) != (int)type)
				modelNTypeChanged();

			dirty();
			needRegenerate();
		}

		updateEntityRender();
	}

	void ModelN::beginUpdateRender()
	{
		if (!m_entity)
			return;

		m_renderFlagCache = m_entity->isEnabled();
		requestRender(this);
		if (m_entity)
			m_entity->setEnabled(false);
	}

	void ModelN::updateRender()
	{
		setRenderData();
		emit colorsDataChanged();
	}

	void ModelN::endUpdateRender()
	{
		if (m_entity)
			m_entity->setEnabled(m_renderFlagCache);
		endRequestRender(this);
	}

	void ModelN::onDiscardMaterial(int materialIndex)
	{
		ModelGroup* group = getModelGroup();
		if (group)
		{
			if (group->defaultColorIndex() == materialIndex)
			{
				group->discardDefaultColor();
			}
			else
			{
				int index = group->defaultColorIndex();
				if (defaultColorIndex() != index)
					setDefaultColorIndex(index);
			}
		}
		else
		{
			setDefaultColorIndex(0);
		}
	}

	void ModelN::setRenderData()
	{
		if (!m_entity)
			return;

		updatePalette();
		dirty();

		RenderDataPtr renderData = getRenderData(sharedDataID());
		cxkernel::MeshDataPtr meshData = getMeshData(sharedDataID());

		if (!renderData || !meshData)
		{
			m_primitiveNum = 0;
			return;
		}

		if (m_colorIndex != materialID())
		{
			m_colorIndex = materialID();
			emit defaultColorIndexChanged();
		}

		requestRender(this);
		m_entity->setEnabled(false);
		m_entity->setGeometry(renderData->geometry(), Qt3DRender::QGeometryRenderer::Triangles, false);
		m_entity->setEnabled(true);
		endRequestRender(this);

		auto colorsData = getColors(sharedDataID());
		if (colorsData.empty())
			m_primitiveNum = meshData->mesh->faces.size();
		else
			m_primitiveNum = renderData->spreadFaceCount();
	}

	RenderDataPtr ModelN::renderData()
	{
		return getRenderData(sharedDataID());
	}

	QString ModelN::fileName() const
	{
		return m_fileName;
	}

	void ModelN::setFileName(const QString& fileName)
	{
		m_fileName = fileName;
	}

	QString ModelN::name() const
	{
		return objectName();
	}

	void ModelN::setName(const QString& name)
	{
		setObjectName(name);
		m_name = name;
		emit nameChanged();
	}

	bool ModelN::isRelief() const
	{
		auto ptr = m_fontMesh;
		bool is = false;
		if (ptr)
			is = true;
		else
			is = false;

		return is;
	}

	QString ModelN::reliefText() const
	{
		if (!m_fontMesh)
			return "";

		QString text = m_fontMesh->text();
		return text;
	}

	FontMeshWrapper* ModelN::fontMesh()
	{
		return m_fontMesh;
	}

	void ModelN::setFontMesh(FontMeshWrapper* fontMesh)
	{
		m_fontMesh = fontMesh;
		if (m_fontMesh)
			m_fontMesh->setRelief(this);
	}

	void ModelN::setFontMesh(std::string fontMeshStr)
	{
		if (fontMeshStr.empty())
			return;

		FontMeshWrapper* fm = new FontMeshWrapper(fontMeshStr);
		m_fontMesh = fm;
		if (m_fontMesh)
		{
			m_fontMesh->setRelief(this);
			if (m_fontMesh->isClonedFontMesh())
				m_fontMesh->regenerateMesh();
		}
	}

	std::string ModelN::fontMeshString(bool isClone)
	{
		std::string fontMeshStr;

		if (!m_fontMesh)
			return fontMeshStr;

		fontMeshStr = m_fontMesh->serialize(isClone);
		return fontMeshStr;
	}

	QString ModelN::description() const
	{
		return m_description;
	}

	void ModelN::setDescription(const QString& description)
	{
		m_description = description;
	}

	cxkernel::MeshDataPtr ModelN::data()
	{
		cxkernel::MeshDataPtr meshData = getMeshData(sharedDataID());
		return meshData;
	}

	std::vector<trimesh::vec3> ModelN::getoutline_ObjectExclude()
	{
		std::vector<trimesh::vec3> outline = outline_path(true, false);
		return outline;
	}

	int ModelN::primitiveNum()
	{
		return m_primitiveNum;
	}

	void ModelN::setPose(const QMatrix4x4& pose)
	{
		if (m_entity)
			m_entity->setPose(pose);
	}

	std::string ModelN::getColors2Facets(int index)
	{
		auto colors = getColors(sharedDataID());
		if (colors.size() > index && colors.size() > 0)
		{
			return colors[index];
		}
		return "";
	}

	void ModelN::setColors2Facets(const std::vector<std::string>& data)
	{
		int colorsID = registerColor(data);
		setColorsID(colorsID);
		setRenderData();
		needRegenerate();
		emit colorsDataChanged();
	}

	std::vector<std::string> ModelN::getColors2Facets()
	{
		return getColors(sharedDataID());
	}

	std::vector<std::string> ModelN::getSeam2Facets()
	{
		return getSeams(sharedDataID());
	}

	std::vector<std::string> ModelN::getSupport2Facets()
	{
		return getSupports(sharedDataID());
	}

	void ModelN::setFacet2Facets(const std::vector<int>& facet2Facets)
	{

	}

	int ModelN::getFacet2Facets(int faceId)
	{
		RenderDataPtr data = getRenderData(sharedDataID());
		if (!data->spreadFaces().empty() && faceId > 0 && faceId < data->spreadFaces().size())
		{
			return data->spreadFaces()[faceId];
		}
		return faceId;
	}

	bool ModelN::hasColors()
	{
		auto sharedDataManager = getKernel()->sharedDataManager();
		for (auto& color : sharedDataManager->getColors(sharedDataID()))
		{
			if (!color.empty())
			{
				return true;
			}
		}
		return false;
	}

	bool ModelN::hasSeamsData()
	{
		for (auto& seam : getSeams(sharedDataID()))
		{
			if (!seam.empty())
			{
				return true;
			}
		}
		return false;
	}

	bool ModelN::hasSupportsData()
	{
		for (auto& support : getSupports(sharedDataID()))
		{
			if (!support.empty())
			{
				return true;
			}
		}
		return false;
	}

	bool ModelN::adaptiveLayerData()
	{
		int nRet = layerHeightProfile().size();
		return nRet > 0;
	}

	void ModelN::setDefaultColorIndex(int colorIndex)
	{
		if (colorIndex < 0)
			return;

		if (materialID() == colorIndex)
			return;

		setMaterialID(colorIndex);

		setRenderData();
		requestVisUpdate(true);

		dirty();
		needRegenerate();

		emit dirtyChanged();
		emit defaultColorIndexChanged();
	}

	int ModelN::defaultColorIndex()
	{
		ModelGroup* mg = getModelGroup();
		QList<ModelN*> models = mg->models();
		if (models.size() == 1 && models.at(0) == this)
		{
			mg->setDefaultColorIndex(materialID());
		}

		return materialID();
	}

	void ModelN::updateRenderByMeshSpreadData(const std::vector<std::string>& colorData, bool updateCapture)
	{
		if (colorData.empty())
			return;

		int colorsID = registerColor(colorData);
		setColorsID(colorsID);
		setRenderData();
		needRegenerate();
		requestVisUpdate(true);
		QList<creative_kernel::ModelGroup *> grouplist;
		grouplist.append(m_modelGroup);
		getKernel()->cx3dManager()->triggerAutoSave(grouplist,AutoSaveJob::COLOR);
		emit colorsDataChanged();
	}

	void ModelN::updateRender(bool updateCapture)
	{
		setRenderData();
		if (updateCapture)
			requestVisPickUpdate(true);
		else
			requestVisUpdate(updateCapture);
	}

	//paint seam
	void ModelN::setSeam2Facets(const std::vector<std::string>& data)
	{
		int seamsID = registerSeam(data);
		setSeamsID(seamsID);
		needRegenerate();
		QList<creative_kernel::ModelGroup *> grouplist;
		grouplist.append(m_modelGroup);
		getKernel()->cx3dManager()->triggerAutoSave(grouplist,AutoSaveJob::SEAM);
		emit seamsDataChanged();
	}

	//paint support
	void ModelN::setSupport2Facets(const std::vector<std::string>& data)
	{
		int supportsID = registerSupport(data);
		setSupportsID(supportsID);
		needRegenerate();
		QList<creative_kernel::ModelGroup *> grouplist;
		grouplist.append(m_modelGroup);
		getKernel()->cx3dManager()->triggerAutoSave(grouplist,AutoSaveJob::SUPPORT);
		emit supportsDataChanged();
	}

	bool ModelN::isContainsMaterial(int materialIndex)
	{
		emit prepareCheckMaterial();

		RenderDataPtr data = getRenderData(sharedDataID());
		if (!data)
			return false;

		return data->colorIndexs().contains(materialIndex);
	}

	std::set<int> ModelN::colorIndexes()
	{
		RenderDataPtr data = getRenderData(sharedDataID());
		if (!data)
		{
			return {};
		}

		auto indexes = data->colorIndexs();
		return { indexes.cbegin(), indexes.cend() };
	}

	void ModelN::setLayerHeightProfile(const std::vector<double>& layerHeightProfile)
	{
		if (m_layerHeightProfile == layerHeightProfile)
			return;

		m_layerHeightProfile = layerHeightProfile;
		needRegenerate();

		//dirty();
		//Q_EMIT adaptiveLayerDataChanged();
	}

	const std::vector<double>& ModelN::layerHeightProfile()
	{
		return m_layerHeightProfile;
	}

	void ModelN::updateEntityRender()
	{
		ModelNEntity* entity = dynamic_cast<ModelNEntity*>(m_entity);
		if (entity)
			entity->updateRender();
	}

	void ModelN::setOffscreenRenderEnabled(bool enabled, bool applyNow)
	{
		ModelNEntity* entity = dynamic_cast<ModelNEntity*>(m_entity);
		if (entity)
			entity->setOffscreenRenderEnabled(enabled, applyNow);
	}

	void ModelN::setEntityType(int type, bool applyNow)
	{
		ModelNEntity* entity = dynamic_cast<ModelNEntity*>(m_entity);
		if (entity)
			entity->setEntityType(type, applyNow);
	}

	void ModelN::setVisible(bool visible, bool applyNow)
	{
		ModelNEntity* entity = dynamic_cast<ModelNEntity*>(m_entity);
		if (entity)
			entity->setVisible(visible, applyNow);
	}

	void ModelN::setLayersColor(const QVariantList& layersColor)
	{
		ModelNEntity* entity = dynamic_cast<ModelNEntity*>(m_entity);
		if (entity)
			entity->setLayersColor(layersColor);
	}

	void ModelN::setOffscreenRenderOffset(const QVector3D& offset)
	{
		ModelNEntity* entity = dynamic_cast<ModelNEntity*>(m_entity);
		if (entity)
			entity->setOffscreenRenderOffset(offset);

	}

	void ModelN::useCustomEffect(qtuser_3d::XEffect* effect)
	{
		ModelNEntity* entity = dynamic_cast<ModelNEntity*>(m_entity);
		if (entity)
			entity->useCustomEffect(effect);
	}

	void ModelN::updatePalette()
	{
		/* init paltte */
		ModelNType type = modelType();
		QVariantList qcolors;
		if (type == ModelNType::normal_part)
		{
			std::vector<trimesh::vec3> colors = currentColors();
			if (colors.size() == 0)
				return;

			int dIndex = defaultColorIndex();
			if (colors.size() <= dIndex)
				dIndex = 0;

			qcolors << QVector4D(colors[dIndex][0], colors[dIndex][1], colors[dIndex][2], 1.0);
			for (int i = 0; i < 16; ++i)
			{
				trimesh::vec3 color;
				if (i < colors.size())
					color = colors[i];
				else
					color = colors[dIndex];

				qcolors << QVector4D(color[0], color[1], color[2], 1.0);
			}
		}
		else if (type == ModelNType::negative_part)
		{
			for (int i = 0; i < 17; ++i)
				qcolors << QVector4D(0, 0, 0, 1.0);
		}
		else if (type == ModelNType::modifier)
		{
			for (int i = 0; i < 17; ++i)
				qcolors << QVector4D(1.0, 1.0, 0.0, 1.0);
		}
		else if (type == ModelNType::support_defender)
		{
			for (int i = 0; i < 17; ++i)
				qcolors << QVector4D(0.76, 0.32, 0.32, 1.0);
		}
		else if (type == ModelNType::support_generator)
		{
			for (int i = 0; i < 17; ++i)
				qcolors << QVector4D(0.32, 0.76, 0.32, 1.0);
		}

		if (m_entity && !qcolors.isEmpty())
		{
			m_entity->setPalette(qcolors);
		}
	}

	void ModelN::setObjectId(int64_t objId)
	{
		m_objectId = objId;
	}

	int64_t ModelN::getObjectId()
	{
		return m_objectId;
	}

	void ModelN::setFixedId(int64_t fixedId)
	{
		m_fixedId = fixedId;
	}

	int64_t ModelN::getFixedId()
	{
		return m_fixedId;
	}

	void ModelN::onAttachModelGroup(ModelGroup* _model_group)
	{
		m_modelGroup = _model_group;
		setParent(_model_group);

		m_globalBoxDirty = true;
	}

	ModelGroup* ModelN::getModelGroup()
	{
		return m_modelGroup;
	}

	bool ModelN::getFanZhuanState()
	{
		if (!m_modelGroup)
			return false;

		trimesh::xform gMatrix = this->getMatrix() * m_modelGroup->getMatrix();
		bool fanzhuan = false;
		for (int i = 0; i < 3; i++)
		{
			if (gMatrix(i, i) < -0.9)
			{
				fanzhuan = !fanzhuan;
			}
		}
		return fanzhuan;
	}

	QObject* ModelN::getModelGroupObject()
	{
		return m_modelGroup;
	}

	int ModelN::getModelGroupSize()
	{
		return m_modelGroup->models().size();
	}

	void ModelN::updateNode()
	{
		if (m_entity)
		{
			//m_entity->setPose(localMatrix());
			m_entity->setParameter("model_matrix", globalMatrix());
		}
	}

	bool ModelN::flushIntoInfill()
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

	void ModelN::setFlushIntoInfillEnabled(bool enabled)
	{
		QString key = "flush_into_infill";
		m_setting->add(key, enabled ? "1" : "0", true);
	}

	bool ModelN::flushIntoObjects()
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

	void ModelN::setFlushIntoObjectsEnabled(bool enabled)
	{
		QString key = "flush_into_objects";
		m_setting->add(key, enabled ? "1" : "0", true);
	}

	bool ModelN::flushIntoSupport()
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

	void ModelN::setFlushIntoSupportEnabled(bool enabled)
	{
		QString key = "flush_into_support";
		m_setting->add(key, enabled ? "1" : "0", true);
	}

	bool ModelN::supportEnabled()
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

		return (value == "true" || value == "1") ? true : false;
	}

	void ModelN::setSupportEnabled(bool enabled)
	{
		QString key = "enable_support";
		m_setting->add(key, enabled ? "1" : "0", true);
	}

	float ModelN::supportAngle()
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
		return value.toFloat();
	}

	void ModelN::setSupportAngle(float angle)
	{
		QString key = "support_threshold_angle";
		QString valueStr = QString::number(angle);
		m_setting->add(key, valueStr, true);
	}

	void ModelN::setCacheGlobalBoxDirty()
	{
		m_cacheGlobalBoxIsDirty = true;
	}

	void ModelN::setPartIndex(int partIndex)
	{
		m_partIndex = partIndex;
	}

	QString ModelN::supportStructure()
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

		return (value == "" ? "normal(auto)" : value);
	}

	int ModelN::getPartIndex()
	{
		return m_partIndex;
	}

	void ModelN::setSupportStructure(const QString& structure)
	{
		QString key = "support_type";
		m_setting->add(key, structure, true);
	}

	void ModelN::onGlobalSettingsChanged(QObject* parameter_base, const QString& key)
	{
		if (key == "enable_support")
		{
			emit supportEnabledChanged();
		}
		else if (key == "support_threshold_angle")
		{
			emit supportAngleChanged();
		}
		else if (key == "support_type")
		{
			emit supportStructureChanged();
		}
	}

	void ModelN::onSettingsChanged(const QString& key, us::USetting* setting)
	{
		if (!checkSettingDirty(key))
			return;

		dirty();
		emit settingsChanged();

		if (key == "enable_support")
		{
			emit supportEnabledChanged();
		}
		else if (key == "support_threshold_angle")
		{
			emit supportAngleChanged();
		}
		else if (key == "support_type")
		{
			emit supportStructureChanged();
		}

	}

	void ModelN::updateSetting()
	{
		if (!m_setting)
			return;

		int index = defaultColorIndex() + 1;
		m_setting->add("extruder", QString::number(index), true);
	}

	bool ModelN::isDirty()
	{
		return m_isDirty;
	}

	void ModelN::dirty()
	{
		QList<creative_kernel::ModelGroup *> grouplist;
		grouplist.append(this->m_modelGroup);
		getKernel()->cx3dManager()->triggerAutoSave(grouplist,AutoSaveJob::MODELSETTINGS);
		if (!m_isDirty)
		{
			m_isDirty = true;
			emit dirtyChanged();
		}
		emit formModified();
	}

	void ModelN::resetDirty()
	{
		if (m_isDirty)
		{
			m_isDirty = false;
			emit dirtyChanged();
		}
	}

	void ModelN::needRegenerate()
	{
		m_need_regenerate_id = true;
	}

	trimesh::dbox3 ModelN::localBoundingBox()
	{
		return _localBoundingBox();
	}

	trimesh::dbox3 ModelN::globalBoundingBox()
	{
		if (m_cacheGlobalBoxIsDirty)
		{
			m_cacheGlobalBox = _globalBoundingBox();
			m_cacheGlobalBoxIsDirty = false;

			QMatrix4x4 gMat = this->globalMatrix();
			QVector3D trans = gMat.column(3).toVector3D();
			//m_cacheGlobalBoxInitPos = trimesh::dvec3(trans.x(), trans.y(), trans.z());
			m_cacheGlobalBoxInitPos.x = double(trans.x());
			m_cacheGlobalBoxInitPos.y = double(trans.y());
			m_cacheGlobalBoxInitPos.z = double(trans.z());

			return m_cacheGlobalBox;
		}

		QMatrix4x4 currentGlobalMat = this->globalMatrix();
		QVector3D curTrans = currentGlobalMat.column(3).toVector3D();
		//trimesh::dvec3 curOffset = trimesh::dvec3(curTrans.x(), curTrans.y(), curTrans.z());
		trimesh::dvec3 curOffset;
		curOffset.x = double(curTrans.x());
		curOffset.y = double(curTrans.y());
		curOffset.z = double(curTrans.z());

		if (!fuzzyCompareDvec3(m_cacheGlobalBoxInitPos, curOffset))
		{
			trimesh::dvec3 boxOffset = curOffset - m_cacheGlobalBoxInitPos;

			trimesh::xform transXf;
			transXf = transXf.trans(boxOffset);

			trimesh::dvec3 pmin = transXf * m_cacheGlobalBox.min;
			trimesh::dvec3 pmax = transXf * m_cacheGlobalBox.max;

			trimesh::dbox3 retBox;
			retBox += pmin;
			retBox += pmax;

			return retBox;
		}
		else
		{
			return m_cacheGlobalBox;
		}
	}

	qtuser_3d::Box3D ModelN::globalSpaceBox()
	{
		return convert(globalBoundingBox());
	}

	QMatrix4x4 ModelN::globalMatrix()
	{
		return _globalMatrix();
	}

	QMatrix4x4 ModelN::localMatrix()
	{
		return _localMatrix();
	}

	trimesh::xform ModelN::globalDMatrix()
	{
		return m_modelGroup->getMatrix() * this->getMatrix();
	}

	QMatrix4x4 ModelN::modelNormalMatrix()
	{
		QMatrix3x3 normalMatrix = globalMatrix().normalMatrix();
		return QMatrix4x4(normalMatrix.constData(), 3, 3);
	}

	void ModelN::onLocalScaleChanged(const trimesh::dvec3& newScale)
	{
		m_cacheGlobalBoxIsDirty = true;
		setParentGroupStateDirty();
	}

	void ModelN::onLocalPositionChanged(const trimesh::dvec3& newPosition)
	{
		setParentGroupStateDirty();
	}

	void ModelN::onLocalQuaternionChanged(const trimesh::dvec3& newQ)
	{
		m_cacheGlobalBoxIsDirty = true;
		setParentGroupStateDirty();
	}

	trimesh::dvec3 ModelN::componentOffset()
	{
		if (!m_modelGroup)
			return trimesh::dvec3();

		trimesh::dbox3 box = m_modelGroup->globalBoundingBox();
		trimesh::dvec3 groupBoxCenter = box.center();

		trimesh::dvec3 componentOffset = _globalBoundingBox().center() - groupBoxCenter;
		return componentOffset;
	}

	trimesh::dbox3 ModelN::calculateBoundingBox(const QMatrix4x4& m)
	{
		trimesh::dbox3 db;
		cxkernel::MeshDataPtr meshData = getMeshData(sharedDataID());
		if (meshData && meshData->mesh)
		{
#ifdef DEBUG
			auto start_time = std::chrono::high_resolution_clock::now();
#endif // DEBUG

			db = meshData->calculateBox(trimesh::xform(qtuser_3d::qMatrix2Xform(m)));

#ifdef DEBUG
			auto end_time = std::chrono::high_resolution_clock::now();
			auto ms = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

			qDebug() << "==== MeshData:: (parallel) calculateBox takes : " << ms;
#endif // DEBUG

		}
		return db;
	}

	bool ModelN::rayCheck(int primitiveID, const qtuser_3d::Ray& ray, QVector3D& collide, QVector3D* normal, bool accurate)
	{
		cxkernel::MeshDataPtr meshData = getMeshData(sharedDataID());
		if (!meshData || !meshData->mesh)
			return false;

		trimesh::fxform xf = qtuser_3d::qMatrix2Xform(_globalMatrix());

		cxkernel::Ray cRay(qtuser_3d::qVector3D2Vec3(ray.start), qtuser_3d::qVector3D2Vec3(ray.dir));
		trimesh::vec3 tposition, tnormal;
		bool result = cxkernel::rayMeshCheck(meshData->mesh.get(), xf, primitiveID, cRay, tposition, tnormal, accurate);
		if (result)
		{
			collide = qtuser_3d::vec2qvector(tposition);
			if (normal)
				*normal = qtuser_3d::vec2qvector(tnormal);
		}
		return result;
	}

	// the normal is returned after applyed by (normal matrix)
	bool ModelN::rayCheckEx(int primitiveID, const qtuser_3d::Ray& ray, QVector3D& collide, QVector3D& outNormal)
	{
		cxkernel::MeshDataPtr meshData = getMeshData(sharedDataID());
		if (!meshData || !meshData->mesh)
			return false;

		QMatrix4x4 normalMatrix = this->modelNormalMatrix();
		trimesh::fxform normalXf = qtuser_3d::qMatrix2Xform(normalMatrix);

		trimesh::fxform xf = qtuser_3d::qMatrix2Xform(_globalMatrix());

		cxkernel::Ray cRay(qtuser_3d::qVector3D2Vec3(ray.start), qtuser_3d::qVector3D2Vec3(ray.dir));
		trimesh::vec3 tposition, tnormal;
		bool result = cxkernel::rayMeshCheckEx(meshData->mesh.get(), xf, normalXf, primitiveID, cRay, tposition, tnormal);
		if (result)
		{
			collide = qtuser_3d::vec2qvector(tposition);
			outNormal = qtuser_3d::vec2qvector(tnormal);
		}
		return result;
	}

	void ModelN::convex(std::vector<trimesh::vec3>& datas, bool origin)
	{
		cxkernel::MeshDataPtr meshData = getMeshData(sharedDataID());
		if (meshData)
		{
			trimesh::fxform xf = qtuser_3d::qMatrix2Xform(_globalMatrix());
			if (origin)
			{
				xf = trimesh::fxform::trans(-qtuser_3d::qVector3D2Vec3(localPosition()))
					* xf;
			}

			std::vector<trimesh::dvec3> ddatas;
			meshData->convex(trimesh::xform(xf), ddatas);

			for (const trimesh::dvec3& v : ddatas)
				datas.push_back(trimesh::vec3(v));
		}
	}

	QList<QVector3D> ModelN::qConvex(bool toOrigin)
	{
		std::vector<trimesh::vec3> datas;
		convex(datas);

		if (toOrigin)
		{
			trimesh::box3 box;
			for (const trimesh::vec3& v : datas)
				box += v;

			for (trimesh::vec3& v : datas)
				v -= box.min;
		}
		QList<QVector3D> result;
		for (const trimesh::vec3& v : datas)
			result.append(QVector3D(v.x, v.y, v.z));
		return result;
	}

	TriMeshPtr ModelN::createGlobalMesh()
	{
		cxkernel::MeshDataPtr meshData = getMeshData(sharedDataID());
		if (!meshData)
			return nullptr;

		trimesh::TriMesh* newMesh = new trimesh::TriMesh();
		*newMesh = *meshData->mesh;

		trimesh::fxform xf = trimesh::fxform(_globalMatrix().data());
		newMesh->flags.clear();
		trimesh::apply_xform(newMesh, trimesh::xform(xf));

		return TriMeshPtr(newMesh);
	}

	// create global mesh with normal changed, the global matrix may possibly involves in mirror transform
	TriMeshPtr ModelN::createGlobalMeshWithNormal()
	{
		cxkernel::MeshDataPtr meshData = getMeshData(sharedDataID());
		if (!meshData)
			return nullptr;

		TriMeshPtr newMesh(new trimesh::TriMesh());
		*newMesh = *meshData->mesh;

		trimesh::fxform globalXf = trimesh::fxform(_globalMatrix().data());
		newMesh->flags.clear();
		trimesh::apply_xform(newMesh.get(), trimesh::xform(globalXf));

		trimesh::fxform normalXf = qtuser_3d::qMatrix2Xform(modelNormalMatrix());

		creative_kernel::changeMeshFaceNormal(meshData->mesh.get(), newMesh.get(), normalXf);

		return newMesh;
	}

	std::vector<trimesh::vec3> ModelN::outline_path(bool global, bool debug)
	{
		cxkernel::NestDataPtr data = nestData();
		QQuaternion rotation = localQuaternion();
		QVector3D scale = localScale();

		trimesh::quaternion q = trimesh::quaternion(rotation.scalar(), rotation.x(), rotation.y(), rotation.z());
		trimesh::vec3 s = trimesh::vec3(scale.x(), scale.y(), scale.z());

		cxkernel::MeshDataPtr meshData = getKernel()->sharedDataManager()->getMeshData(sharedDataID());
		std::vector<trimesh::vec3> paths = global ? data->qPath(meshData->hull, q, s) : data->path(meshData->hull, s);

		if (debug)
		{
			QVector3D pos = localPosition();
			trimesh::fxform xf = trimesh::fxform::trans(trimesh::vec3(pos.x(), pos.y(), 0.0f));

			for (trimesh::vec3& point : paths)
				point = xf * point;
		}
		return paths;
	}

	std::vector<trimesh::vec3> ModelN::concave_path()
	{
		cxkernel::MeshDataPtr meshData = getMeshData(sharedDataID());
		TriMeshPtr mesh(meshData->mesh);
		trimesh::quaternion src_rotation = nestData()->nestRotation();
		bool src_dirty = nestData()->dirty();
		nestData()->setNestRotation(trimesh::quaternion(src_rotation.wp, -src_rotation.xp, -src_rotation.yp, -src_rotation.zp));
		QVector3D scale = localScale();
		std::vector<trimesh::vec3> concave = nestData()->concave_path(mesh, trimesh::vec3(scale.x(), scale.y(), scale.z()));
		nestData()->setNestRotation(src_rotation);
		nestData()->setDirty(src_dirty);
		return concave;
	}

	trimesh::quaternion ModelN::nestRotation()
	{
		return nestData()->nestRotation();
	}

	void ModelN::resetNestRotation()
	{
		QQuaternion rotation = localQuaternion();
		nestData()->setNestRotation(trimesh::quaternion(rotation.scalar(), rotation.x(), rotation.y(), rotation.z()));
	}

	void ModelN::dirtyNestData()
	{
		nestData()->setDirty(true);
	}

	cxkernel::NestDataPtr ModelN::nestData()
	{
		if (!m_nestData)
		{
			m_nestData.reset(new cxkernel::NestData());
			resetNestRotation();
		}
		return m_nestData;
	}

	void ModelN::copyNestData(ModelN* model)
	{
		if (model)
		{
			cxkernel::NestDataPtr data = model->nestData();
			cxkernel::NestDataPtr d = nestData();

			d->copyData(data.get());
		}
	}

	cxkernel::MeshDataPtr ModelN::modelNData()
	{
		cxkernel::MeshDataPtr meshData = getMeshData(sharedDataID());
		return meshData;
	}

	QVector3D ModelN::zeroLocation()
	{
		QVector3D loc = localPosition();
		qtuser_3d::Box3D box = globalSpaceBox();
		return loc - QVector3D(0.0f, 0.0f, box.min.z());
	}

	trimesh::point ModelN::getMinYZ()
	{
		trimesh::point pointMinZ(0, 0, 99999);

		cxkernel::MeshDataPtr meshData = getMeshData(sharedDataID());
		if (meshData && meshData->mesh)
		{
			trimesh::fxform xf = qtuser_3d::qMatrix2Xform(_globalMatrix());
			for (trimesh::point apoint : meshData->mesh->vertices)
			{
				apoint = xf * apoint;
				if (pointMinZ.z > apoint.z)
				{
					pointMinZ = apoint;
				}
				else if (pointMinZ.z == apoint.z && pointMinZ.y > apoint.y)
				{
					pointMinZ = apoint;
				}
			}
		}
		return pointMinZ;
	}

	float ModelN::localZ()
	{
		cxkernel::MeshDataPtr meshData = getMeshData(sharedDataID());
		return meshData ? meshData->localZ() : 0.0f;
	}

	void ModelN::setSerialName(const QString& serialName)
	{
		m_serialName = serialName;
	}

	QString ModelN::getSerialName()
	{
		return m_serialName;
	}

	void ModelN::setParentGroupStateDirty()
	{
		if (m_modelGroup)
		{
			m_modelGroup->setOutlinePathDirty();
		}
	}

	Qt3DRender::QGeometry* createGeometryFromMesh(trimesh::TriMesh* mesh)
	{
		cxkernel::GeometryData data;
		cxkernel::generateGeometryDataFromMesh(mesh, data);
		return qtuser_3d::GeometryCreateHelper::create(data.vcount, data.position, data.normal,
			data.texcoord, data.color);
	}

}
