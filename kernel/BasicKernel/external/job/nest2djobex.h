#ifndef AUTOLAYOUT_NEST2D_JOB_EX_H
#define AUTOLAYOUT_NEST2D_JOB_EX_H
#include "basickernelexport.h"
#include "data/modeln.h"
#include "data/undochange.h"

#include "qtusercore/module/job.h"
#include "cxkernel/wrapper/nest2djobex.h"
#include "nestplacer/placer.h"

namespace creative_kernel
{
    class LayoutItemEx : public QObject
    {
    public:
        LayoutItemEx(ModelN* model, bool layoutConcave = false, QObject* parent = nullptr);
        virtual ~LayoutItemEx();

        trimesh::vec3 position();
        trimesh::box3 globalBox();
        std::vector<trimesh::vec3> outLine(bool global = false, bool bTranslate = false);
        void setNestResult(const cxkernel::NestResultEx& nestResult);

    public:
        ModelN* model;
        cxkernel::NestResultEx nestResult;
        bool m_outlineConcave;
    };


    // this strategy only need the first box info, and need some translation after layout algorythm
    class  MultiBinExtendStrategyEx : public nestplacer::BinExtendStrategy
    {
    public:
        MultiBinExtendStrategyEx(int priorBin = 0);
        virtual ~MultiBinExtendStrategyEx();

        trimesh::box3 bounding(int index) const override;
        trimesh::box3 fixBounding(int fixBinId) const override;

        std::vector<int> getPriorIndexArray() const;
        int getPriorPrinterIndex(int resultIndex) const;

        void setEstimateBoxSize(int boxSize);

    public:
        qtuser_3d::Box3D m_box0;  // the first printer box in scene 
        int m_priorBinId = 0;
        bool m_hasWipeTowerFlag;
        int m_estimateBoxSizes;
        int extraPrinterCount;
        QList<qtuser_3d::Box3D> m_estimateBoxs;
    };


    class BASIC_KERNEL_API Nest2DJobEx : public cxkernel::Nest2DJobEx
    {
        Q_OBJECT
    public:
        explicit Nest2DJobEx(QObject* parent = nullptr);
        virtual ~Nest2DJobEx();

        void setSpaceBox(const qtuser_3d::Box3D& box);//

        void setSelectAllFlag(bool bSelectAll);
        void setActiveLayoutModels(QList<ModelN*> activeModels);
        void setPriorBinIndex(int priorBinIndex);
        void setPrintersNeedLayout(bool bNeedLayout);
        void setNeedAddToScene(bool bNeedAddToScene);
        void setDoExtraFlag(bool extraFlag, bool finalFlag, int estimatePrinterSize);
        void setSavedSceneInfo(const LayoutChangeInfo& initInfo);
    protected:
        void successed(qtuser_core::Progressor* progressor) override; //                    // invoke from main thread
        void failed() override;//
        void prepare() override;//

        void beforeWork() override;
        void afterWork() override;
        void prepareBySelectFlag();

        void buildNestFixWipeTowerInfo();
        void buildNestFixModelInfo(bool judgeSelect);

        void buildSelectAllNestInfo();
        void activeModelsAddToScene();
        void getActualNeedPrintersNum(const QList<int>& binIndexList);

    Q_SIGNALS:
        void secondLayoutSuccessed(const LayoutChangeInfo& changeInfo);
        void extraLayout(int estimatePrinterSize);
        void finalLayoutSuccessed();

    protected:
    /*******************/
        QList<LayoutItemEx*> m_objects;
        LayoutItemEx* m_object;

        QString m_md5Name = "";
        bool m_bCapture = false;
    /*******************/
        creative_kernel::MultiBinExtendStrategyEx* m_strategy;
        QList<ModelN*> m_importModels;
        QList<LayoutItemEx*> m_activeObjects;
        QList<ModelN*> m_activeLayoutModels;

        QStringList m_md5NameList;
        bool m_selectAll;

        //whether or not a new model is imported
        bool m_importFlag;

        bool m_printersNeedLayout;

        // after models layoutting, whether those models need to add into scene
        bool m_needAddToScene;

        //the current operated bin index
        int m_priorBinIndex;

        int m_needPrintersCount;
        int m_maxBinId;

        bool m_doExtraFlag;
        bool m_finalFlag;
        std::vector<cxkernel::NestResultEx> m_outResults;
        LayoutChangeInfo m_initSceneInfo;
    };
}

#endif // AUTOLAYOUT_NEST2D_JOB_EX_H
