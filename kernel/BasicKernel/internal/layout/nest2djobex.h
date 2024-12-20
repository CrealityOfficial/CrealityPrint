#ifndef AUTOLAYOUT_NEST2D_JOB_EX_H
#define AUTOLAYOUT_NEST2D_JOB_EX_H
#include "basickernelexport.h"
#include "data/modeln.h"

#include "qtusercore/module/job.h"
#include "cxkernel/wrapper/nest2djobex.h"
#include "nestplacer/placer.h"
#include "layoutmanager.h"

namespace creative_kernel
{
    class MultiBinExtendStrategy;
    class LayoutItemEx;

    class BASIC_KERNEL_API Nest2DJobEx : public qtuser_core::Job
    {
        Q_OBJECT
    public:
        explicit Nest2DJobEx(QObject* parent = nullptr);
        virtual ~Nest2DJobEx();

        void setSpaceBox(const qtuser_3d::Box3D& box);//

        void setDoExtraFlag(bool extraFlag, int estimatePrinterSize);
        void setLayoutParameterInfo(const LayoutParameterInfo& paramInfo);

    protected:
        void doLayout(ccglobal::Tracer& tracer);
        void successed(qtuser_core::Progressor* progressor) override; //                    // invoke from main thread
        void failed() override;//
        void prepare() override;//

        void work(qtuser_core::Progressor* progressor) override;    // invoke from worker thread
        void beforeWork();
        void prepareBySelectFlag();

        void buildNestFixWipeTowerInfo();
        void buildNestFixModelGroupInfo();

        void buildSelectAllNestInfo();

        void getActualNeedPrintersNum(const QList<int>& binIndexList);

    Q_SIGNALS:
        void layoutSuccessed(const LayoutChangeInfo& changeInfo);
        void extraLayout(int estimatePrinterSize);

    protected:
        creative_kernel::MultiBinExtendStrategy* m_strategy;
        QList<LayoutItemEx*> m_activeObjects;
        QList<int> m_activeLayoutGroupIds;
        bool m_selectAll;

        // if true, meaning only layout groups in one printer
        bool m_layoutOnePrinterFlag;

        //the current operated bin index
        int m_priorBinIndex;

        int m_needPrintersCount;

        bool m_doExtraFlag;
        LayoutChangeInfo m_initSceneInfo;

        LayoutParameterInfo m_layoutParameterInfo;

    private:
        trimesh::box3 m_box;

        float m_panDistance;

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

#endif // AUTOLAYOUT_NEST2D_JOB_EX_H
