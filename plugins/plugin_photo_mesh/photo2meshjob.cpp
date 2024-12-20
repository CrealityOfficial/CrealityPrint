#include "photo2meshjob.h"

#include "interface/spaceinterface.h"
#include "qtusercore/module/progressortracer.h"
#include "ccglobal/log.h"

Photo2MeshJob::Photo2MeshJob(QObject* parent)
	: qtuser_core::Job(parent)
	, m_model(nullptr)
{
}

Photo2MeshJob::~Photo2MeshJob()
{
}

void Photo2MeshJob::setFileNames(const QList<QString>& file_names)
{
	m_file_names = file_names;
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
	for (const QString& file_name : m_file_names)
	{
		TriMeshPtr mesh = m_model ? m_model->build(file_name, &tracer) : nullptr;

		QString shortName = file_name;
		QStringList stringList = shortName.split("/");
		if (stringList.size() > 0)
			shortName = stringList.back();

		if (!mesh)
		{
			LOGW("PhotoMeshModel error. [%s]", file_name.toLocal8Bit().constData());
			continue;
		}

		creative_kernel::MeshCreateInput input;
		input.fileName = file_name;
		input.mesh = mesh;
		input.name = shortName;

		m_create_datas.append(input);
	}

	if (m_create_datas.isEmpty())
		tracer.failed("Photo2MeshJob error.");
}

void Photo2MeshJob::failed()
{
}

void Photo2MeshJob::successed(qtuser_core::Progressor* progressor)
{
	creative_kernel::createFromInputs(m_create_datas);
}