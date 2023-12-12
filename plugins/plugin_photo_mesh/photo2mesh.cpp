#include "photo2mesh.h"
#include "photo2meshjob.h"

#include "interface/uiinterface.h"
#include "cxkernel/interface/jobsinterface.h"

Photo2Mesh::Photo2Mesh(QObject* parent)
	: QObject(parent)
{
	m_model = new cxkernel::PhotoMeshModel(this);
}

Photo2Mesh::~Photo2Mesh()
{
}

void Photo2Mesh::processFiles(const QStringList& fileNames)
{
	m_fileNames = fileNames;
	creative_kernel::requestQmlDialog(this, "importimageDlg");
}

void Photo2Mesh::accept()
{
	QList<qtuser_core::JobPtr> jobs;
	for (const QString& fileName : m_fileNames)
	{
		Photo2MeshJob* job = new Photo2MeshJob();
		job->setFileName(fileName);
		job->setModel(m_model);
		jobs.push_back(qtuser_core::JobPtr(job));
	}
	
	cxkernel::executeJobs(jobs);
}

void Photo2Mesh::cancel()
{

}

void Photo2Mesh::setBlur(int blur)
{
	m_model->setBlur(blur);
}

void Photo2Mesh::setLighterOrDarker(int index)
{
	m_model->setLighterOrDarker(index);
}

void Photo2Mesh::setBaseHeight(float baseHeight)
{
	m_model->setBaseHeight(baseHeight);
}

void Photo2Mesh::setMaxHeight(float maxHeight)
{
	m_model->setMaxHeight(maxHeight);
}

void Photo2Mesh::setMeshWidth(float meshX)
{
	m_model->setMeshWidth(meshX);
}

