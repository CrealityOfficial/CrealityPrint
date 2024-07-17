#include "modeln.h"

#include "entity/purecolorentity.h"
#include "qtuser3d/utils/texturecreator.h"

#include "interface/reuseableinterface.h"
#include "interface/spaceinterface.h"
#include "interface/selectorinterface.h"
#include "interface/appsettinginterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/parameterinterface.h"
#include "interface/machineinterface.h"

#include "us/usettings.h"

#include "qtuser3d/math/angles.h"
#include "qtuser3d/geometry/geometrycreatehelper.h"
#include "cxkernel/data/trimeshutils.h"

#include "qtuser3d/math/space3d.h"

#include "qtuser3d/trimesh2/conv.h"

#include "cxkernel/data/raymesh.h"

#include "internal/render/modelnentity.h"

#include <QColor>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include "qtuser3d/refactor/xeffect.h"
#include "msbase/mesh/deserializecolor.h"
#include "external/interface/modelinterface.h"
#include "qtuser3d/refactor/xeffect.h"
#include "renderpass/zprojectrenderpass.h"

#include "kernel/kernel.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/printmachine.h"

#include "us/usettingwrapper.h"
#include "nestplacer/printpiece.h"
#include "slice/adaptiveslice.h"

#include "interface/renderinterface.h"

namespace creative_kernel
{
	ModelN::ModelN(QObject* parent)
		:Node3D(parent)
		, m_nestRotation(0)
		, m_localAngleStack({QVector3D()})
		, m_currentLocalDispalyAngle(0)
		, m_objectId(-1)
		, m_groupId(-1)
	{
		m_entity = new ModelNEntity();
		setRenderType(ViewRender);
		// m_entity->setEffect(getCachedModelEffect());

		m_lastUsedSetting = new us::USettings(this);
		setsetting(new us::USettings(this));

		m_visibility = true;
		setNozzle(0);

		m_modelOffscreenEffect = new qtuser_3d::XEffect(m_entity);
		{
			qtuser_3d::XRenderPass* rtPass = new qtuser_3d::XRenderPass("modeloffscreen", m_modelOffscreenEffect);
			rtPass->addFilterKeyMask("rt", 0);
			rtPass->setPassCullFace();
			rtPass->setPassDepthTest();
			m_modelOffscreenEffect->addRenderPass(rtPass);
		}

		m_modelCombindEffect = new qtuser_3d::XEffect(m_entity);
		{
			qtuser_3d::XRenderPass* viewPass = new qtuser_3d::XRenderPass("modelpreview", m_modelCombindEffect);
			viewPass->addFilterKeyMask("alpha", 0);
			viewPass->setPassBlend(Qt3DRender::QBlendEquationArguments::SourceAlpha, Qt3DRender::QBlendEquationArguments::OneMinusSourceAlpha);
			viewPass->setPassCullFace(Qt3DRender::QCullFace::Back);
			viewPass->setPassDepthTest();
			viewPass->setPassNoDepthMask();
			m_modelCombindEffect->addRenderPass(viewPass);

			qtuser_3d::XRenderPass* rtPass = new qtuser_3d::XRenderPass("modeloffscreen", m_modelCombindEffect);
			rtPass->addFilterKeyMask("rt", 0);
			rtPass->setPassCullFace();
			rtPass->setPassDepthTest();
			m_modelCombindEffect->addRenderPass(rtPass);

			qtuser_3d::XRenderPass* shadowPass = new qtuser_3d::ZProjectRenderPass(m_modelCombindEffect);
			shadowPass->addFilterKeyMask("modelzproject", 0);
			shadowPass->setEnabled(false);
			m_modelCombindEffect->addRenderPass(shadowPass);
		}

		QVariantList emptyList;
		setLayersColor(emptyList);

		ParameterManager* parameterManager = getKernel()->parameterManager();
		connect(parameterManager, &ParameterManager::parameterEdited, this, &ModelN::onGlobalSettingsChanged);
		connect(parameterManager, &ParameterManager::extruderClearanceRadiusChanged, this, [=](float radius)
			{
				m_sweepPathDirty = true;
				updateSweepAreaPath();
			});
		connect(this, &ModelN::defaultColorIndexChanged, this, &ModelN::updateSetting);
	}

