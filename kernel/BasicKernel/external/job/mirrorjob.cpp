#include "mirrorjob.h"
#include "interface/selectorinterface.h"
#include "trimesh2/TriMesh.h"
#include "kernel/kernelui.h"
#include "qtusercore/module/progressortracer.h"
#include "qtuser3d/trimesh2/conv.h"
#include "interface/selectorinterface.h"
#include "interface/renderinterface.h"
#include "interface/visualsceneinterface.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include "interface/shareddatainterface.h"

using namespace creative_kernel;

MirrorJob::MirrorJob(const QList<creative_kernel::ModelN*>& models, int mode) 
    : qtuser_core::Job()
{     // 0: x, 1: y, 2: z
  m_models = models;
  m_mode = mode;
}

MirrorJob::~MirrorJob()
{

}

void MirrorJob::work(qtuser_core::Progressor* progressor)
{
    qtuser_core::ProgressorTracer tracer(progressor);

    QMatrix4x4 mirrorMatrix;
    if (m_mode == 0)  //x
    {
        mirrorMatrix(0, 0) = -1;
    }
    else if (m_mode == 1)//y
    {
        mirrorMatrix(1, 1) = -1;
    }
    else if (m_mode == 2)//z
    {
        mirrorMatrix(2, 2) = -1;
    }

    for (auto model : m_models)
    {
        trimesh::TriMesh* mesh = new trimesh::TriMesh();
        *mesh = *(model->data()->mesh);

        QMatrix4x4 matrix = model->globalMatrix();
        trimesh::fxform xf = qtuser_3d::qMatrix2Xform(matrix);
        int _size = mesh->vertices.size();
        for (int n = 0; n < _size; ++n)
        {
            trimesh::vec3 v = mesh->vertices.at(n);
            mesh->vertices.at(n) = xf * v;
        }

        _size = mesh->faces.size();
        for (int n = 0; n < _size; ++n)
        {
            auto face = mesh->faces.at(n);
            mesh->faces.at(n)[1] = face[2];
            mesh->faces.at(n)[2] = face[1];
        }
        trimesh::apply_xform(mesh, trimesh::xform(qtuser_3d::qMatrix2Xform(mirrorMatrix)));

        //cxkernel::ModelCreateInput input = model->data()->input;
        //input.mesh.reset(mesh);
        //input.defaultColor = model->defaultColorIndex();

       // cxkernel::ModelNDataCreateParam param;
       //// param.toCenter = false;
       // cxkernel::ModelNDataPtr data = cxkernel::createModelNData(input, &tracer, param);

        //cxkernel::MeshData* meshData = new cxkernel::MeshData(mesh);
        //cxkernel::MeshDataPtr meshDataPtr(meshData);
        //int meshID = registerMeshData(meshDataPtr);
        //
        //creative_kernel::SharedDataID id = model->sharedDataID();
        //id(MeshID) = meshID;
        //
        //m_modelIDs << id;
    }
}

void MirrorJob::failed()
{

}

void MirrorJob::successed(qtuser_core::Progressor* progressor)
{
    for (int i = 0, count = m_models.count(); i < count; ++i)
    {
        m_models[i]->setSharedData(m_modelIDs[i]);
        // bottomModel(m_models[i], false);
        m_models[i]->resetLocalScale(false);
        m_models[i]->resetLocalQuaternion(true);
        //m_models[i]->getModelEntity()->setEnabled(true);
        m_models[i]->dirty();
    }

    requestVisUpdate();
  
}

