#include "photo2meshjob.h"

#include "interface/modelinterface.h"
#include "qtusercore/module/progressortracer.h"

Photo2MeshJob::Photo2MeshJob(QObject* parent)
	: qtuser_core::Job(parent)
	, m_model(nullptr)
{
}

Photo2MeshJob::~Photo2MeshJob()
{
}

void Photo2MeshJob::setFileName(const QString& fileName)
{
	m_name = fileName;
}

void Photo2MeshJob::setModel(cxkernel::PhotoMeshModel* model)
{
	m_model = model;
}

QString Photo2MeshJob::name()
{
	return "Photo2MeshJob";
}

QString Photo2MeshJob::description()
{
	return "Photo2MeshJob";
}

void Photo2MeshJob::work(qtuser_core::Progressor* progressor)
{
	qtuser_core::ProgressorTracer tracer(progressor);
	TriMeshPtr mesh = m_model ? m_model->build(m_name, &tracer) : nullptr;

	QString shortName = m_name;
	QStringList stringList = shortName.split("/");
	if (stringList.size() > 0)
		shortName = stringList.back();

	if (!mesh)
	{
		tracer.failed("PhotoMeshModel error.");
		return;
	}

	cxkernel::ModelCreateInput input;
	input.fileName = m_name;
	input.mesh = mesh;
	input.name = shortName;
	input.type = cxkernel::ModelNDataType::mdt_algrithm;

	m_data = cxkernel::createModelNData(input);
}

void Photo2MeshJob::failed()
{
}

void Photo2MeshJob::successed(qtuser_core::Progressor* progressor)
{
	creative_kernel::ModelN* model = creative_kernel::createModelFromData(m_data);
	QString fileName = m_data->input.fileName;
	creative_kernel::setMostRecentFile(fileName);
	creative_kernel::addModelLayout(model);
}