	ModelN::~ModelN()
	{
		if (m_entity)
		{
			// 避免主线程删除entity与渲染线程冲突。
			setContinousRender();
			m_entity->setEnabled(false);
			delete m_entity;
			m_entity = nullptr;
			setCommandRender();
		}

		if (m_setting)
		{
			delete m_setting;
			m_setting = nullptr;
		}

		if (m_lastUsedSetting)
		{
			delete m_lastUsedSetting;
			m_lastUsedSetting = NULL;
		}
	}

	void ModelN::onGlobalMatrixChanged(const QMatrix4x4& globalMatrix)
	{
		m_entity->setPose(globalMatrix);
		qtuser_3d::Box3D box = globalSpaceBox();

		QMatrix4x4 matrix = globalMatrix;
		m_entity->updateBoxLocal(box, matrix);

    }

	void ModelN::onStateChanged(qtuser_3d::ControlState state)
	{
		setState((int)state);
		m_entity->setBoxVisibility(selected() ? true : false);
	}

	QVector3D ModelN::localRotateAngle()
	{
		return m_localAngleStack[m_currentLocalDispalyAngle];
	}

	QQuaternion ModelN::rotateByStandardAngle(QVector3D axis, float angle, bool updateCurFlag)
	{
		if (!updateCurFlag)
		{
			QVector3D currentLDR = m_localAngleStack[m_currentLocalDispalyAngle];
			QVector3D v = axis * angle;
			currentLDR += v;

			m_localAngleStack.erase(m_localAngleStack.begin() + m_currentLocalDispalyAngle + 1, m_localAngleStack.end());
			m_localAngleStack.push_back(currentLDR);
			m_currentLocalDispalyAngle++;
		}
		else if (m_currentLocalDispalyAngle > 0)
		{
			m_localAngleStack[m_currentLocalDispalyAngle] = m_localAngleStack[m_currentLocalDispalyAngle - 1] + axis * angle;
		}

		setLocalQuaternion(QQuaternion::fromAxisAndAngle(axis, angle) * m_localRotate);
		return m_localRotate;
	}

	void ModelN::updateDisplayRotation(bool redoFlag, int step)
	{
		if (redoFlag)
			m_currentLocalDispalyAngle += step;
		else
			m_currentLocalDispalyAngle -= step;

		m_currentLocalDispalyAngle = std::min(m_localAngleStack.size() - 1, std::max(0, m_currentLocalDispalyAngle));
	}

	void ModelN::resetRotate()
	{
		QVector3D currentLDR = m_localAngleStack[m_currentLocalDispalyAngle];
		currentLDR = QVector3D();

		m_localAngleStack.erase(m_localAngleStack.begin() + m_currentLocalDispalyAngle + 1, m_localAngleStack.end());
		m_localAngleStack.push_back(currentLDR);
		m_currentLocalDispalyAngle++;

		QQuaternion q = QQuaternion();
		setLocalQuaternion(q);
	}

	void ModelN::meshChanged()
	{

    }

	void ModelN::faceBaseChanged(int faceBase)
	{
		QPoint vertexBase(0, 0);
		vertexBase.setX(faceBase * 3);
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
		dirty();
		m_visibility = visibility;

		if (visibility)
		{
			if (m_entity->parent() != spaceEntity())
			{
				m_entity->setParent(spaceEntity());
			}
			m_entity->setEnabled(true);
		}
		else
		{
			if (m_entity->parent() != spaceEntity())
			{
				m_entity->setParent(spaceEntity());
			}
			m_entity->setEnabled(false);
		}
	}

	bool ModelN::isVisible()
	{
		return m_visibility;
	}

	Qt3DCore::QEntity* ModelN::getModelEntity()
	{
		return m_entity;
	}

	void ModelN::setModelEffect(qtuser_3d::XEffect* effect)
	{
		m_entity->setEffect(effect);
	}

	void ModelN::useVariableLayerHeightEffect()
	{
		setModelEffect(getCacheModelLayerEffect());
	}

	void ModelN::useDefaultModelEffect()
	{
		setModelEffect(getCachedModelEffect());
	}

	void ModelN::setNozzle(int nozzle)
	{
		if (m_entity != nullptr)
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
		QVector3D z = QVector3D(0.0f, 0.0f, -1.0f);
		QMatrix4x4 qmatrix = globalMatrix();
		QVector3D qNormal = normal.normalized();
		qNormal.normalize();
		QQuaternion qq = qtuser_3d::quaternionFromVector3D2(qNormal, z, true);

		QQuaternion oldRotate = localQuaternion();
		QQuaternion q = oldRotate;
		QVector3D oldCenter = globalSpaceBox().center();

		q = qq * q;
		setLocalQuaternion(q);

		qtuser_3d::Box3D box = globalSpaceBox();
		QVector3D offset = oldCenter - box.center();
		QVector3D zoffset = QVector3D(offset.x(), offset.y(), -box.min.z());
		QVector3D _lPosition = localPosition();

		position = _lPosition + zoffset;
		rotate = q;
	}

