#include "cx3dreader.h"
#include "quazip/quazipfile.h"
#include <QtDebug>
#include <QMatrix4x4>
#include <Qt3DCore/QTransform>
#include "cx3dexportjob.h"
#include "trimesh2/TriMesh.h"

using namespace creative_kernel;
Cx3dReader::Cx3dReader(QString path,QObject *parent) : QObject(parent)
{
    m_zip.setZipName(path);
    m_zip.open(QuaZip::mdUnzip);
    m_pModelFile = new QuaZipFile(m_zip.getZipName(), "3D/3dmodel.model");
    m_pModelFile->open(QIODevice::ReadOnly );
    m_pXmlReader = new QXmlStreamReader(m_pModelFile);
}

//{
//    trimesh::TriMesh *mesh=nullptr;
//    bool startbuild = false;
//    while (!m_pXmlReader->atEnd()) {
//        if(m_pXmlReader->isStartElement())
//        {
//
//            QStringRef tagname = m_pXmlReader->name();
//            if(tagname == "object")
//            {
//                QXmlStreamAttributes attributes = m_pXmlReader->attributes();
//                int id = attributes.value("id").toInt();
//            }
//            if(tagname == "mesh")
//            {
//                mesh  = new trimesh::TriMesh();
//
//            }
//            if(tagname == "vertex")
//            {
//                QXmlStreamAttributes attributes = m_pXmlReader->attributes();
//                trimesh::point point;
//                point[0] = attributes.value("x").toFloat();
//                point[1] = attributes.value("y").toFloat();
//                point[2] = attributes.value("z").toFloat();
//                mesh->vertices.push_back(point);
//            }
//            if(tagname == "triangle")
//            {
//                QXmlStreamAttributes attributes = m_pXmlReader->attributes();
//                trimesh::TriMesh::Face face;
//                face[0]=attributes.value("v1").toInt();
//                face[1]=attributes.value("v2").toInt();
//                face[2]=attributes.value("v3").toInt();
//                mesh->faces.push_back(face);
//            }
//            if(tagname == "component")
//            {
//                QXmlStreamAttributes attributes = m_pXmlReader->attributes();
//                int id = attributes.value("objectid").toInt();
//                QMatrix4x4 matrix = convertStringToMatrix(attributes.value("transform").toString());
//                Qt3DCore::QTransform transform;
//                transform.setMatrix(matrix);
//            }
//            if(tagname == "build")
//            {
//                startbuild = true;
//            }
//            if(tagname == "item")
//            {
//
//                if(!startbuild)
//                {
//                    m_pXmlReader->readNext();
//                    continue;
//                }
//                QXmlStreamAttributes attributes = m_pXmlReader->attributes();
//                int id = attributes.value("objectid").toInt();
//                m_models[id]->setParent(nullptr);
//                topModels.push_back(m_models[id]);
//                Qt3DCore::QTransform transform;
//                QMatrix4x4 matrix = convertStringToMatrix(attributes.value("transform").toString());
//                transform.setMatrix(matrix);
//                qDebug()<<transform.scale3D();
//            }
//
//        }
//        if(m_pXmlReader->isEndElement())
//        {
//            QStringRef tagname = m_pXmlReader->name();
//            if(tagname == "mesh")
//            {
//                model->setMesh(mesh);
//            }
//            if(tagname == "object")
//            {
//                model = nullptr;
//            }
//            if(tagname == "build")
//            {
//                startbuild = false;
//            }
//        }
//
//         m_pXmlReader->readNext();
//
//
//        }
//        if (m_pXmlReader->hasError()) {
//              emit onError(ZIP_ERROR);
//        }
//}

Cx3dReader::~Cx3dReader()
{

    m_pModelFile->close();
    delete  m_pModelFile;
    m_zip.close();
}




QMatrix4x4 Cx3dReader::convertStringToMatrix(QString sMatrix)
{
    QStringList ls = sMatrix.split(" ");
    float fMat[16];
    memset(fMat,0,16*4);
    for(int i=0;i<16;i++)
    {
        fMat[i] = ls[i].toFloat();
    }
    //fMat[15]=1.0;
    QMatrix4x4 m(fMat);

    return m.transposed();
}

