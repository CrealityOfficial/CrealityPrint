#ifndef CX3DWRITER_H
#define CX3DWRITER_H

#include <QObject>
#include <QUrl>
#include "trimesh2/TriMesh.h"
#include "quazip/quazip.h"
#include "quazip/quazipfile.h"
#include "qtusercore/module/progressor.h"
#include <QXmlStreamWriter>
#include <QMap>
#include "data/modeln.h"

typedef struct modelMatrix
{
    QVector3D Position;
    QVector3D Scale;
    QQuaternion Rotate;
    QString supName;
};

typedef struct contentXML
{
    QString  sliceType;
    QString  profile;
    QString  machine;
    QString  materials;
    //QMap<QString, modelMatrix> STLPathName_mMatrix;
    QVector<QString> meshPathName;
    QVector<modelMatrix> meshMatrix;
    QVector<QString> meshName;
};


class Cx3dWriter : public QObject
{
    Q_OBJECT
public:
    enum EXPORTERROR { ZIP_ERROR,FILEIO_ERROR} ;
    explicit Cx3dWriter(const QList<creative_kernel::ModelN*>& allModels, QString path, qtuser_core::Progressor* progressor,QObject *parent = nullptr, bool isBackup = false);
    ~Cx3dWriter();
    void saveProfile(QString strPath);
    void saveSTL(QString strName, QString strPathName, creative_kernel::ModelN* modeln);
    void saveQXML(QString strName, QString strPathName);

    void saveSettings(QString strName, QString strPathName);
	int addMesh();
signals:
    void onError(EXPORTERROR error);
public slots:
private:
    void mesh2DocObject(trimesh::TriMesh *mesh);
    QString convertMatrixToString(QMatrix4x4 matrix);
private:
    QuaZip* m_quaZip;
    QuaZipFile* m_pModelFile;
    QXmlStreamWriter *m_xmlWriter;
    qtuser_core::Progressor* m_progress;
    contentXML m_contentxml;
};

#endif // CX3DWRITER_H