	void ModelN::setState(int state)
	{
		m_entity->setState(state);
	}

	int ModelN::getState()
	{
		return m_entity->getState();
	}

	/*void ModelN::setErrorState(bool error)
	{
		m_entity->setErrorState(error);
	}*/

	void ModelN::setBoxState(int state)
	{
		if(state == 0)
			m_entity->setBoxVisibility(false);
		else
			m_entity->setBoxVisibility(selected() ? true : false);
	}

	void ModelN::enterSupportStatus()
	{
		m_entity->enterSupportStatus();
	}

	void ModelN::leaveSupportStatus()
	{
		m_entity->leaveSupportStatus();
	}

	void ModelN::setSupportCos(float cos)
	{
		m_entity->setSupportCos(cos);
	}

	qtuser_3d::Box3D ModelN::boxWithSup()
	{
		qtuser_3d::Box3D box = Node3D::globalSpaceBox();
		return box;
	}

	void ModelN::SetModelViewClrState(qtuser_3d::ControlState statevalue,bool boxshow)
	{
		//m_entity->setState((float)ControlState::selected);

		setState((float)statevalue);
		m_entity->setBoxVisibility(boxshow);
	}

	trimesh::TriMesh* ModelN::mesh()
	{
		TriMeshPtr resultPtr = meshptr();
		return resultPtr ? &*resultPtr : nullptr;
	}

	TriMeshPtr ModelN::meshptr()
	{
		return m_data ? m_data->mesh : nullptr;
	}

	TriMeshPtr ModelN::globalMesh(bool needMergeColorMesh)
	{
		trimesh::TriMesh* mesh = nullptr;

		EngineType engine_type = getEngineType();
		//解析颜色
		if (needMergeColorMesh && hasColors() && engine_type == EngineType::ET_CURA)
		{
			std::vector<int> facet2Facets;
			mesh = msbase::mergeColorMeshes(m_data->mesh.get(), m_data->colors, facet2Facets);
			mesh->need_normals();
		}
		else
		{
			mesh = new trimesh::TriMesh();
			*mesh = *m_data->mesh;

			//add default color
			if (mesh->flags.size() != mesh->faces.size())
			{
				mesh->flags.clear();
				mesh->flags.resize(mesh->faces.size(),0);
			}
		}

		trimesh::apply_xform(mesh, trimesh::xform(qtuser_3d::qMatrix2Xform(globalMatrix())));
		return TriMeshPtr(mesh);
	}

	TriMeshPtr ModelN::localMesh(bool needMergeColorMesh)
	{
		trimesh::TriMesh* mesh = nullptr;
		EngineType engine_type = getEngineType();
		//解析颜色
		if (needMergeColorMesh && hasColors() && engine_type == EngineType::ET_CURA)
		{
			std::vector<int> facet2Facets;
			mesh = msbase::mergeColorMeshes(m_data->mesh.get(), m_data->colors, facet2Facets);
			mesh->need_normals();
		}
		else
		{
			mesh = new trimesh::TriMesh();
			*mesh = *m_data->mesh;
			//add default color
			if (mesh->flags.size() != mesh->faces.size())
			{
				mesh->flags.clear();
				mesh->flags.resize(mesh->faces.size(), 0);
			}
		}
		//trimesh::apply_xform(mesh, trimesh::xform(qtuser_3d::qMatrix2Xform(globalMatrix())));
		return TriMeshPtr(mesh);
	}

	int ModelN::getErrorEdges()
	{
		return  m_errorEdges;
	}
	int ModelN::getErrorNormals()
	{
		return m_errorNormals;
	}

	int ModelN::getErrorHoles()
	{
		return m_errorHoles;
	}
	int ModelN::getErrorIntersects()
	{
		return m_errorIntersects;
	}

	void ModelN::setErrorEdges(int value)
	{
		m_errorEdges = value;
	}

	void ModelN::setErrorNormals(int value)
	{
		m_errorNormals = value;
	}

	void ModelN::setErrorHoles(int value)
	{
		m_errorHoles = value;
	}

	void ModelN::setErrorIntersects(int value)
	{
		m_errorIntersects = value;
	}

