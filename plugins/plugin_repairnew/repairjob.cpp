#include "repairjob.h"

#include "interface/visualsceneinterface.h"
#include "qtusercore/module/progressortracer.h"

#include "cmesh/mesh/repair.h"
#include "cmesh/cgal/subrepairmenu.h"

using namespace creative_kernel;
#ifdef Q_OS_WINDOWS
//using namespace trimesh;
RepairJob::RepairJob(QObject* parent)
	: qtuser_core::Job(parent)
	, m_model(nullptr)
	, m_receiver(nullptr)
{
}

RepairJob::~RepairJob()
{
}

void RepairJob::setModel(creative_kernel::ModelN* model)
{
	m_model = model;
}

QString RepairJob::name()
{
	return "";
}

QString RepairJob::description()
{
	return "";
}

void RepairJob::setObject(QObject* receiver)
{
	m_receiver = receiver;
}

void RepairJob::work(qtuser_core::Progressor* progressor)
{
	if (!m_model)
	{
		qDebug() << "Please choose one model";
		return;
	}

	qDebug() << "reapir work";
	trimesh::TriMesh* choosedMesh = m_model->mesh();
	TriMeshPtr mesh(repairMeshObj(choosedMesh, progressor));

	if (mesh)
	{
		QString name = m_model->objectName().left(m_model->objectName().lastIndexOf("."));
		QString suffix = m_model->objectName().right(m_model->objectName().length() - m_model->objectName().lastIndexOf("."));
		if (!name.endsWith("-repair"))
		{
			name += "-repair";
		}
		name += suffix;
		m_data = cxkernel::createModelNData(mesh, name, cxkernel::ModelNDataType::mdt_algrithm);
	}

	progressor->progress(1.0f);
}

void RepairJob::failed()
{
	qDebug() << "repair failed";
	m_model = nullptr;
}

void RepairJob::successed(qtuser_core::Progressor* progressor)
{
	if (m_data)
	{
		ModelN* m = nullptr;
		if (m)
		{
			cmesh::ErrorInfo info;
			cmesh::getErrorInfo(m->mesh(), info);

			//m->setErrorEdges(info.edgeNum);
			//m->setErrorNormals(info.normalNum);
			//m->setErrorHoles(info.holeNum);
			//m->setErrorIntersects(info.intersectNum);
		}
	}

	qDebug() << "repair success";
}

trimesh::TriMesh* RepairJob::repairMeshObj(trimesh::TriMesh* choosedMesh, qtuser_core::Progressor* progressor)
{
#ifdef USE_CGAL_REPAIR_MESH
	m_mesh = cgalRepairMeshObj(choosedMesh);
#else
	
	//m_mesh = tmeshRepairMeshObj(choosedMesh, progressor);
	trimesh::TriMesh* result = nullptr;

	try {
		qtuser_core::ProgressorTracer tracer(progressor);
		//repair::TriMeshRepair repair;
		//repair.initMesh(choosedMesh);
		//m_mesh = repair.repair(&tracer);

		//m_mesh = cmesh::repairMenu(choosedMesh, true, &tracer);

		result = cmesh::subRepairMenu(choosedMesh, &tracer);
	}
	catch (...)
	{
		qDebug() << "Repair::work, mmesh::repairMeshObj  catch(...)";
		progressor->failed("repair failed!");
		return nullptr;
	}


#endif
	return result;
}
#endif