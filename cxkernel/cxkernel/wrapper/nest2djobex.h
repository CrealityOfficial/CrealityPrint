#ifndef CXKERNEL_AUTOLAYOUT_NEST2DEX_JOB_H
#define CXKERNEL_AUTOLAYOUT_NEST2DEX_JOB_H
#include "cxkernel/data/header.h"

#include "qtusercore/module/job.h"
#include "ccglobal/tracer.h"

namespace nestplacer
{
    class BinExtendStrategy;
};

namespace cxkernel
{
    class PlaceItemEx;
	//
    struct  NestResultEx
    {
        trimesh::vec3 rt; // x, y translation  z rotation angle
        int binIndex = 0;
    };

    struct NestFixedItemInfo
    {
        int fixBinId;
        trimesh::box3 fixBinGlobalBox;
        std::vector<trimesh::vec3>  fixOutline;
    };

    enum class NestMode
    {
        NEST_LAYOUT,
        NEST_EXTEND_FILL
    };

    class CXKERNEL_API Nest2DJobEx : public qtuser_core::Job
    {
        Q_OBJECT
    public:
        Nest2DJobEx(QObject* parent = nullptr);
        virtual ~Nest2DJobEx();

        void setStrategy(nestplacer::BinExtendStrategy* strategy);
        void setBounding(const trimesh::box3& box);
        void setPanDistance(float distance);

		void setLayoutParameter(float modelSpacing, float platformSpacing, int angle, bool alignMove, bool outlineConcave);

        void setNestMode(int nestMode);

    protected:
        QString name();
        QString description();
        void failed();                        // invoke from main thread
        void successed(qtuser_core::Progressor* progressor);                     // invoke from main thread
        void work(qtuser_core::Progressor* progressor);    // invoke from worker thread

        virtual void beforeWork();
        virtual void afterWork();

        void doLayout(ccglobal::Tracer& tracer);

    protected:
        trimesh::box3 m_box;

        nestplacer::BinExtendStrategy* m_strategy { NULL };

        float m_panDistance;

        std::vector<NestResultEx> m_results;

        std::vector<NestFixedItemInfo> m_fixInfos;

        std::vector< std::vector<trimesh::vec3> > m_activeOutlines;

		float m_modelspacing;
		float m_platformSpacing;
		int m_angle;
		bool m_alignMove;
        bool m_outlineConcave;

        //the current operated bin index
        int m_curBinIndex;

        // ==== MultiBin related parameters ====

        NestMode m_nestMode;
        
    };
}

#endif // CXKERNEL_AUTOLAYOUT_NEST2DEX_JOB_H