	void ModelN::setTexture()
	{
		if (m_data && m_data->mesh)
		{
			TriMeshPtr mesh = m_data->mesh;

			int maptype = trimesh::Material::MapType::DIFFUSE;
			int bufferSize = mesh->map_bufferSize[maptype];
			unsigned char* imagedata = mesh->map_buffers[maptype];
			if (imagedata)
			{
				QImage aimage;
				aimage.loadFromData(imagedata, bufferSize);
				aimage = aimage.rgbSwapped();
				aimage = aimage.mirrored(false, true);
				Qt3DRender::QTexture2D* textureDiffuse = qtuser_3d::createFromImage(aimage);
				m_entity->setTDiffuse(textureDiffuse);
			}

			maptype = trimesh::Material::MapType::SPECULAR;
			bufferSize = mesh->map_bufferSize[maptype];
			imagedata = mesh->map_buffers[maptype];
			if (imagedata)
			{
				QImage aimage;
				aimage.loadFromData(imagedata, bufferSize);
				Qt3DRender::QTexture2D* textureSpecular = qtuser_3d::createFromImage(aimage);
				m_entity->setTAmbient(textureSpecular);
			}

			maptype = trimesh::Material::MapType::AMBIENT;
			bufferSize = mesh->map_bufferSize[maptype];
			imagedata = mesh->map_buffers[maptype];
			if (imagedata)
			{
				QImage aimage;
				aimage.loadFromData(imagedata, bufferSize);
				Qt3DRender::QTexture2D* textureAmbient = qtuser_3d::createFromImage(aimage);
				m_entity->setTSpecular(textureAmbient);
			}

			maptype = trimesh::Material::MapType::NORMAL;
			bufferSize = mesh->map_bufferSize[maptype];
			imagedata = mesh->map_buffers[maptype];
			if (imagedata)
			{
				QImage aimage;
				aimage.loadFromData(imagedata, bufferSize);
				Qt3DRender::QTexture2D* textureNormal = qtuser_3d::createFromImage(aimage);
				m_entity->setTNormal(textureNormal);
			}
		}
	}

	void ModelN::setData(cxkernel::ModelNDataPtr data)
	{
		if (m_data == data)
			return;

		// m_entity->setEnabled(false);
		setRenderData(std::make_shared<ModelNRenderData>(data));
		setObjectName(data->input.name);
		// m_entity->setEnabled(true);
	}

	void ModelN::setRenderData(ModelNRenderDataPtr renderData)
	{
		//if (m_renderData == renderData)
		//	return;

		// m_entity->setEnabled(false);
		m_data = renderData->data();
		m_renderData = renderData;

		if (m_colorIndex != m_data->defaultColor)
		{
			m_colorIndex = m_data->defaultColor;
			emit defaultColorIndexChanged();
		}

		if (m_renderData)
		{
			setObjectName(m_renderData->data()->input.name);

			// 切换geometry前先隐藏模型，避免主线程更改geometry与渲染线程冲突。
			setContinousRender();
			m_entity->setEnabled(false);
			m_entity->setGeometry(m_renderData->geometry(), Qt3DRender::QGeometryRenderer::Triangles, false);
			m_entity->setEnabled(true);
			setCommandRender();

			//bool use = (m_data && m_data->mesh->colors.size() > 0);
			//m_entity->setUseVertexColor(use);

			m_localBox = qtuser_3d::Box3D(QVector3D());
			if (m_data->mesh)
			{
				m_localBox = qtuser_3d::triBox2Box3D(m_data->mesh->bbox);
			}

			setCenter(m_localBox.center());
			m_data->resetHull();
		}

		updateSweepAreaPath();
		// m_entity->setEnabled(true);
	}

	ModelNRenderDataPtr ModelN::renderData()
	{
		return m_renderData;
	}

	cxkernel::ModelNDataPtr ModelN::data()
	{
		return m_data;
	}

	std::vector<trimesh::vec3> ModelN::getoutline_ObjectExclude()
	{
		std::vector<trimesh::vec3> outline = outline_path(true, false);
		return outline;
	}

	int ModelN::primitiveNum()
	{
		if (!m_data)
			return 0;

		if (m_data->colors.empty())
			return m_data->primitiveNum();
		else
			return m_data->spreadFaceCount;
	}

	void ModelN::setUnitType(UnitType type)
	{
		if (m_ut == type)
			return;
		m_ut = type;
	}

