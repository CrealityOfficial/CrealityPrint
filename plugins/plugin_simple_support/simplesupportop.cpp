#include "simplesupportop.h"
#include "fdmsupporthandler.h"

#include "qtuser3d/math/angles.h"
#include "qtuser3d/math/space3d.h"

#include "data/modeln.h"
#include "data/gridcache.h"

#include "msbase/utils/box2dgrid.h"
#include "msbase/primitive/primitive.h"

#include <QtCore/QDebug>

#include "entity/alonepointentity.h"
#include "entity/lineentity.h"

#include "interface/visualsceneinterface.h"
#include "interface/camerainterface.h"
#include "interface/modelinterface.h"
#include "interface/selectorinterface.h"
#include "interface/spaceinterface.h"
#include "interface/eventinterface.h"

#include "data/fdmsupportgroup.h"
#include "data/fdmsupport.h"

#include  <cfloat>
#include "qtuser3d/trimesh2/conv.h"
#include "qtuser3d/refactor/xentity.h"
#include "renderpass/phongrenderpass.h"
#include "qtuser3d/refactor/xeffect.h"

using namespace creative_kernel;
SimpleSupportOp::SimpleSupportOp(QObject* parent)
	:SceneOperateMode(parent)
	, m_supportMode(SupportMode::esm_delete/*SupportMode::esm_add*/)//by TCJ
	, m_supportAngle(75.0f)
	, m_supportSize(2.0f)
{
	m_cache = new GridCache(this);
	m_debugEntity = new qtuser_3d::AlonePointEntity();
	//m_lineEntity = new qtuser_3d::LineEntity();
	//
	m_cylinderEntity = new qtuser_3d::XEntity();
	{
		
		qtuser_3d::PhongRenderPass* pass = new qtuser_3d::PhongRenderPass(m_cylinderEntity);
		pass->addFilterKeyMask("view", 0);

		qtuser_3d::XEffect* effect = new qtuser_3d::XEffect(m_cylinderEntity);
		effect->addRenderPass(pass);

		m_cylinderEntity->setEffect(effect);
	}


	QVector4D color = QVector4D(1.0f, 0.0f, 0.0f, 0.5f);
	m_cylinderEntity->setParameter("color", color);

	m_handler = new FDMSupportHandler(this);
}

SimpleSupportOp::~SimpleSupportOp()
{
	delete m_debugEntity;
}

void SimpleSupportOp::onAttach()
{
	qDebug() << "support onAttach";
	visShow(m_debugEntity);
	//visShow(m_lineEntity);
	//m_debugEntity->setEnabled(false);
	//m_lineEntity->setEnabled(false);
	//visShow(m_cylinderEntity);
	visHide(m_cylinderEntity);

	QList<ModelN*> models = modelns();
	m_cache->build(models);

	for (ModelN* model : models)
	{
		model->enterSupportStatus();
	}

	addLeftMouseEventHandler(this);
	addHoverEventHandler(this);

	addSelectTracer(this);
    disableReverseSelect(true);
	int baseFace = 40000000;
	for (ModelN* model : models)
	{
		model->fdmSupport()->setFaceBase(baseFace);
		baseFace += 10000000;
	}

	m_handler->setPickSource(visPickSource());

	requestVisUpdate(true);
	m_bShowPop = true;
}

void SimpleSupportOp::onDettach()
{
	qDebug() << "support onDettach";
	visHide(m_debugEntity);
	//visHide(m_lineEntity);
	visHide(m_cylinderEntity);

	QList<ModelN*> models = modelns();
	for (ModelN* model : models)
	{
		model->leaveSupportStatus();
	}

	m_cache->clear();

	removeSelectorTracer(this);
	disableReverseSelect(false);
	removeLeftMouseEventHandler(this);
	removeHoverEventHandler(this);

	m_handler->clear();
	requestVisUpdate(true);
	m_bShowPop = false;
}

void SimpleSupportOp::onSelectionsChanged()
{
}

void SimpleSupportOp::selectChanged(qtuser_3d::Pickable* pickable)
{
}

void SimpleSupportOp::changeShaders(const QString& name)
{
}

qtuser_3d::Pickable* SimpleSupportOp::surfaceXy(const QPoint& pos, int* primitiveID)
{
	qtuser_3d::Pickable* pickable = checkPickable(pos, primitiveID);
	return pickable;
}

