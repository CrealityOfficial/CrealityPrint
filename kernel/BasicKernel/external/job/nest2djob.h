#ifndef AUTOLAYOUT_NEST2D_JOB_H
#define AUTOLAYOUT_NEST2D_JOB_H
#include "basickernelexport.h"
#include "data/modeln.h"

#include "qtusercore/module/job.h"
#include "cxkernel/wrapper/nest2djob.h"

namespace creative_kernel
{
    class LayoutItem : public QObject,
        public cxkernel::PlaceItem
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

    class BASIC_KERNEL_API Nest2DJob : public cxkernel::Nest2DJob
    {
        Q_OBJECT
    public:
        explicit Nest2DJob(QObject* parent = nullptr);
        virtual ~Nest2DJob();

        void setSpaceBox(const qtuser_3d::Box3D& box);
        void setSpecialModels(const QList<ModelN*>& models);
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

        QList<ModelN*> m_specialModels;
    };
}

#endif // AUTOLAYOUT_NEST2D_JOB_H