	ModelN::UnitType ModelN::unitType()
	{
		return m_ut;
	}

	void ModelN::setPose(const QMatrix4x4& pose)
	{
		m_entity->setPose(pose);
	}

	QMatrix4x4 ModelN::pose()
	{
		return m_entity->pose();
	}

	std::string ModelN::getColors2Facets(int index)
	{
		if (m_data && m_data->colors.size() > index && m_data->colors.size() > 0)
		{
			return m_data->colors[index];
		}
		return "";
	}

	void ModelN::setColors2Facets(const std::vector<std::string>& data)
	{
		cxkernel::ModelNDataPtr modelData = m_data->clone();
		modelData->colors = data;
		setData(modelData);

		emit colorsDataChanged();
	}

	std::vector<std::string> ModelN::getColors2Facets()
	{
		if (m_data)
		{
			if (m_data->colors.empty())
			{
				m_data->colors.resize(m_data->mesh->faces.size());
			}
			return m_data->colors;
		}

		return std::vector<std::string>();
	}

	std::vector<std::string> ModelN::getSeam2Facets()
	{
		if (m_data)
		{
			if (m_data->seams.empty())
			{
				m_data->seams.resize(m_data->mesh->faces.size());
			}
			return m_data->seams;
		}

		return std::vector<std::string>();
	}
	std::vector<std::string> ModelN::getSupport2Facets()
	{
		if (m_data)
		{
			if (m_data->supports.empty())
			{
				m_data->supports.resize(m_data->mesh->faces.size());
			}
			return m_data->supports;
		}

		return std::vector<std::string>();
	}

	void ModelN::setFacet2Facets(const std::vector<int>& facet2Facets)
	{
		cxkernel::ModelNDataPtr modelData = m_data->clone();
		modelData->spreadFaces = facet2Facets;
		setData(modelData);
	}

	int ModelN::getFacet2Facets(int faceId)
	{
		if (!m_data->spreadFaces.empty() && faceId > 0 && faceId < m_data->spreadFaces.size())
		{
			return m_data->spreadFaces[faceId];
		}

		return faceId;
	}

	bool ModelN::hasColors()
	{
		for (auto& color: m_data->colors)
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
		for (auto& seam: m_data->seams)
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
		for (auto& support: m_data->supports)
		{
			if (!support.empty())
			{
				return true;
			}
		}
		return false;
	}

	void ModelN::setDefaultColorIndex(int colorIndex)
	{
		if (m_colorIndex == colorIndex)
			return;

		m_colorIndex = colorIndex;

		cxkernel::ModelNDataPtr data = m_data->clone();
		data->defaultColor = m_colorIndex;
		m_data = data;

		setRenderData(std::make_shared<ModelNRenderData>(m_data));
		requestVisUpdate(true);
		//updateRender(false);

		dirty();
		emit dirtyChanged();
		emit defaultColorIndexChanged();
	}

	int ModelN::defaultColorIndex() const
	{
		return m_colorIndex;
	}

	void ModelN::updateRenderByMeshSpreadData(const std::vector<std::string>& colorData, bool updateCapture)
	{
		if (m_data == NULL || colorData.empty())
			return;

		setColors2Facets(colorData);

		// cxkernel::ModelNDataPtr data = m_data->clone();
		// data->colors = colorData;
		// m_data = data;
		// setRenderData(std::make_shared<ModelNRenderData>(m_data));
		requestVisUpdate(true);
	}

	void ModelN::updateRender(bool updateCapture)
	{
		if (m_data == NULL)
			return;

		// setRenderData(std::make_shared<ModelNRenderData>(m_data));

		m_renderData->updateRenderDataForced();
		setRenderData(m_renderData);
		//m_entity->setGeometry(createGeometry());
		requestVisUpdate(updateCapture);
	}

	//paint seam
	void ModelN::setSeam2Facets(const std::vector<std::string>& data)
	{
		cxkernel::ModelNDataPtr modelData = m_data->clone();
		modelData->seams = data;
		setData(modelData);

		emit seamsDataChanged();
	}

	//paint support
	void ModelN::setSupport2Facets(const std::vector<std::string>& data)
	{
		cxkernel::ModelNDataPtr modelData = m_data->clone();
		modelData->supports = data;
		setData(modelData);

		emit supportsDataChanged();
	}

	bool ModelN::isContainsMaterial(int materialIndex)
	{
		if (!m_data)
			return false;

		emit prepareCheckMaterial();

		return m_data->colorIndexs.contains(materialIndex);
	}

