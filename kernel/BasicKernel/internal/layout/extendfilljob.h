#ifndef EXTEND_FILL_JOB_H
#define EXTEND_FILL_JOB_H
#include "basickernelexport.h"
#include "data/modeln.h"

#include "qtusercore/module/job.h"
#include "cxkernel/wrapper/nest2djobex.h"
#include "nestplacer/placer.h"
#include "layoutmanager.h"

namespace creative_kernel
{
    class BASIC_KERNEL_API ExtendFillJob : public qtuser_core::Job
    {
        Q_OBJECT
    public:
        explicit ExtendFillJob(QObject* parent = nullptr);
        virtual ~ExtendFillJob();

        void setLayoutParamInfo(const LayoutParameterInfo& paramInfo);

    protected:
        void successed(qtuser_core::Progressor* progressor) override; //                    // invoke from main thread
        void failed() override;//
        void prepare() override;//
        void work(qtuser_core::Progressor* progressor) override;    // invoke from worker thread

        void doExtendFill(ccglobal::Tracer& tracer);
        void beforeWork();
        void afterWork();
        void doPrepare();

        void buildFixWipeTowerInfo();
        void buildFixModelInfo();

        QString getExtendGroupName(const QString& exGroupName, int nameIdx);

        void setBounding(const trimesh::box3& box);

    Q_SIGNALS:
        void jobSuccessed();

    protected:
        int m_extendModelGroupId;
        int m_fillNum;

    private:
        trimesh::box3 m_box;

        float m_panDistance;

        std::vector<LayoutNestResultEx> m_results;

        std::vector<LayoutNestFixedItemInfo> m_fixInfos;

        std::vector< std::vector<trimesh::vec3> > m_activeOutlines;

        float m_modelspacing;
        float m_platformSpacing;
        int m_angle;
        bool m_alignMove;
        bool m_outlineConcave;

        //the current operated bin index
        int m_curBinIndex;

    };
}

#endif // EXTEND_FILL_JOB_H
