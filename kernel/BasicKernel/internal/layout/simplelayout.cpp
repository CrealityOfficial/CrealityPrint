#include "simplelayout.h"
#include "layoutitemex.h"
#include "oneBinStrategy.h"
#include "layoutmanager.h"
#include "interface/spaceinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/modelgroupinterface.h"
#include "interface/machineinterface.h"
#include "interface/selectorinterface.h"
#include "interface/printerinterface.h"
#include "interface/layoutinterface.h"
#include "internal/multi_printer/printermanager.h"
#include "qtuser3d/trimesh2/conv.h"
#include "kernel/kernel.h"
#include "nestplacer/placer.h"
#include "internal/multi_printer/printermanager.h"
#include "qtusercore/module/progressortracer.h"
#include "cxkernel/interface/constinterface.h"
#include "cxkernel/interface/cacheinterface.h"
#include "cxkernel/wrapper/placeitem.h"
#include <QtCore/QDateTime>

namespace creative_kernel
{
    SimpleLayout::SimpleLayout(QObject* parent)
		: QObject(parent)
		, m_curBinIndex(0)
		, m_strategy(nullptr)
    {
    }

    SimpleLayout::~SimpleLayout()
    {
		if (m_strategy)
		{
			delete m_strategy;
			m_strategy = nullptr;
		}
    }

	void SimpleLayout::doLayout()
	{
		nestplacer::PlacerParameter parameter;
		parameter.itemGap = m_modelspacing;
		parameter.binItemGap = m_platformSpacing;
		parameter.rotate = false;
		parameter.rotateAngle = 0.0f;
		parameter.needAlign = false;
		parameter.concaveCal = false;
		parameter.box = m_box;
		parameter.curBinId = m_curBinIndex;

		if (!cxkernel::isReleaseVersion())
		{
			QString cacheName = cxkernel::createNewAlgCache("autolayout");
			parameter.fileName = cacheName.toLocal8Bit().constData();
		}

		//std::shared_ptr<trimesh::TriMesh>
		std::vector<nestplacer::PlacerItem*> fixed;
		std::vector<nestplacer::PlacerItem*> actives;
		std::vector<nestplacer::PlacerResultRT> results;

		for (int i = 0; i < m_fixInfos.size(); i++)
		{
			cxkernel::PlaceItemEx* fixPlaceItem = new cxkernel::PlaceItemEx(m_fixInfos[i].fixOutline);
			fixPlaceItem->setFixPanIndex(m_fixInfos[i].fixBinId);
			fixed.push_back(fixPlaceItem);
		}

		for (int k = 0; k < m_activeOutlines.size(); k++)
		{
			actives.push_back(new cxkernel::PlaceItemEx(m_activeOutlines[k]));
		}

#ifdef DEBUG
		qint64 t1 = QDateTime::currentDateTime().toMSecsSinceEpoch();
#endif // DEBUG 

		place(fixed, actives, parameter, results, *m_strategy);


#ifdef DEBUG
		qint64 t1_1 = QDateTime::currentDateTime().toMSecsSinceEpoch();
		qInfo() << "==== place Time use " << (t1_1 - t1) << " ms";
#endif // DEBUG

		m_layoutPos.clear();

		for (int i = 0; i < results.size(); i++)
		{
			if (results[i].binIndex < 0)
			{
				// fix bug : [ID1027475]
				qtuser_3d::Box3D pbox = qtuser_3d::triBox2Box3D(m_box);
				QVector3D boxMin = pbox.min;
				m_layoutPos.push_back(QVector3D(results[i].rt.x - boxMin.x() - 10.0f, results[i].rt.y - boxMin.y(), 0.0f));
			}
			else if(results[i].binIndex == 0)
				m_layoutPos.push_back(QVector3D(results[i].rt.x, results[i].rt.y, 0.0f));
			else
			{
				qtuser_3d::Box3D box0 = getPrinter(0)->globalBox();
				QVector3D someFixPos(50.0f, box0.size().y() + 50.0f, 0.0f);
				m_layoutPos.push_back(someFixPos);
			}
		}

		for (int i = 0; i < fixed.size(); i++)
		{
			if (fixed[i])
			{
				delete fixed[i];
				fixed[i] = nullptr;
			}
		}

		for (int j = 0; j < actives.size(); j++)
		{
			if (actives[j])
			{
				delete actives[j];
				actives[j] = nullptr;
			}
		}

	}

	void SimpleLayout::setLayoutParameterInfo(const LayoutParameterInfo& paramInfo)
	{
		if (paramInfo.priorBinIndex < 0 || paramInfo.priorBinIndex >= printersCount())
		{
			qWarning() << Q_FUNC_INFO << "==== priorBinIndex out of range;====";
			return;
		}

		m_curBinIndex = paramInfo.priorBinIndex;
		m_strategy = new creative_kernel::OneBinStrategy(m_curBinIndex);
		m_box =  qtuser_3d::qBox32box3(getPrinter(m_curBinIndex)->globalBox());

		m_modelspacing = paramInfo.modelSpacing;
		m_platformSpacing = paramInfo.platfromSpacing;

	}

	void SimpleLayout::setFixItemInfo(const std::vector<LayoutNestFixedItemInfo>& fixInfos)
	{
		m_fixInfos = fixInfos;
	}

	void SimpleLayout::setActiveItemInfo(const QList<CONTOUR_PATH>& activeOutlineInfos)
	{
		for (CONTOUR_PATH cPath : activeOutlineInfos)
		{
			m_activeOutlines.push_back(cPath);
		}

	}

	QList<QVector3D> SimpleLayout::getLayoutPos()
	{
		return m_layoutPos;
	}

}