	void ModelN::setLayerHeightProfile(const std::vector<double>& layerHeightProfile)
	{
		m_layerHeightProfile = layerHeightProfile;

		ParameterManager* parameterManager = getKernel()->parameterManager();
		float minLayerHeight = parameterManager->currentMachine()->minLayerHeight();
		float maxLayerHeight = parameterManager->currentMachine()->maxLayerHeight();
		float uniqueLayerHeight = parameterManager->currentMachine()->currentProcessLayerHeight();

		auto layerHeight = creative_kernel::generateObjectLayers(m_layerHeightProfile, globalMesh().get());
		m_entity->setLayerHeightProfile(layerHeight, minLayerHeight, maxLayerHeight, uniqueLayerHeight);
		dirty();
		Q_EMIT adaptiveLayerDataChanged();
	}

	std::vector<double> ModelN::layerHeightProfile()
	{
		return m_layerHeightProfile;
	}

	void ModelN::setRenderType(int type)
	{
		if (!m_entity)
			return;

		if (m_renderType == type)
			return;

		m_renderType = type;
		if (type == NoRender)
		{
			//m_entity->setEffect(m_modelOffscreenEffect);
			m_entity->setEnabled(false);	
			return;
		}

		m_entity->setEnabled(true);
		if (type & ViewRender)
		{
			if (type & OffscreenRender)
				m_entity->setEffect(m_modelCombindEffect);
			else
				m_entity->setEffect(getCachedModelEffect());
		}
		else
		{
			if (type & OffscreenRender)
				m_entity->setEffect(m_modelOffscreenEffect);
			else
				m_entity->setEffect(m_modelOffscreenEffect);
		}
	}

	void ModelN::setLayersColor(const QVariantList& layersColor)
	{
		m_modelCombindEffect->setParameter("specificColorLayers[0]", layersColor);
		m_modelOffscreenEffect->setParameter("specificColorLayers[0]", layersColor);

		m_modelCombindEffect->setParameter("layersCount", layersColor.count());
		m_modelOffscreenEffect->setParameter("layersCount", layersColor.count());
	}
	
	void ModelN::setOffscreenRenderOffset(const QVector3D& offset)
	{
		//  QMatrix4x4 offsetMatrix;
		//  offsetMatrix.translate(offset);
		//  m_modelCombindEffect->setParameter("offsetMatrix", offsetMatrix);
		//  m_modelOffscreenEffect->setParameter("offsetMatrix", offsetMatrix);

		//
		m_modelCombindEffect->setParameter("offset1", offset);
		m_modelOffscreenEffect->setParameter("offset1", offset);

	}

	void ModelN::setBoxVisibility(bool visibility)
	{
		m_entity->setBoxEnabled(visibility);
	}

	void ModelN::setObjectId(int objId)
	{
		m_objectId = objId;
	}

	int ModelN::getObjectId()
	{
		return m_objectId;
	}

	void ModelN::setGroupId(int groupId)
	{
		m_groupId = groupId;
	}

	int ModelN::getGroupId()
	{
		return m_groupId;
	}

	cxkernel::ModelNDataPtr ModelN::getScaledUniformUnCheckData()
	{
		QMatrix4x4 scaleRotMat;
		scaleRotMat.setToIdentity();

		scaleRotMat.translate(m_localCenter);
		scaleRotMat.scale(m_localScale);
		scaleRotMat.rotate(m_localRotate);
		scaleRotMat.translate(-m_localCenter);

		TriMeshPtr mesh(new trimesh::TriMesh());
		*mesh = *(m_data->mesh);

		trimesh::apply_xform(mesh.get(), trimesh::xform(qtuser_3d::qMatrix2Xform(scaleRotMat)));

		cxkernel::ModelNDataPtr newMeshdata;
		if (mesh)
		{
			mesh->need_bbox();

			cxkernel::ModelCreateInput input;
			input.mesh = mesh;
			input.name = this->objectName();

			cxkernel::ModelNDataCreateParam param;
			param.toCenter = true;

			newMeshdata = cxkernel::createModelNData(input, nullptr, param);

			return newMeshdata;
		}

		Q_ASSERT(newMeshdata != nullptr);
		return nullptr;
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

	void ModelN::onSettingsChanged(const QString& key,us::USetting* setting)
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
		if (!m_isDirty)
		{
			m_isDirty = true;
			emit dirtyChanged();
		}
	}

