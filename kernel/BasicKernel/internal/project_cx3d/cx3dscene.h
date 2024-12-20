#ifndef _NULLSPACE_CX3DSCENE_1591149954610_H
#define _NULLSPACE_CX3DSCENE_1591149954610_H
#include <QtCore/QObject>
#include <QtGui/QMatrix4x4>
#include <QtCore/QMap>
#include "trimesh2/TriMesh.h"
#include "data/modeln.h"
#include "cx3drenderex.h"
#include "cx3dprojectmanager.h"

class Cx3dScene: public QObject
{
public:
	Cx3dScene(QObject* parent = nullptr);
	virtual ~Cx3dScene();
    void build(cx3d::Cx3dFileProject* reader, QThread* mainThread);
    void setScene();
    void setMachineProfile();
    void setMeshs();
    void setMachineSettings(QString& machineName);
protected:
    std::vector<creative_kernel::ModelN*> m_models;
	cx3d::contentXML* m_contentXML;
    std::vector<QDataStream*> m_qstreams;

	QByteArray m_paramSettings;

    QList<cxkernel::ModelNDataPtr> m_modelnDatas;
    QList<std::vector<double>> m_layerDatas;
    QList<us::USettings*> m_objectSettings;
};
#endif // _NULLSPACE_CX3DSCENE_1591149954610_H
