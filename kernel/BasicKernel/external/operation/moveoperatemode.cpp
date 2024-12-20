#include "translateop.h"
#include <QQmlProperty>
#include "qtuser3d/math/space3d.h"
#include "qtuser3d/camera/cameracontroller.h"

#include "interface/spaceinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/camerainterface.h"
#include "interface/eventinterface.h"
#include "internal/utils/operationutil.h"
#include "interface/modelgroupinterface.h"

#include "qtuser3d/module/manipulatepickable.h"
#include "data/spaceutils.h"

float VALUE_MAX = 600;
namespace creative_kernel
{
	void getTSProperPlane(QVector3D& planeCenter, QVector3D& planeDir, qtuser_3d::Ray& ray, qtuser_3d::Box3D box, TMode m)
	{
		planeCenter = box.center();

		QList<QVector3D> dirs;
		if (m == TMode::x)  // x
		{
			dirs << QVector3D(0.0f, 1.0f, 0.0f);
			dirs << QVector3D(0.0f, 0.0f, 1.0f);
		}
		else if (m == TMode::y)
		{
			dirs << QVector3D(1.0f, 0.0f, 0.0f);
			dirs << QVector3D(0.0f, 0.0f, 1.0f);
		}
		else if (m == TMode::z)
		{
			dirs << QVector3D(1.0f, 0.0f, 0.0f);
			dirs << QVector3D(0.0f, 1.0f, 0.0f);
		}
		else if (m == TMode::xy)
		{
			planeDir = QVector3D(0.0, 0.0, 1.0);
		}
		else if (m == TMode::yz)
		{
			planeDir = QVector3D(1.0, 0.0, 0.0);
		}
		else if (m == TMode::zx)
		{
			planeDir = QVector3D(0.0, 1.0, 0.0);
		}
		else if (m == TMode::xyz)
		{
			planeDir = ray.dir;
		}
		else if (m == TMode::sp)
		{
			dirs << QVector3D(0.0f, 0.0f, 1.0f);
			dirs << QVector3D(0.0f, 0.0f, 1.0f);
		}

		float d = -1.0f;
		for (QVector3D& n : dirs)
		{
			float dd = QVector3D::dotProduct(n, ray.dir);
			if (qAbs(dd) > d)
			{
				planeDir = n;
				//			d = dd;
			}
		}
	}

	void getRProperPlane(QVector3D& planeCenter, QVector3D& planeDir, qtuser_3d::Ray& ray, qtuser_3d::Box3D box, TMode m)
	{
		planeCenter = box.center();
		if (m == TMode::x)  // x
		{
			planeDir = QVector3D(1.0f, 0.0f, 0.0f);
		}
		else if (m == TMode::y)
		{
			planeDir = QVector3D(0.0f, 1.0f, 0.0f);
		}
		else if (m == TMode::z)
		{
			planeDir = QVector3D(0.0f, 0.0f, 1.0f);
		}
		else if (m == TMode::xy)
		{
			planeDir = QVector3D(0.0, 0.0, 1.0);
		}
		else if (m == TMode::yz)
		{
			planeDir = QVector3D(1.0, 0.0, 0.0);
		}
		else if (m == TMode::zx)
		{
			planeDir = QVector3D(0.0, 1.0, 0.0);
		}
		else if (m == TMode::xyz)
		{
			planeDir = ray.dir;
		}
	}

	QVector3D operationProcessCoord(const QPoint& point, const qtuser_3d::Box3D& box, int op_type, TMode m)
	{
		qtuser_3d::Ray ray = visRay(point);
		QVector3D planeCenter;
		QVector3D planeDir;

		if (op_type == 0)
		{
			getTSProperPlane(planeCenter, planeDir, ray, box, m);
		}
		else if (op_type == 1)
		{
			getRProperPlane(planeCenter, planeDir, ray, box, m);
		}

		QVector3D c;
		qtuser_3d::lineCollidePlane(planeCenter, planeDir, ray, c);
		if (c.x() > VALUE_MAX) { c.setX(VALUE_MAX); }
		if (c.y() > VALUE_MAX) { c.setY(VALUE_MAX); }
		if (c.z() > VALUE_MAX) { c.setZ(VALUE_MAX); }
		if (c.x() < -VALUE_MAX) { c.setX(-VALUE_MAX); }
		if (c.y() < -VALUE_MAX) { c.setY(-VALUE_MAX); }
		if (c.z() < -VALUE_MAX) { c.setZ(-VALUE_MAX); }
		return c;
	}


	MoveOperateMode::MoveOperateMode(QObject* parent)
		:SceneOperateMode(parent)
		, m_mode(TMode::null)
	{
	}

	MoveOperateMode::~MoveOperateMode()
	{
	}


	void MoveOperateMode::onAttach()
	{
		addLeftMouseEventHandler(this);
		requestVisPickUpdate(true);
	}

