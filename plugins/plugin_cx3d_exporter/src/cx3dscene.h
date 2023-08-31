#ifndef _NULLSPACE_CX3DSCENE_1591149954610_H
#define _NULLSPACE_CX3DSCENE_1591149954610_H
#include <QtCore/QObject>
#include <QtGui/QMatrix4x4>
#include <QtCore/QMap>
#include "trimesh2/TriMesh.h"
#include "data/modeln.h"
#include "interface/modelinterface.h"
#include "cx3drenderex.h"

class Cx3dScene: public QObject
{
public:
	Cx3dScene(QObject* parent = nullptr);
	virtual ~Cx3dScene();
    void build(Cx3dRenderEx* reader, QThread* mainThread);
    void setScene();
    void setMachineProfile();
    void setMeshs();
    void setMachineSettings(QString& machineName);
    void setSupport(QString filePathName,creative_kernel::ModelN* model);


    void OldsetMachineProfile();
protected:
    std::vector<trimesh::TriMesh*> m_meshes;
	contentXML* m_contentXML;
    std::vector<QDataStream*> m_qstreams;

    std::vector<creative_kernel::FDMSupportGroup*> m_fdmSup;

	QByteArray m_machineSettings;
	QByteArray m_profileSettings;
};
#endif // _NULLSPACE_CX3DSCENE_1591149954610_H