	void ModelN::resetDirty()
	{
		if (m_isDirty)
		{
			m_isDirty = false;
			emit dirtyChanged();
		}
	}

	qtuser_3d::Box3D ModelN::calculateGlobalSpaceBox()
	{
		if (m_data && m_data->mesh)
		{
			QMatrix4x4 matrix = globalMatrix();
			return qtuser_3d::triBox2Box3D(m_data->calculateBox(qtuser_3d::qMatrix2Xform(matrix)));
		}
		return m_localBox;
	}

	qtuser_3d::Box3D ModelN::calculateGlobalSpaceBoxNoScale()
	{
		if (m_data && m_data->mesh)
		{
			QMatrix4x4 mtNoScale;
			mtNoScale.translate(m_localCenter);
			mtNoScale.rotate(m_localRotate);
			mtNoScale.translate(-m_localCenter);
			return qtuser_3d::triBox2Box3D(m_data->calculateBox(qtuser_3d::qMatrix2Xform(mtNoScale)));
		}
		return m_localBox;
	}

	bool ModelN::rayCheck(int primitiveID, const qtuser_3d::Ray& ray, QVector3D& collide, QVector3D* normal)
	{
		if (!m_data || !m_data->mesh)
			return false;

		trimesh::fxform xf = qtuser_3d::qMatrix2Xform(globalMatrix());

		cxkernel::Ray cRay(qtuser_3d::qVector3D2Vec3(ray.start), qtuser_3d::qVector3D2Vec3(ray.dir));
		trimesh::vec3 tposition, tnormal;
		bool result = cxkernel::rayMeshCheck(m_data->mesh.get(), xf, primitiveID, cRay, tposition, tnormal);
		if (result)
		{
			collide = qtuser_3d::vec2qvector(tposition);
			if(normal)
				*normal = qtuser_3d::vec2qvector(tnormal);
		}
		return result;
	}