void SimpleSupportOp::onLeftMouseButtonClick(QMouseEvent* event)
{
	qDebug() << "support onLeftMouseButtonClick";

	if (m_supportSize > -0.0001f && m_supportSize < 0.0001f)
	{
		//֧�Ŵ�СΪ0ʱ���޷��ֶ�����֧��
		return;
	}

	auto checkCollide = [&event](unsigned primitiveID, creative_kernel::ModelN* model)->QVector3D {
		QVector3D collide;
		model->rayCheck(primitiveID, visRay(event->pos()), collide, nullptr);
		return collide;
	};

	int primitiveID = 0;
	qtuser_3d::Pickable* pickable = surfaceXy(event->pos(), &primitiveID);

	QVector3D c;
	if (pickable && m_supportMode == SupportMode::esm_add)
	{
		std::vector<msbase::VerticalSeg> tcollides;
		ModelN* m = qobject_cast<ModelN*>(pickable);
		msbase::Box2DGrid* tree = nullptr;

		float supportSize = m_supportSize;
		//qtuser_3d::Box3D box = m->globalSpaceBox();
		//QVector3D boxSize = box.size();
		//float dmax = std::max(boxSize.x(), boxSize.y()) * 0.01f;
		//if (supportSize < dmax)
		//{
		//	supportSize = dmax;
		//}
			
		if (m)
		{
			c = checkCollide(primitiveID, m);
			tree = m_cache->grid(m);

			if (tree && !tree->checkFace(primitiveID))
			{
				tree = nullptr;
			}
		}
		else
		{
			qtuser_3d::Ray ray = visRay(event->pos());
			QVector3D pc(0.0f, 0.0f, 0.0f);
			QVector3D pn(0.0f, 0.0f, 1.0f);
			QVector3D xyz;
			if (lineCollidePlane(pc, pn, ray, xyz))
			{
				m = m_cache->check(trimesh::vec2(xyz.x(), xyz.y()));
				tree = m_cache->grid(m);
				c = xyz;
			}
		}

		if (tree)
		{
			trimesh::vec3 tc(c.x(), c.y(), c.z());
			tree->check(tcollides, tc, primitiveID);
		}

		if (tcollides.size() == 1 && tree)
		{
			trimesh::fxform xf = qtuser_3d::qMatrix2Xform(m->globalMatrix());
			trimesh::fxform invXf = trimesh::inv(xf);

			msbase::VerticalSeg& seg = tcollides.at(0);
			FDMSupportParam param;
			param.bottom = invXf * seg.b;
			param.top = invXf * seg.t;
			QVector3D s = m->localScale();
			s = QVector3D(supportSize, supportSize, supportSize) / s;
			param.radius = std::min(s.x(), s.y()) * sqrtf(2.0f) * 0.9f / 2.0f;

			FDMSupportGroup* supportGroup = m->fdmSupport();
			supportGroup->createSupport(param);
			dirtyModelSpace();
		}
	}
	else if (m_supportMode == SupportMode::esm_delete)
	{
		QVector3D debugPosition;
		
		QList<ModelN*> selections = selectionms();
		FDMSupport* support = m_handler->check(event->pos(), debugPosition);
		if (support)
		{
			//FDMSupportGroup* supportGroup = qobject_cast<FDMSupportGroup*>(support->parent());
			for (int i = 0; i < selections.size(); i++)
			{
				FDMSupportGroup* supportGroup = selections.at(i)->fdmSupport();

				QList<FDMSupport*> listSuport = supportGroup->FDMSupports();

				if (listSuport.contains(support))
				{
					supportGroup->removeSupport(support);
					dirtyModelSpace();
				}
			}

			m_handler->reset();
		}
	}

	requestVisUpdate(true);
}

void SimpleSupportOp::onHoverMove(QHoverEvent* event)
{
	//-------by TCJ ------
	visHide(m_cylinderEntity);
	if (m_supportSize > -0.0001f && m_supportSize < 0.0001f)
	{
		return;
	}
	auto checkCollide = [&event](unsigned primitiveID, creative_kernel::ModelN* model)->QVector3D {
		QVector3D collide;
		model->rayCheck(primitiveID, visRay(event->pos()), collide, nullptr);
		return collide;
	};

	int primitiveID = 0;
	qtuser_3d::Pickable* pickable = surfaceXy(event->pos(), &primitiveID);

	QVector3D c;
	if (pickable && m_supportMode == SupportMode::esm_add)
	{
		std::vector<msbase::VerticalSeg> tcollides;
		ModelN* m = qobject_cast<ModelN*>(pickable);
		msbase::Box2DGrid* tree = nullptr;

		float supportSize = m_supportSize;
		
		if (m)
		{
			c = checkCollide(primitiveID, m);
			tree = m_cache->grid(m);

			if (tree && !tree->checkFace(primitiveID))
			{
				tree = nullptr;
			}
		}
		else
		{
			qtuser_3d::Ray ray = visRay(event->pos());
			QVector3D pc(0.0f, 0.0f, 0.0f);
			QVector3D pn(0.0f, 0.0f, 1.0f);
			QVector3D xyz;
			if (lineCollidePlane(pc, pn, ray, xyz))
			{
				m = m_cache->check(trimesh::vec2(xyz.x(), xyz.y()));
				tree = m_cache->grid(m);
				c = xyz;
			}
		}

		if (tree)
		{
			trimesh::vec3 tc(c.x(), c.y(), c.z());
			tree->check(tcollides, tc, primitiveID);
		}

		if (tcollides.size() == 1 && tree)
		{
			trimesh::fxform xf = qtuser_3d::qMatrix2Xform(m->globalMatrix());
			trimesh::fxform invXf = trimesh::inv(xf);

			msbase::VerticalSeg& seg = tcollides.at(0);
			QVector3D s = m->localScale();
			//s = QVector3D(supportSize, supportSize, supportSize) / s;
			s = QVector3D(supportSize, supportSize, supportSize);
			float cylinderRadius = std::min(s.x(), s.y()) * sqrtf(2.0f) * 0.9f / 2.0f;

			visShow(m_cylinderEntity);
			if (m_cylinderEntity->isEnabled())
			{
				m_cylinderMesh.reset(msbase::createCylinderMesh(seg.t, seg.b, cylinderRadius, 4/*30*/, 45));
				Qt3DRender::QGeometry* geometry = creative_kernel::createGeometryFromMesh(m_cylinderMesh.get());
				m_cylinderEntity->setGeometry(geometry);
			}
		}
	}
	else if (m_supportMode == SupportMode::esm_delete)
	{
		QVector3D debugPosition;
		FDMSupport* support = m_handler->hover(event->pos(), debugPosition);
		if (support)
		{
			visShow(m_debugEntity);
			m_debugEntity->setColor(QVector4D(1.0f, 0.0f, 0.0f, 1.0f));
		}
		m_debugEntity->updateGlobal(debugPosition);
	}

	requestVisUpdate(false);
}

