#include "modeln.h"

#include "entity/purecolorentity.h"
#include "qtuser3d/utils/texturecreator.h"

#include "interface/reuseableinterface.h"
#include "interface/spaceinterface.h"
#include "interface/selectorinterface.h"
#include "interface/appsettinginterface.h"

#include "data/fdmsupportgroup.h"

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

namespace creative_kernel
{
	ModelN::ModelN(QObject* parent)
		:Node3D(parent)
		, m_fdmSupportGroup(nullptr)
		, m_nestRotation(0)
		, m_localAngleStack({QVector3D()})
		, m_currentLocalDispalyAngle(0)
	{
		m_entity = new ModelNEntity();
		m_entity->setEffect(getCachedModelEffect());

		m_setting = new us::USettings(this);

		m_visibility = true;
		m_detectAdd = true;
		setNozzle(0);
	}
	
	ModelN::~ModelN()
	{
		m_entity->deleteLater();
		m_setting->deleteLater();
	}

	void ModelN::onGlobalMatrixChanged(const QMatrix4x4& globalMatrix)
	{
		m_entity->setPose(globalMatrix);
		qtuser_3d::Box3D box = globalSpaceBox();

		QMatrix4x4 matrix = globalMatrix;
		m_entity->updateBoxLocal(box, matrix);

#if 0
		if(m_data)
		{
			std::vector<trimesh::vec3> convexData = outline_path();
			m_entity->updateLines(convexData);
		}
#endif
    }

