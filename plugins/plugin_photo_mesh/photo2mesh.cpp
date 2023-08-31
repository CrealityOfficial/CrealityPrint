#include "photo2mesh.h"
#include "photo2meshjob.h"

#include "interface/uiinterface.h"
#include "cxkernel/interface/jobsinterface.h"

Photo2Mesh::Photo2Mesh(QObject* parent)
	: QObject(parent)
{
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
		Photo2MeshInput input = m_input;
		input.fileName = fileName;
		Photo2MeshJob* job = new Photo2MeshJob();
		job->setInput(input);
		jobs.push_back(qtuser_core::JobPtr(job));
	}
	
	cxkernel::executeJobs(jobs);
}

void Photo2Mesh::cancel()
{

}

void Photo2Mesh::setBlur(int blur)
{
	if (blur < 0)
	{
		blur = 0;
	}
	if (blur > 18)
	{
		blur = 18;
	}
	m_input.blur = blur;
}

void Photo2Mesh::setLighterOrDarker(int index)
{
	m_input.invert = index == 0 ? true : false;
}

void Photo2Mesh::setBaseHeight(float baseHeight)
{
	if (baseHeight > 0.0f)
		m_input.baseHeight = baseHeight;
}

void Photo2Mesh::setMaxHeight(float maxHeight)
{
	if (maxHeight > 0)
		m_input.maxHeight = maxHeight;
}

void Photo2Mesh::setMeshWidth(float meshX)
{
	m_input.meshXWidth = meshX;
}

