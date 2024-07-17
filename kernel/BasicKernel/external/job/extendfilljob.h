#ifndef EXTEND_FILL_JOB_H
#define EXTEND_FILL_JOB_H
#include "basickernelexport.h"
#include "data/modeln.h"

#include "qtusercore/module/job.h"
#include "cxkernel/wrapper/nest2djobex.h"
#include "nestplacer/placer.h"

namespace creative_kernel
{
    class LayoutItemEx;


    class BASIC_KERNEL_API ExtendFillJob : public cxkernel::Nest2DJobEx
    {
        Q_OBJECT
    public:
        explicit ExtendFillJob(QObject* parent = nullptr);
        virtual ~ExtendFillJob();

        void setExtendModel(ModelN* extendModel);
        void setExtendBinIndex(int binIndex);

    protected:
        void successed(qtuser_core::Progressor* progressor) override; //                    // invoke from main thread
        void failed() override;//
        void prepare() override;//

        void beforeWork() override;
        void afterWork() override;
        void doPrepare();

        void buildFixWipeTowerInfo();
        void buildFixModelInfo();

    Q_SIGNALS:
        void jobSuccessed();

    protected:
        ModelN* m_extendModel;

        int m_fillNum;

    };
}

#endif // EXTEND_FILL_JOB_H
