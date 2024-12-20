#include "wipetowerhandler.h"
#include "interface/camerainterface.h"
#include "qtuser3d/math/space3d.h"
#include "internal/render/wipetowerentity.h"
#include "internal/multi_printer/printermanager.h"
#include "interface/selectorinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/machineinterface.h"
#include "internal/multi_printer/printermanager.h"
#include "kernel/kernel.h"
#include "external/data/modelspaceundo.h"

#define PosMax 10000

namespace creative_kernel {
	
	WipeTowerHandler::WipeTowerHandler(QObject* parent)
		:QObject(parent)
		, m_mode(TMode::xy)
		, m_processEntity(nullptr)
	{

	}

	WipeTowerHandler::~WipeTowerHandler()
	{
	}

	void WipeTowerHandler::onLeftMouseButtonPress(QMouseEvent* event)
	{	
		m_processEntity = nullptr;
		m_startPressPos = QVector3D();
		m_startEntityPosOfLeftBottom = QVector3D();

		qtuser_3d::Pickable* pickable = checkPickable(event->pos(), nullptr);
		if (pickable == nullptr || pickable->selected() == false)
			return;

		m_startPressPos = process(event->pos());

		QList<WipeTowerEntity*>list = getPrinterMangager()->wipeTowerEntities();

		for (size_t i = 0; i < list.size(); i++)
		{
			WipeTowerEntity* sub = list.at(i);
			if (sub->pickable() == pickable)
			{
				m_processEntity = sub;
				m_startEntityPos = sub->position();
				m_startEntityPosOfLeftBottom = sub->positionOfLeftBottom();
				break;
			}
		}
	}
	
	void WipeTowerHandler::onLeftMouseButtonRelease(QMouseEvent* event)
	{
		if (m_processEntity != nullptr)
		{
			auto pm = getPrinterMangager();
			Printer* ptr = pm->findPrinterFromWipeTower(m_processEntity);
			if (ptr)
			{
				WipeTowerChange change;
				change.index = ptr->index();
				change.start = m_startEntityPosOfLeftBottom;
				change.end = m_processEntity->positionOfLeftBottom();
				getKernel()->modelSpaceUndo()->modifyWipeTower(change);
			}

			m_processEntity = nullptr;
			requestVisUpdate(true);


		}
	}
	
	void WipeTowerHandler::onLeftMouseButtonMove(QMouseEvent* event)
	{
		if (m_processEntity == nullptr || m_processEntity.data() == nullptr)
			return;

		QVector3D p = process(event->pos());
		QVector3D delta = clip(p - m_startPressPos);
		
		QVector3D dest = m_startEntityPos + delta;

		QVector3D better;
		bool can = m_processEntity->shouldTranslateTo(dest, better);
		if (can)
		{
			m_processEntity->translateTo(dest);
		}
		else {
			m_processEntity->translateTo(better);
		}

		//us::USettings* settings = creative_kernel::currentGlobalUserSettings();
		//settings->add("wipe_tower_x", getPrinterMangager()->getAllWipeTowerPosition(0, ","), true);
		//settings->add("wipe_tower_y", getPrinterMangager()->getAllWipeTowerPosition(1, ","), true);

		requestVisUpdate();
	}

	void WipeTowerHandler::onLeftMouseButtonClick(QMouseEvent* event)
	{

	}


	QVector3D WipeTowerHandler::process(const QPoint& point)
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

	void WipeTowerHandler::getProperPlane(QVector3D& planeCenter, QVector3D& planeDir, qtuser_3d::Ray& ray)
	{
		planeCenter = QVector3D(0.0f, 0.0f, 0.0f);
		
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

	QVector3D WipeTowerHandler::clip(const QVector3D& delta)
	{
		QVector3D clipDelta = delta;
		clipDelta.setZ(0.0f);

		return clipDelta;
	}
}
