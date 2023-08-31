#include "nest2djob.h"
#include "interface/spaceinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/modelinterface.h"
#include "interface/machineinterface.h"
#include "utils/modelpositioninitializer.h"

#include "qcxutil/trimesh2/conv.h"
#include "polygonLib/polygonLib.h"

namespace creative_kernel
{
    LayoutItem::LayoutItem(ModelN* _model, QObject* parent)
        : QObject(parent)
        , model(_model)
    {

    }

    LayoutItem::~LayoutItem()
    {

    }

    trimesh::vec3 LayoutItem::position()
    {
        QVector3D pos = model->localPosition();
        return trimesh::vec3(pos.x(), pos.y(), pos.z());
    }

    trimesh::box3 LayoutItem::globalBox()
    {
        qtuser_3d::Box3D gbx = model->globalSpaceBox();
        trimesh::box3 gbx3;
        gbx3.min = gbx.min;
        gbx3.max = gbx.max;
        return gbx3;
    }

    std::vector<trimesh::vec3> LayoutItem::outLine(bool global)
    {
        return model->outline_path(global);
    }

    std::vector<trimesh::vec3> LayoutItem::outLine_concave()
    {
        return model->concave_path();
    }

    Nest2DJob::Nest2DJob(QObject* parent)
        : qcxutil::Nest2DJob(parent)
        , m_object(nullptr)
    {
        qtuser_3d::Box3D bbx = baseBoundingBox();
        trimesh::box3 bbx3;
        bbx3.min = bbx.min;
        bbx3.max = bbx.max;
        setBounding(bbx3);
    }

    Nest2DJob::~Nest2DJob()
    {
    }

    void Nest2DJob::setInsert(ModelN* model)
    {
        m_object = new LayoutItem(model, this);
        setInsertItem(m_object);
        if (m_object)
        {
            m_md5Name = model->modelNData()->input.fileName;
            m_bCapture = true;
        }
    }

    void Nest2DJob::failed()
    {
        if (m_object)
            m_object->model->setParent(this);
    }

    void Nest2DJob::prepare()
    {
        const QList<ModelN*>& models = modelns();
        QList<qcxutil::PlaceItem*> items;
        for (ModelN* model : models)
        {
            LayoutItem* item = new LayoutItem(model, this);
            m_objects.append(item);
            items.append(item);
        }

        setItems(items);
    }

    bool caseInsensitiveLessThan(ModelN* s1, ModelN* s2)
    {
        qtuser_3d::Box3D s1box = s1->globalSpaceBox();
        qtuser_3d::Box3D s2box = s2->globalSpaceBox();
        return  s1box.size().z() > s2box.size().z();
    }

    // invoke from main thread
    void Nest2DJob::successed(qtuser_core::Progressor* progressor)
    {
        QList<QVector3D> d_ts;
        QList<QQuaternion> d_qs;

        QList<QVector3D> s_ts;
        QList<QQuaternion> s_qs;

        QList<ModelN*> models;


        auto f = [&d_ts, &d_qs, &s_ts, &s_qs, &models](LayoutItem* item) {
            const qcxutil::NestResult& result = item->result;

            ModelN* m = item->model;
            QVector3D lt = m->localPosition();
            trimesh::quaternion rotation = m->nestRotation();
            QQuaternion q = QQuaternion(rotation.wp, rotation.xp, rotation.yp, rotation.zp);

            QVector3D t = QVector3D(result.x, result.y, lt.z());

            QQuaternion lq = m->localQuaternion();
            trimesh::fxform xf = trimesh::inv(trimesh::fromQuaterian(trimesh::quaternion(lq.scalar(), -lq.x(), -lq.y(), -lq.z())));
            trimesh::vec3 axis = trimesh::normalized(xf * trimesh::vec3(0.0f, 0.0f, 1.0f));
            QQuaternion dq = QQuaternion::fromAxisAndAngle(QVector3D(axis.x, axis.y, axis.z), -result.r);
            QQuaternion rq = dq * q;

            models.push_back(m);

            d_ts.push_back(t);
            d_qs.push_back(QQuaternion(rq.scalar(), -rq.x(), -rq.y(), -rq.z()));

            s_ts.push_back(lt);
            s_qs.push_back(q);

            m->dirtyNestData();
        };
        if (m_object)
        {
            f(m_object);
        }
        else {
            for (LayoutItem* item : m_objects)
            {
                f(item);
            }
        }

        mixTRModels(models, s_ts, d_ts, s_qs, d_qs, true);

        if (m_object)
        {
            bottomModel(m_object->model);
            addModel(m_object->model, true);
        }
    }

}