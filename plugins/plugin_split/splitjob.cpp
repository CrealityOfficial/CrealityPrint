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

//#include "msbase/utils/cut.h"
#include "CUT/cut.h"

#include "msbase/mesh/tinymodify.h"
#include "interface/selectorinterface.h"
using namespace creative_kernel;
SplitJob::SplitJob(QObject* parent)
	: Job(parent)
	, m_model(nullptr)
	, m_gap(0)
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

void SplitJob::setPlane(const trimesh::vec3& position, const trimesh::vec3& normal)
{
	m_position = position;
	m_normal = normal;
}
void SplitJob::setGap(float gap)
{
	m_gap = gap;
}
void SplitJob::work(qtuser_core::Progressor* progressor)
{
	if (!m_model)
		return;

	//QMatrix4x4 m = m_model->globalMatrix();

	//trimesh::fxform xf = qtuser_3d::qMatrix2Xform(m);
	//QMatrix4x4 invm = m.inverted();
	//msbase::CutPlane aplane;
	//aplane.normal = m_normal;
	//aplane.position = m_position;

	//progressor->progress(0.1f);
	//m_model->mesh()->bbox;

	//trimesh::TriMesh amesh = *m_model->mesh();
	//for (trimesh::point& apoint: amesh.vertices)
	//{
		//apoint = xf * apoint;
	//}

	//msbase::planeCut(&amesh, aplane, m_meshes);
	//progressor->progress(1.0f);

	CUT::CutPlane aplane;
	aplane.normal = m_normal;
	aplane.position = m_position;

	//std::vector<trimesh::TriMesh*> meshes;

	TriMeshPtr inputMesh(m_model->createGlobalMesh());
	progressor->progress(0.1f);
	//m_model->mesh()->bbox;
	CUT::planeCut(&*inputMesh, aplane, m_meshes);
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
		if (!m_model->objectName().contains("."))
		{
			suffix = QString();
		}
		int id = 0;
		int size = m_meshes.size();
		float centerOfIndex = float(size - 1) / 2.0;
		float gap = m_gap;

		QVector3D dirOnFloor = qtuser_3d::vec2qvector( m_normal );
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
			QString subName = QString("%1-%2").arg(name).arg(id) + suffix;

			size_t vertexCount = mesh->vertices.size();
			//for (size_t i = 0; i < vertexCount; ++i)
			//{
			//	trimesh::vec3 v = mesh->vertices.at(i);
			//	mesh->vertices.at(i) = /*xf **/ v;
			//}
			
			mesh->clear_bbox();
			mesh->need_bbox();
			//trimesh::vec3 offset = msbase::moveTrimesh2Center(mesh);
			auto box = mesh->bbox;
			trimesh::vec3 center = box.center();

			trimesh::vec3 min = box.min;
			trimesh::vec3 planeDir = m_normal;
			bool atPlaneForward = true;
			for (trimesh::vec3 v : mesh->vertices)
			{
				float d = (v - m_position).dot(planeDir);
				if (d > 0.01)
				{
					atPlaneForward = true;
					break;
				}
				else if (d < -0.01)
				{
					atPlaneForward = false;
					break;
				}
			}
			trimesh::vec3 offsetDir = atPlaneForward ? planeDir : -planeDir;

			if (offsetDir == trimesh::vec3(0.0f, 0.0f, 1.0f)) 
			{
				center.z += gap;
			}
			else if (offsetDir == trimesh::vec3(0.0f, 0.0f, -1.0f)) 
			{	// 贴底一侧不需要平移
				//center.z;
			}
			else 
			{	// 其它方向水平平移
				offsetDir[2] = 0;
				trimesh::normalize(offsetDir);
				double offsetLen = gap / 2.0;
				center += offsetDir * offsetLen;
			}

			TriMeshPtr ptr(mesh);
			cxkernel::ModelCreateInput input;
			cxkernel::ModelNDataCreateParam param;
			param.toCenter = true;
			input.mesh = ptr;
			input.name = subName;
			input.defaultColor = m_model->defaultColorIndex();
			cxkernel::ModelNDataPtr data(cxkernel::createModelNData(input, nullptr, param));

			m->setData(data);
			m->setLocalPosition(QVector3D(center.x, center.y, center.z));
			m->SetInitPosition(m->localPosition());
			newModels.push_back(m);
		}
		m_meshes.clear();

		if (newModels.size() > 0)
		{
			QList<ModelN*> removes;
			removes << m_model;

			modifySpace(removes, newModels, true);

			creative_kernel::selectOne(newModels.last());
			
			
		}

		requestVisUpdate(true);
	}
}
