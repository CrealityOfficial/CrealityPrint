#ifndef CXKERNEL_AUTOLAYOUT_NEST2D_JOB_H
#define CXKERNEL_AUTOLAYOUT_NEST2D_JOB_H
#include "cxkernel/data/header.h"

#include "qtusercore/module/job.h"
#include "ccglobal/tracer.h"

namespace cxkernel
{
    struct NestResult
    {
        float x;
        float y;
        float r;
    };

    enum class NestPlaceType {
        CENTER_TO_SIDE,
        ALIGNMENT,
        ONELINE,
        CONCAVE,
        TOP_TO_BOTTOM,
        BOTTOM_TO_TOP,
        LEFT_TO_RIGHT,
        RIGHT_TO_LEFT,
        CONCAVE_ALL,
        NULLTYPE
    };

    class PlaceItem
    {
    public:
        virtual ~PlaceItem() {}

        virtual trimesh::vec3 position() = 0;
        virtual trimesh::box3 globalBox() = 0;
        virtual std::vector<trimesh::vec3> outLine(bool global = false) = 0;
        virtual std::vector<trimesh::vec3> outLine_concave() = 0;

        void setNestResult(const NestResult& _result) { result = _result; }
    public:
        NestResult result;
    };

    class CXKERNEL_API Nest2DJob : public qtuser_core::Job
    {
        Q_OBJECT
    public:
        Nest2DJob(QObject* parent = nullptr);
        virtual ~Nest2DJob();

        void setBounding(const trimesh::box3& box);
        void setNestType(NestPlaceType type);
        void setDistance(float distance);
        void setInsertItem(PlaceItem* item);
        void setItems(const QList<PlaceItem*>& items);
        void setLayoutNeedRotate(bool bNeedRotate);
    protected:
        QString name();
        QString description();
        void failed();                        // invoke from main thread
        void successed(qtuser_core::Progressor* progressor);                     // invoke from main thread
        void work(qtuser_core::Progressor* progressor);    // invoke from worker thread

        void insert(ccglobal::Tracer& tracer);
        void layoutAll(ccglobal::Tracer& tracer);
        virtual void beforeWork();
    protected:
        trimesh::box3 m_box;

        QList<PlaceItem*> m_items;
        PlaceItem* m_insert;

        NestPlaceType m_nestType;
        float m_distance;
        bool m_layoutNeedRotate;
    };
}

#endif // CXKERNEL_AUTOLAYOUT_NEST2D_JOB_H
