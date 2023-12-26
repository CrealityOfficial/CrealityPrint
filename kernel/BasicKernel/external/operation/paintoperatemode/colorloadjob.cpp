#include "colorloadjob.h"
#include "interface/visualsceneinterface.h"
#include "spread/meshspread.h"
#include <QCoreApplication>

ColorLoadJob::ColorLoadJob(QObject* parent) : qtuser_core::Job(parent)
{

}

void ColorLoadJob::setMeshSpreadWrapper(spread::MeshSpreadWrapper* wrapper)
{
	m_wrapper = wrapper;
}

void ColorLoadJob::setTriMesh(TriMeshPtr mesh)
{
	m_mesh = mesh;
}

void ColorLoadJob::setColorsList(const std::vector<trimesh::vec>& colorsList)
{
	m_colorsList = colorsList;
}

void ColorLoadJob::setData(const std::vector<std::string>& data)
{
	m_data = data;
}

QString ColorLoadJob::name() 
{
	return "ColorLoadJob";
}

QString ColorLoadJob::description() 
{
	return "ColorLoadJob";
}

void ColorLoadJob::wait()
{
	while (!m_finished) {
		QThread::msleep(25);
		QCoreApplication::processEvents();
	}
}

void ColorLoadJob::work(qtuser_core::Progressor* progressor) 
{
	m_wrapper->setInputs(m_mesh.get());
	progressor->progress(0.4);
	m_wrapper->set_triangle_from_data(m_data);
	m_wrapper->updateTriangle();
	progressor->progress(0.5);
}

void ColorLoadJob::failed() 
{

}

void ColorLoadJob::successed(qtuser_core::Progressor* progressor) 
{
	progressor->progress(0.8);
	emit sig_finished();
	progressor->progress(1.0);

	m_finished = true;
}