	void MoveOperateMode::onDettach()
	{
		removeLeftMouseEventHandler(this);
	}

	void MoveOperateMode::onLeftMouseButtonPress(QMouseEvent* event)
	{
		m_mode = TMode::null;
		m_tempLocal = QVector3D();

		QList<ModelGroup*>list = creative_kernel::selectedGroups();
		if (list.size() > 0)
		{
			prepareMove_groups(event, list);
			if (m_mode == TMode::sp)
			{
				beginNodeSnap(list, QList<ModelN*>());
			}
		}
		else {
			auto models = selectedParts();
			if (!models.isEmpty())
			{
				auto groups = creative_kernel::findRelativeGroups(models);
				prepareMove_models(event, models);
				if (m_mode == TMode::sp)
				{
					beginNodeSnap(groups, models);
				}
			}
			
		}
	}

	void MoveOperateMode::onLeftMouseButtonRelease(QMouseEvent* event)
	{
		if (m_mode != TMode::sp || m_tempLocal.isNull())
			return;

		QList<ModelGroup*>list = creative_kernel::selectedGroups();
		if (list.size() > 0)
		{
			finishMove_groups(event, list);
		}
		else {
			finishMove_models(event, selectedParts());
		}

		endNodeSnap();
	}

	void MoveOperateMode::onLeftMouseButtonMove(QMouseEvent* event)
	{
		if (m_mode == TMode::null)
			return;

		QList<ModelGroup*>list = creative_kernel::selectedGroups();
		if (list.size() > 0)
		{
			onMove_groups(event, list);
		}
		else {
			onMove_models(event, selectedParts());
		}
	}

	void MoveOperateMode::onLeftMouseButtonClick(QMouseEvent* event)
	{
	}

	bool MoveOperateMode::shouldMultipleSelect()
	{
		return true;
	}

	QVector3D MoveOperateMode::process(const QPoint& point)
	{
		qtuser_3d::Ray ray = visRay(point);

		QVector3D planeCenter;
		QVector3D planeDir;
		getProperPlane(planeCenter, planeDir, ray);

		QVector3D c;
		qtuser_3d::lineCollidePlane(planeCenter, planeDir, ray, c);
		if (c.x() > PosMax) { c.setX(PosMax); }
		if (c.y() > PosMax) { c.setY(PosMax); }
		if (c.z() > PosMax) { c.setZ(PosMax); }
		return c;
	}

	void MoveOperateMode::getProperPlane(QVector3D& planeCenter, QVector3D& planeDir, qtuser_3d::Ray& ray)
	{
		planeCenter = QVector3D(0.0f, 0.0f, 0.0f);
		auto models = selectionms();

		QList<ModelGroup*>list = selectedGroups();
		if (list.size() > 0)
		{
			qtuser_3d::Box3D box = list.last()->globalBox();
			planeCenter = box.center();
		}
		else if (models.size() > 0) {
			qtuser_3d::Box3D box = models[models.size() - 1]->globalSpaceBox();
			planeCenter = box.center();
		}

		QList<QVector3D> dirs;
		if (m_mode == TMode::x)  // x
		{
			dirs << QVector3D(0.0f, 1.0f, 0.0f);
			dirs << QVector3D(0.0f, 0.0f, 1.0f);
		}
		else if (m_mode == TMode::y)
		{
			dirs << QVector3D(1.0f, 0.0f, 0.0f);
			dirs << QVector3D(0.0f, 0.0f, 1.0f);
		}
		else if (m_mode == TMode::z)
		{
			dirs << QVector3D(1.0f, 0.0f, 0.0f);
			dirs << QVector3D(0.0f, 1.0f, 0.0f);
		}
		else if (m_mode == TMode::xy)
		{
			planeDir = QVector3D(0.0, 0.0, 1.0);
		}
		else if (m_mode == TMode::yz)
		{
			planeDir = QVector3D(1.0, 0.0, 0.0);
		}
		else if (m_mode == TMode::zx)
		{
			planeDir = QVector3D(0.0, 1.0, 0.0);
		}
		else if (m_mode == TMode::xyz)
		{
			planeDir = ray.dir;
		}
		else if (m_mode == TMode::sp)
		{
			dirs << QVector3D(0.0f, 0.0f, 1.0f);
			dirs << QVector3D(0.0f, 0.0f, 1.0f);
		}

		float d = 0.0f;
		for (QVector3D& n : dirs)
		{
			float dd = QVector3D::dotProduct(n, ray.dir);
			if (qAbs(dd) > d)
			{
				d = qAbs(dd);
				planeDir = n;
			}
		}
	}

