#include "colorloadjob.h"
#include "interface/visualsceneinterface.h"
#include "spread/meshspread.h"
#include <QCoreApplication>

ColorLoadJob::ColorLoadJob(QObject* parent) : qtuser_core::Job(parent)
{

}

void ColorLoadJob::setMeshSpreadWrapper(QList<spread::MeshSpreadWrapper*> wrappers)
{
	m_wrappers = wrappers;
}

void ColorLoadJob::setTriMesh(QList<TriMeshPtr> meshs)
{
	m_meshs = meshs;
}

void ColorLoadJob::setColorsList(const std::vector<trimesh::vec>& colorsList)
{
	m_colorsList = colorsList;
}

void ColorLoadJob::setData(const QList<std::vector<std::string>>& datas)
{
	m_datas = datas;
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
	int count = m_wrappers.count();
	for (int i = 0, count = m_wrappers.count(); i < count; ++i)
	{
		float percent = (float)(i + 1) / count;
		auto& wrapper = m_wrappers[i];
		wrapper->setInputs(m_meshs[i].get());
		progressor->progress(percent * 0.6);
		wrapper->set_triangle_from_data(m_datas[i]);
		progressor->progress(percent * 0.9);
		wrapper->updateTriangle();
	}
	// progressor->progress(1.0);
}

void ColorLoadJob::failed() 
{

}

void ColorLoadJob::successed(qtuser_core::Progressor* progressor) 
{
	// progressor->progress(0.8);
	emit sig_finished();
	progressor->progress(1.0);

	m_finished = true;
}