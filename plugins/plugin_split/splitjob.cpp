#include "splitjob.h"

#include "data/modeln.h"

#include "trimesh2/TriMesh_algo.h"
#include "trimesh2/XForm.h"
#include <stack>

#include "qtuser3d/math/const.h"
#include "qtuser3d/trimesh2/conv.h"
#include "data/modeln.h"
#include "data/modelgroup.h"

#include "interface/visualsceneinterface.h"
#include "interface/modelinterface.h"

#include "msbase/utils/cut.h"
#include "msbase/mesh/tinymodify.h"

using namespace creative_kernel;
SplitJob::SplitJob(QObject* parent)
	: Job(parent)
	, m_model(nullptr)
{
}

SplitJob::~SplitJob()
{
	qDeleteAll(m_meshes);
	m_meshes.clear();
}

QString SplitJob::name()
{
	return "Split";
}

QString SplitJob::description()
{
	return "";
}

void SplitJob::setModel(creative_kernel::ModelN* model)
{
	m_model = model;
}

void SplitJob::setPlane(const qtuser_3d::Plane& plane)
{
	m_plane = plane;
}

void SplitJob::work(qtuser_core::Progressor* progressor)
{
	if (!m_model)
		return;

	QMatrix4x4 m = m_model->globalMatrix();

	trimesh::fxform xf = qtuser_3d::qMatrix2Xform(m);
	//QMatrix4x4 invm = m.inverted();
	msbase::CutPlane aplane;
	aplane.normal = m_plane.dir;
	aplane.position = /*invm * */m_plane.center;

	progressor->progress(0.1f);
	m_model->mesh()->bbox;

	trimesh::TriMesh amesh = *m_model->mesh();
	for (trimesh::point& apoint: amesh.vertices)
	{
		apoint = xf * apoint;
	}

	msbase::planeCut(&amesh, aplane, m_meshes);
	progressor->progress(1.0f);
}

void SplitJob::failed()
{

}

void SplitJob::successed(qtuser_core::Progressor* progressor)
{
	creative_kernel::ModelGroup* group = qobject_cast<creative_kernel::ModelGroup*>(m_model->parent());
	if (m_meshes.size() > 0 && group)
	{
		QList<ModelN*> newModels;
		QString name = m_model->objectName().left(m_model->objectName().lastIndexOf("."));
		QString suffix = m_model->objectName().right(m_model->objectName().length() - m_model->objectName().lastIndexOf("."));
		int id = 0;
		int size = m_meshes.size();
		float centerOfIndex = float(size - 1) / 2.0;
		float gap = 10.0;

		QVector3D dirOnFloor = m_plane.dir;
		dirOnFloor.setZ(0.0);
		dirOnFloor.normalize();

		for (int i = 0; i < size; i++ )
		{
			trimesh::TriMesh* mesh = m_meshes.at(i);
			if (mesh->vertices.size() == 0 || mesh->faces.size() == 0)
			{
				delete mesh;
				continue;
			}
			++id;

			creative_kernel::ModelN* m = new creative_kernel::ModelN();
			QString subName = QString("%1-split-%2").arg(name).arg(id) + suffix;

			size_t vertexCount = mesh->vertices.size();
			//for (size_t i = 0; i < vertexCount; ++i)
			//{
			//	trimesh::vec3 v = mesh->vertices.at(i);
			//	mesh->vertices.at(i) = /*xf **/ v;
			//}
			
			mesh->clear_bbox();
			mesh->need_bbox();
			trimesh::vec3 offset = msbase::moveTrimesh2Center(mesh);

			trimesh::vec3 layoutOffset = (trimesh::vec3(dirOnFloor.x(), dirOnFloor.y(), dirOnFloor.z()) *  trimesh::vec3(i-centerOfIndex, i-centerOfIndex, 0.0f)) * gap;
			offset += layoutOffset;

			TriMeshPtr ptr(mesh);
			cxkernel::ModelCreateInput input;
			cxkernel::ModelNDataCreateParam param;
			param.toCenter = false;
			input.mesh = ptr;
			input.name = subName;
			cxkernel::ModelNDataPtr data(cxkernel::createModelNData(input, nullptr, param));

			m->setData(data);
			m->setLocalPosition(-QVector3D(offset.x, offset.y, offset.z));
			m->SetInitPosition(m->localPosition());
			newModels.push_back(m);
		}
		m_meshes.clear();

		if (newModels.size() > 0)
		{
			QList<ModelN*> removes;
			removes << m_model;

			modifySpace(removes, newModels, true);
		}

		requestVisUpdate(true);
	}
}
