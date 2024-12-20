#include "nest2djobex.h"
#include "qtusercore/module/progressortracer.h"

// #if USE_NESTPLACER
// #include "nestplacer/nestplacer.h"
// #endif

#include "nestplacer/placer.h"
#include "cxkernel/interface/constinterface.h"
#include "cxkernel/interface/cacheinterface.h"
#include "cxkernel/wrapper/placeitem.h"
#include <QDateTime>

using namespace nestplacer;

namespace cxkernel
{
    Nest2DJobEx::Nest2DJobEx(QObject* parent)
        : Job(parent)
        , m_modelspacing(1.0f)
        , m_platformSpacing(1.0f)
        , m_angle(0)
        , m_panDistance(10.0f)
        , m_outlineConcave(false)
        , m_curBinIndex(0)
        , m_nestMode(NestMode::NEST_LAYOUT)
    {

    }

    Nest2DJobEx::~Nest2DJobEx()
    {

    }

    void Nest2DJobEx::setStrategy(nestplacer::BinExtendStrategy* strategy)
    {
        m_strategy = strategy;
    }

    void Nest2DJobEx::setBounding(const trimesh::box3& box)
    {
        m_box = box;
        m_box.valid = true;
    }

    void Nest2DJobEx::setPanDistance(float distance)
    {
        m_panDistance = distance;
    }
	
	void Nest2DJobEx::setLayoutParameter(float modelSpacing, float platformSpacing, int angle, bool alignMove, bool outlineConcave)
	{
		m_modelspacing = modelSpacing;
		m_platformSpacing = platformSpacing;
		m_angle = angle;
		m_alignMove = alignMove;
        m_outlineConcave = outlineConcave;
	}

    void Nest2DJobEx::setNestMode(int nestMode)
    {
        m_nestMode = (NestMode)nestMode;
    }

    QString Nest2DJobEx::name()
    {
        return "qcxutil::Nest2DJobEx";
    }

    QString Nest2DJobEx::description()
    {
        return "qcxutil::Nest2DJobEx";
    }

    void Nest2DJobEx::failed()
    {
    }

    bool caseInsensitiveLessThan(PlaceItemEx* item1, PlaceItemEx* item2)
    {
        return false;
    }

    // invoke from main thread
    void Nest2DJobEx::successed(qtuser_core::Progressor* progressor)
    {
    }

    void Nest2DJobEx::work(qtuser_core::Progressor* progressor)
    {
        beforeWork();

        qtuser_core::ProgressorTracer tracer(progressor);

        doLayout(tracer);

        afterWork();
    }

    void Nest2DJobEx::beforeWork()
    {

    }

    void Nest2DJobEx::afterWork()
    {
    }

    void Nest2DJobEx::doLayout(ccglobal::Tracer& tracer)
    {
        if (m_strategy == NULL && NestMode::NEST_LAYOUT == m_nestMode)
        {
            m_strategy = new nestplacer::YDefaultBinExtendStrategy(m_box, m_panDistance);
        }

        PlacerParameter parameter;
		parameter.itemGap = m_modelspacing;
		parameter.binItemGap = m_platformSpacing;
		parameter.rotate = (m_angle ==0 ? false : true);
		parameter.rotateAngle = m_angle;
		parameter.needAlign = m_alignMove;
        parameter.concaveCal = m_outlineConcave;
        parameter.box = m_box;
        parameter.curBinId = m_curBinIndex;
        parameter.tracer = &tracer;
		
        if (!cxkernel::isReleaseVersion())
        {
            QString cacheName = cxkernel::createNewAlgCache("autolayout");
			parameter.fileName = cacheName.toLocal8Bit().constData();
        }

        std::vector<nestplacer::PlacerItem*> fixed;
        std::vector<nestplacer::PlacerItem*> actives;
        std::vector<PlacerResultRT> results;

        for (int i = 0; i < m_fixInfos.size(); i++)
        {
            PlaceItemEx* fixPlaceItem = new PlaceItemEx(m_fixInfos[i].fixOutline);
            fixPlaceItem->setFixPanIndex(m_fixInfos[i].fixBinId);
            fixed.push_back(fixPlaceItem);
        }

        for (int k = 0; k < m_activeOutlines.size(); k++)
        {
            actives.push_back(new PlaceItemEx(m_activeOutlines[k]));
        }

#ifdef DEBUG
        qint64 t1 = QDateTime::currentDateTime().toMSecsSinceEpoch();
#endif // DEBUG 

        if (NestMode::NEST_LAYOUT == m_nestMode)
        {
            place(fixed, actives, parameter, results, *m_strategy);
        }
        else if (NestMode::NEST_EXTEND_FILL == m_nestMode)
        {
            Q_ASSERT(actives.size() == 1);
            extendFill(fixed, actives[0], parameter, results);
        }
        

#ifdef DEBUG
        qint64 t1_1 = QDateTime::currentDateTime().toMSecsSinceEpoch();
        qInfo() << "==== place Time use " << (t1_1 - t1) << " ms";
#endif // DEBUG

        for (int i = 0; i < results.size(); i++)
        {
            NestResultEx aResult;
            aResult.binIndex = results[i].binIndex;
            aResult.rt = results[i].rt;
            m_results.push_back(aResult);
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
}