	QVector3D MoveOperateMode::clip(const QVector3D& delta)
	{
		QVector3D clipDelta = delta;
		if (m_mode == TMode::x)  // x
		{
			clipDelta.setY(0.0f);
			clipDelta.setZ(0.0f);
		}
		else if (m_mode == TMode::y)
		{
			clipDelta.setX(0.0f);
			clipDelta.setZ(0.0f);
		}
		else if (m_mode == TMode::z)
		{
			clipDelta.setY(0.0f);
			clipDelta.setX(0.0f);
		}

		return clipDelta;
	}

	void MoveOperateMode::setMode(TMode mode)
	{
		m_mode = mode;
	}

	void MoveOperateMode::resetSpacePoint(QPoint pos)
	{
		m_spacePoint = process(pos);
	}

	void MoveOperateMode::prepareMove_models(QMouseEvent* event, const QList<ModelN*>& models)
	{
		if (models.isEmpty())
			return;

		qtuser_3d::Pickable* pickable = checkPickable(event->pos(), nullptr);
		ModelN* model = dynamic_cast<ModelN*>(pickable);
		if (models.contains(model))
		{
			setMode(TMode::sp);
			resetSpacePoint(event->pos());
		}
	}

	void MoveOperateMode::onMove_models(QMouseEvent* event, const QList<ModelN*>& models)
	{
		if (models.isEmpty())
			return;

		QVector3D p = process(event->pos());
		QVector3D delta = clip(p - m_spacePoint);
		QVector3D moveDeltaWorld = delta - m_tempLocal;
		if (m_mode == TMode::sp || m_mode == TMode::xy)
		{
			moveDeltaWorld.setZ(0.0f);
		}
		m_tempLocal = delta;

		
		for (auto m : models)
		{
			QVector3D curr = m->globalSpaceBox().center();
			QVector3D next = curr + moveDeltaWorld;
			setModelToWorldPosition(m, next);
			m->dirty();
		}
		
		updateSpaceNodeRender(models, false);
		
		emit moving();
	}

	void MoveOperateMode::finishMove_models(QMouseEvent* event, const QList<ModelN*>& models)
	{
		if (models.isEmpty())
			return;

		m_mode = TMode::null;
	}

	void MoveOperateMode::prepareMove_groups(QMouseEvent* event, const QList<ModelGroup*>& groups)
	{
		qtuser_3d::Pickable* pickable = checkPickable(event->pos(), nullptr);
		ModelN* model = dynamic_cast<ModelN*>(pickable);
		qtuser_3d::ManipulatePickable* mani = dynamic_cast<qtuser_3d::ManipulatePickable *>(pickable);
		if ((model && groups.contains(model->getModelGroup())) || mani != nullptr)
		{
			setMode(TMode::sp);
			resetSpacePoint(event->pos());
		}
	}

	void MoveOperateMode::onMove_groups(QMouseEvent* event, const QList<ModelGroup*>& groups)
	{
		QVector3D p = process(event->pos());
		QVector3D delta = clip(p - m_spacePoint);
		QVector3D moveDelta = delta - m_tempLocal;
		if (m_mode == TMode::sp || m_mode == TMode::xy)
		{
			moveDelta.setZ(0.0f);
		}

		trimesh::dvec3 tDelta(moveDelta.x(), moveDelta.y(), moveDelta.z());

		m_tempLocal = delta;

		for (ModelGroup* m : groups)
		{
			//QVector3D newLocalPosition = m->localPosition() + moveDelta;
			//m->setLocalPosition(newLocalPosition, true);

			trimesh::xform xf = trimesh::xform::trans(tDelta);
			m->applyMatrix(xf);

			m->dirty();
		}
		
		updateSpaceNodeRender(groups, false);
		emit moving();
	}

	void MoveOperateMode::finishMove_groups(QMouseEvent* event, const QList<ModelGroup*>& list)
	{
		if (list.isEmpty())
			return;

		if (!m_tempLocal.isNull())
		{
			/*QList<QVector3D> starts;
			QList<QVector3D> ends;*/

			for (auto m : list)
			{
				//QVector3D originLocalPosition = m->localPosition() - m_tempLocal;
				//
				//starts << originLocalPosition;
				//
				//ends << m->localPosition();
			}

			//creative_kernel::moveModelGroups(list, starts, ends, true);

			m_mode = TMode::null;
		}
	}

	void MoveOperateMode::setModelToWorldPosition(ModelN* model, const QVector3D& wps)
	{
		ModelGroup* group = model->getModelGroup();
		trimesh::xform m = trimesh::inv(group->getMatrix());

		trimesh::dvec3 src_w = model->globalBoundingBox().center();
		trimesh::dvec3 src_l = m * src_w;
		trimesh::dvec3 dst_l = m * trimesh::dvec3(wps);

		trimesh::dvec3 tDelta = dst_l - src_l;
		trimesh::xform xf = trimesh::xform::trans(tDelta);
		model->applyMatrix(xf);
	}

}