bool SimpleSupportOp::hasSupport()
{
	QList<creative_kernel::ModelN*> selections = selectionms();
	foreach(creative_kernel::ModelN * model, selections)
	{
		if (model->hasFDMSupport())
		{
			FDMSupportGroup* p_support = model->fdmSupport();
			if (p_support->fdmSupportNum())
			{
				return true;
			}
		}
	}
	return false;
}

void SimpleSupportOp::setAddMode()
{
	m_supportMode = SupportMode::esm_add;
}

void SimpleSupportOp::setDeleteMode()
{
	m_supportMode = SupportMode::esm_delete;
}

void SimpleSupportOp::deleteAllSupport()
{
	QList<ModelN*> selections = selectionms();
//	if (selections.size() > 0)
	for(int i = 0; i < selections.size(); i++)
	{
		FDMSupportGroup* supportGroup = selections.at(i)->fdmSupport();
		supportGroup->clearSupports();
		m_handler->reset();
	}
	dirtyModelSpace();
	requestVisUpdate(true);
}

void SimpleSupportOp::setSupportAngle(float angle)
{
	m_supportAngle = angle;

	// 有乘除法或者三角函数计算时最好采用双精度，尽量不要用单精度去存储中间结果，以避免精度丢失，进而对临界值的计算造成影响
    //float minOverhangCos = cosf(M_PIf * (90.0f - angle) / 180.0f);
	float minOverhangCos = cos(M_PIl * (90.0 - angle) / 180.0);

	QList<ModelN*> models = selectionms();
    for(ModelN * model : models)
	{
		model->setSupportCos(minOverhangCos);
	}

	requestVisUpdate(true);
}

void SimpleSupportOp::setSupportSize(float size)

{
#ifdef __APPLE__
	if (1 == isnan(size))
#else
	if (1 == std::isnan(size))
#endif
	{
		size = 0.0f;
	}
	m_supportSize = size;
	if (m_supportSize > -0.0001f && m_supportSize < 0.0001f) return;
	if (m_supportSize < 0.1f) m_supportSize = 2.0f;
	if (m_supportSize > 20.0f) m_supportSize = 20.0f;
}

void SimpleSupportOp::autoSupport(bool platform)
{
	QList<creative_kernel::ModelN*> models = selectionms();
	if (models.size() == 0) return;

	for (int i = 0; i < models.size(); i++)
	{
		ModelN* m = models.at(i);

		float supportSize = m_supportSize;

		std::vector<msbase::VerticalSeg> supports;
		msbase::Box2DGrid* grid = m_cache->grid(m);
		if (grid)
		{
			grid->buildGlobalProperties();
			grid->autoSupport(supports, supportSize, m_supportAngle, platform);
			size_t FaceSupportsize = supports.size();
			grid->autoSupportOfVecAndSeg(supports, supportSize, platform, true);

			size_t size = supports.size();
			qDebug() << "Auto add supports " << size;

			for (int i = 0; i < size; ++i)
			{
				trimesh::fxform xf = qtuser_3d::qMatrix2Xform(m->globalMatrix());
				trimesh::fxform invXf = trimesh::inv(xf);

				msbase::VerticalSeg& seg = supports.at(i);
				FDMSupportParam param;
				param.bottom = invXf * seg.b;
				param.top = invXf * seg.t;

				QVector3D s = m->localScale();
				if (i < FaceSupportsize)
					s = QVector3D(supportSize, supportSize, supportSize) / s;
				else
					s = QVector3D(1.5 * supportSize, 1.5 * supportSize, 1.5 * supportSize) / s;
				param.radius = std::min(s.x(), s.y()) * sqrtf(2.0f) * 0.9f / 2.0f;

				FDMSupportGroup* supportGroup = m->fdmSupport();
				supportGroup->createSupport(param);
			}
		}
	}

	dirtyModelSpace();
	requestVisUpdate(true);
}

bool SimpleSupportOp::getShowPop()
{
	return m_bShowPop;
}