	void ModelN::convex(std::vector<trimesh::vec3>& datas, bool origin)
	{
		if (m_data)
		{
			trimesh::fxform xf = qtuser_3d::qMatrix2Xform(globalMatrix());
			if (origin)
			{
				xf = trimesh::fxform::trans(- qtuser_3d::qVector3D2Vec3(localPosition()))
					* xf;
			}
			m_data->convex(xf, datas);
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

	trimesh::TriMesh* ModelN::createGlobalMesh()
	{
		if (!m_data)
			return nullptr;

		trimesh::fxform xf = trimesh::fxform(globalMatrix().data());

		trimesh::TriMesh* newMesh = new trimesh::TriMesh();
		*newMesh = *m_data->mesh;
		trimesh::apply_xform(newMesh, trimesh::xform(xf));
		return newMesh;
	}

	std::vector<trimesh::vec3> ModelN::outline_path(bool global, bool debug)
	{
		cxkernel::NestDataPtr data = nestData();
		QQuaternion rotation = localQuaternion();
		QVector3D scale = localScale();

		trimesh::quaternion q = trimesh::quaternion(rotation.scalar(), rotation.x(), rotation.y(), rotation.z());
		trimesh::vec3 s = trimesh::vec3(scale.x(), scale.y(), scale.z());

		std::vector<trimesh::vec3> paths = global ? data->qPath(m_data->hull, q, s) : data->path(m_data->hull, s);

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
		TriMeshPtr mesh(m_data->mesh);
		trimesh::quaternion src_rotation = nestData()->nestRotation();
		bool src_dirty = nestData()->dirty();
		nestData()->setNestRotation(trimesh::quaternion(src_rotation.wp, -src_rotation.xp, -src_rotation.yp, -src_rotation.zp));
		QVector3D scale = localScale();
		std::vector<trimesh::vec3> concave = nestData()->concave_path(mesh, trimesh::vec3(scale.x(), scale.y(), scale.z()));
		nestData()->setNestRotation(src_rotation);
		nestData()->setDirty(src_dirty);
		return concave;
	}

	std::vector<trimesh::vec3> ModelN::rsPath()
	{
		if (!m_rIsDirty && !m_sIsDirty)
		{
			return m_rsPath;
		}

		m_rsPath = outline_path(true, false);
		m_rIsDirty = false;
		m_sIsDirty = false;

		return m_rsPath;
	}

	const std::vector<trimesh::vec3>& ModelN::sweepAreaPath()
	{
		if (!m_rIsDirty && !m_sIsDirty && !m_sweepPathDirty) {
			return m_sweepPath;
		}
		std::vector<trimesh::vec3> convexData = rsPath();

		std::vector<trimesh::vec3> orbit = getKernel()->parameterManager()->extruderClearanceContour();
		m_sweepPath = nestplacer::sweepAreaProfile(convexData, orbit, trimesh::vec3(0.0f));

		m_sweepPathDirty = false;

		return m_sweepPath;
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

	cxkernel::ModelNDataPtr ModelN::modelNData()
	{
		return m_data;
	}

	void ModelN::adaptBox(const qtuser_3d::Box3D& box)
	{
		if (!box.valid)
			return;

		if (!m_data)
			return;

		trimesh::box3 _box = qtuser_3d::qBox32box3(box);
		trimesh::box3 _b = m_data->localBox();

		if (!_b.valid)
			return;

		trimesh::vec3 bsize = 0.9f * _box.size();
		trimesh::vec3 scale = bsize / _b.size();
		float s = scale.min();

		QVector3D qScale = QVector3D(s, s, s);
		QVector3D qTranslate = QVector3D(0.0f, 0.0f, bsize.z / 2.0f);

		setLocalPosition(qTranslate);
		setLocalScale(qScale);
	}

	void ModelN::adaptSmallBox(const qtuser_3d::Box3D& box)
	{
		if (!box.valid)
			return;

		if (!m_data)
			return;

		trimesh::box3 _box = qtuser_3d::qBox32box3(box);
		trimesh::vec3 bsize = 0.9f * _box.size();

		float s = 1000.0f;
		QVector3D qScale = QVector3D(s, s, s);
		QVector3D qTranslate = QVector3D(0.0f, 0.0f, bsize.z / 2.0f);

		setLocalPosition(qTranslate);
		setLocalScale(qScale);
	}

	QVector3D ModelN::zeroLocation()
	{
		QVector3D loc = localPosition();
		qtuser_3d::Box3D box = calculateGlobalSpaceBox();
		return loc - QVector3D(0.0f, 0.0f, box.min.z());
	}

	trimesh::point ModelN::getMinYZ()
	{
		trimesh::point pointMinZ(0, 0, 99999);

		if (m_data && m_data->mesh)
		{
			trimesh::fxform xf = qtuser_3d::qMatrix2Xform(globalMatrix());
			for (trimesh::point apoint : m_data->mesh->vertices)
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
		return m_data ? m_data->localZ() : 0.0f;
	}
	void ModelN::setSerialName(const QString& serialName)
	{
		m_serialName = serialName;
	}

	QString ModelN::getSerialName()
	{
		return m_serialName;
	}

	void ModelN::setLocalData(const trimesh::vec3& position, const QQuaternion& q, const trimesh::vec3& scale)
	{
		m_localPosition = qtuser_3d::vec2qvector(position);
		m_localRotate = q;
		m_localScale = qtuser_3d::vec2qvector(scale);

		m_localMatrixDirty = true;
		m_parentMatrixDirty = true;

		updateMatrix();
	}

	void ModelN::setOuterLinesColor(const QVector4D& color)
	{
		m_entity->setOuterLinesColor(color);
	}

	void ModelN::setOuterLinesVisibility(bool visible)
	{
		m_entity->setOuterLinesVisibility(visible);
	}

	void ModelN::setNozzleRegionVisibility(bool visible)
	{
		m_entity->setNozzleRegionVisibility(visible);
	}

	Qt3DRender::QGeometry* createGeometryFromMesh(trimesh::TriMesh* mesh)
	{
		cxkernel::GeometryData data;
		cxkernel::generateGeometryDataFromMesh(mesh, data);
		return qtuser_3d::GeometryCreateHelper::create(data.vcount, data.position, data.normal,
			data.texcoord, data.color);
	}

	void ModelN::onLocalPositionChanged(const QVector3D& newPosition)
	{
		QMatrix4x4 m;
		m.translate(newPosition.x(), newPosition.y(), 0.0f);
		m_entity->setLinesPose(m);
	}

	void ModelN::onLocalScaleChanged(const QVector3D& newScale)
	{
		m_sIsDirty = true;
	}

	void ModelN::onLocalQuaternionChanged(const QQuaternion& newQ)
	{
		m_rIsDirty = true;
	}

	void ModelN::updateSweepAreaPath()
	{
		if (!m_rIsDirty && !m_sIsDirty && !m_sweepPathDirty) {
			return;
		}

		m_entity->updateLines(sweepAreaPath());
	}
}
