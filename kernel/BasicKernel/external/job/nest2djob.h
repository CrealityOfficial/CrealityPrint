#ifndef AUTOLAYOUT_NEST2D_JOB_H
#define AUTOLAYOUT_NEST2D_JOB_H
#include "basickernelexport.h"
#include "data/modeln.h"
#include "nestplacer/nestplacer.h"

#include "qtusercore/module/job.h"
#include "qcxutil/util/nest2djob.h"

namespace creative_kernel
{
    struct NestResult
    {
        ModelN* model;
        nestplacer::TransMatrix trans;
    };

    class LayoutItem : public QObject,
        public qcxutil::PlaceItem
    {
    public:
        LayoutItem(ModelN* model, QObject* parent = nullptr);
        virtual ~LayoutItem();

        trimesh::vec3 position() override;
        trimesh::box3 globalBox() override;
        std::vector<trimesh::vec3> outLine(bool global) override;
        std::vector<trimesh::vec3> outLine_concave() override;

        ModelN* model;
    };

    class BASIC_KERNEL_API Nest2DJob : public qcxutil::Nest2DJob
    {
        Q_OBJECT
    public:
        explicit Nest2DJob(QObject* parent = nullptr);
        virtual ~Nest2DJob();

        void setInsert(ModelN* model);
    protected:
        void successed(qtuser_core::Progressor* progressor) override;                     // invoke from main thread
        void failed() override;
        void prepare() override;
    protected:
        QList<LayoutItem*> m_objects;
        LayoutItem* m_object;

        QString m_md5Name = "";
        bool m_bCapture = false;
    };
}

#endif // AUTOLAYOUT_NEST2D_JOB_H
