#ifndef _NULLSPACE_CX3DRENDEREX_1591149954579_H
#define _NULLSPACE_CX3DRENDEREX_1591149954579_H
#include <QtCore/QObject>
//#include "cx3dscene.h"
#include "qtusercore/module/progressor.h"
#include <QtCore/QXmlStreamReader>
#include "quazip/quazip.h"
#include "quazip/quazipfile.h"
#include "cx3dwriter.h"


class Cx3dRenderEx: public QObject
{
public:
	Cx3dRenderEx(QString zipPathName, qtuser_core::Progressor* progressor,QObject* parent = nullptr);
	virtual ~Cx3dRenderEx();
	void build();
	contentXML* getContentXML();
	std::vector<trimesh::TriMesh*> getMeshs();
	std::vector<creative_kernel::FDMSupportGroup*> getFDMSup();
	QByteArray getMachineDefault();
	QByteArray getProfileDefault();
protected:
	void readContents();
	void readDefault();
	void loadZip();
	void meshs();
	void FDMSup();
	void Support();
protected:
	QuaZip* m_zip;
	QuaZipFile* m_pModelFile;
	QXmlStreamReader* m_pXmlReader;
	QString m_zipPathName;
	
	bool isFDM;
	contentXML* m_contentxml;
	qtuser_core::Progressor* m_progressor;
	std::vector<creative_kernel::FDMSupportGroup*> m_vctFDMSup;
	std::vector<trimesh::TriMesh*> m_meshs;
	QByteArray m_machineSettings;
	QByteArray m_profileSettings;

};
#endif // _NULLSPACE_CX3DRENDEREX_1591149954579_H