	void ModelN::onStateChanged(qtuser_3d::ControlState state)
	{
		setState((float)state);
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

	void ModelN::setSupportsVisibility(bool visibility)
	{
		if (m_fdmSupportGroup)
			m_fdmSupportGroup->setVisibility(visibility);
	}

	us::USettings* ModelN::setting()
	{
		return m_setting;
	}

	void ModelN::setsetting(us::USettings* modelsetting)
	{
		m_setting = modelsetting;
	}

	void ModelN::setVisualMode(ModelVisualMode mode)
	{
		m_entity->setRenderMode((int)mode); 
	}

	void ModelN::setVisibility(bool visibility)
	{
		dirtyModelSpace();

		m_visibility = visibility;

		visibility ? spaceShow(m_entity) : spaceHide(m_entity);
		setSupportsVisibility(visibility);
		m_entity->update();
	}

	bool ModelN::isVisible()
	{
		return m_visibility;
	}

	Qt3DCore::QEntity* ModelN::getModelEntity()
	{
		return m_entity;
	}

	void ModelN::setCustomColor(QColor color)
	{
		m_entity->setCustomColor(color);
	}

	QColor ModelN::getCustomColor()
	{
		return m_entity->getCustomColor();
	}

	void ModelN::mirror(const QMatrix4x4& matrix, bool apply)
	{
		Node3D::mirror(matrix, apply);
		bool fanzhuan = isFanZhuan();
		m_entity->setFanZhuan((int)fanzhuan);
	}

	

	void ModelN::setNozzle(int nozzle)
	{
		if (m_entity != nullptr)
			m_entity->setNozzle(nozzle);
		
		m_setting->add("extruder_nr", QString("%1").arg(nozzle), true);
	}

	int ModelN::nozzle()
	{
		return m_setting->vvalue("extruder_nr").toInt();
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

	bool ModelN::hasFDMSupport()
	{
		return m_fdmSupportGroup && (m_fdmSupportGroup->fdmSupportNum() > 0);
	}

	void ModelN::setFDMSup(FDMSupportGroup* fdmSup)
	{
		m_fdmSupportGroup = fdmSup;
		m_fdmSupportGroup->setParent(this);
		m_fdmSupportGroup->setParent2Global(globalMatrix());
		m_fdmSupportGroup->setVisibility(true);
	}


	void ModelN::setState(float state)
	{
		m_entity->setState(state);
	}

	float ModelN::getState()
	{
		return m_entity->getState();
	}

	void ModelN::setErrorState(bool error)
	{
		m_entity->setErrorState(error);
	}

	void ModelN::setBoxState(int state)
	{
		if(state == 0)
			m_entity->setBoxVisibility(false);
		else
			m_entity->setBoxVisibility(selected() ? true : false);
	}

    FDMSupportGroup* ModelN::fdmSupport()
	{
        if (!m_fdmSupportGroup)
            buildFDMSupport();
		return m_fdmSupportGroup;
	}

	void ModelN::setFDMSuooprt(bool detectAdd)
	{
		m_detectAdd = detectAdd;
	}

	bool ModelN::getFDMSuooprt()
	{
		return m_detectAdd;
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

	/*void ModelN::setNeedCheckScope(int checkscope)
	{
		m_entity->setNeedCheckScope(checkscope);
	}*/

	qtuser_3d::Box3D ModelN::boxWithSup()
	{
		qtuser_3d::Box3D box = Node3D::globalSpaceBox();
		return box;
	}

    void ModelN::buildFDMSupport()
	{
        //bool bHasFdm = true;
		if (m_fdmSupportGroup) return;
		m_fdmSupportGroup = new FDMSupportGroup(this);
        m_fdmSupportGroup->setParent2Global(globalMatrix());
        m_fdmSupportGroup->setVisibility(true);
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

	TriMeshPtr ModelN::globalMesh()
	{
		trimesh::TriMesh* mesh = new trimesh::TriMesh();

		*mesh = *m_data->mesh;
		trimesh::apply_xform(mesh, trimesh::xform(qtuser_3d::qMatrix2Xform(globalMatrix())));
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
		setRenderData(std::make_shared<ModelNRenderData>(data));
	}

	void ModelN::setRenderData(ModelNRenderDataPtr renderData)
	{
		if (m_renderData == renderData)
			return;

		m_data = renderData->data();
		m_renderData = renderData;

		if (m_renderData)
		{
			setObjectName(m_renderData->data()->input.name);

			m_entity->setGeometry(m_renderData->geometry());

			bool use = (m_data && m_data->mesh->colors.size() > 0);
			m_entity->setUseVertexColor(use);

			m_localBox = qtuser_3d::Box3D(QVector3D());
			if (m_data->mesh)
			{
				m_localBox = qtuser_3d::triBox2Box3D(m_data->mesh->bbox);
			}

			setCenter(m_localBox.center());
			m_data->resetHull();
		}
	}

	ModelNRenderDataPtr ModelN::renderData()
	{
		return m_renderData;
	}

	cxkernel::ModelNDataPtr ModelN::data()
	{
		return m_data;
	}

	QMatrix4x4 ModelN::embedScaleNRot()
	{
		QMatrix4x4 scaleRotMat;

		scaleRotMat.translate(m_localCenter);
		scaleRotMat.rotate(m_localRotate);
		scaleRotMat.scale(m_localScale);
		scaleRotMat.translate(-m_localCenter);

		embedMatrix(scaleRotMat);

		resetLocalQuaternion(false);
		resetLocalScale(true);

		m_localBox = qtuser_3d::Box3D(QVector3D());
		if (m_data->mesh)
		{
			m_data->resetHull();
			m_data->updateRenderData();
			m_localBox = qtuser_3d::triBox2Box3D(m_data->calculateBox(trimesh::fxform()));
		}

		setCenter(m_localBox.center());

		return scaleRotMat;
	}

	void ModelN::embedMatrix(QMatrix4x4 mat)
	{
		Qt3DRender::QGeometry* modelGeo = m_entity->geometry();
		if (modelGeo)
		{
			// update geometry
			{
				//QVector<Qt3DRender::QAttribute*> attrList = modelGeo->attributes();
				//for each (Qt3DRender::QAttribute * attr in attrList)
				//{
				//	if (attr && attr->attributeType() == Qt3DRender::QAttribute::AttributeType::VertexAttribute)
				//	{
				//		QByteArray bufferData = attr->buffer()->data();
				//		// check float type
				//		if (bufferData.size() > 0 && bufferData.size() % 4 == 0)
				//		{
				//			trimesh::vec3* vertices = (trimesh::vec3*)bufferData.data();
				//			int vCount = bufferData.size() / (3 * 4);

				//			for (int i = 0; i < vCount; i++)
				//			{
				//				trimesh::vec3 newV = qcxutil::qMatrix2Xform(mat) * vertices[i];
				//				vertices[i].x = newV.x;
				//				vertices[i].y = newV.y;
				//				vertices[i].z = newV.z;
				//			}
				//		}

				//		attr->buffer()->setData(bufferData);

				//		break;
				//	}
				//}
			}

			// update modelNData
			{
				cxkernel::ModelNDataPtr data = m_renderData->data();
				if (data && data->mesh)
				{
					int vCount = data->mesh->vertices.size();

					for (int i = 0; i < vCount; i++)
					{
						data->mesh->vertices[i] = qtuser_3d::qMatrix2Xform(mat) * data->mesh->vertices[i];
					}
					m_renderData->updateRenderDataForced();
					m_entity->setGeometry(m_renderData->geometry());
				}
			}
		}
	}

	std::vector<trimesh::vec3> ModelN::getoutline_ObjectExclude()
	{
		std::vector<trimesh::vec3> outline = outline_path(true, false);
		return outline;
	}

	int ModelN::primitiveNum()
	{
		return m_data ? m_data->primitiveNum() : 0;
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

		trimesh::quaternion q = trimesh::quaternion(rotation.scalar(), -rotation.x(), -rotation.y(), -rotation.z());
		trimesh::vec3 s = trimesh::vec3(scale.x(), scale.y(), scale.z());

		std::vector<trimesh::vec3> paths;
		if (data->dirty() && hasFDMSupport())
		{
			TriMeshPtr mesh(new trimesh::TriMesh());

			*mesh = *m_data->hull;
			//trimesh::fxform ixf = trimesh::inv(trimesh::fxform(globalMatrix().data()));
			TriMeshPtr supportMesh(m_fdmSupportGroup->createFDMSupportMesh());
			//trimesh::apply_xform(supportMesh.get(), trimesh::xform(ixf));

			mesh->vertices.insert(mesh->vertices.end(), supportMesh->vertices.begin(), supportMesh->vertices.end());
			paths = global ? data->qPath(mesh, q, s) : data->path(mesh, s);
		}
		else
			paths = global ? data->qPath(m_data->hull, q, s) : data->path(m_data->hull, s);

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
	Qt3DRender::QGeometry* createGeometryFromMesh(trimesh::TriMesh* mesh)
	{
		cxkernel::GeometryData data;
		cxkernel::generateGeometryDataFromMesh(mesh, data);
		return qtuser_3d::GeometryCreateHelper::create(data.vcount, data.position, data.normal,
			data.texcoord, data.color);
	}
}
