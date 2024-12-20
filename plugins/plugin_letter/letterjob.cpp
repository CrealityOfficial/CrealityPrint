#include "letterjob.h"

#include "data/modeln.h"
#include "interface/visualsceneinterface.h"
#include "interface/spaceinterface.h"
#include "interface/selectorinterface.h"
#include "interface/shareddatainterface.h"

#include "data/shareddatamanager/shareddatamanager.h"

#include "qtusercore/module/progressortracer.h"

#include <Qt3DRender/QCamera>
#include "interface/camerainterface.h"
#include "qtuser3d/camera/screencamera.h"
#include "qtuser3d/trimesh2/conv.h"
#include "us/usettings.h"
#include "msbase/mesh/tinymodify.h"
#include "data/spaceutils.h"

LetterJob::LetterJob(QObject* parent)
	:m_letterModel(nullptr)
{
}

LetterJob::~LetterJob()
{

}

void LetterJob::SetModelGroup(creative_kernel::ModelGroup* modelgroup)
{
	m_pModelGroup = modelgroup;
}


void LetterJob::SetObjects(const QList<QObject*>& objectList)
{
	m_objectLists = objectList;
}

void LetterJob::SetLetterModel(cxkernel::LetterModel* letterModel)
{
	m_letterModel = letterModel;
}

QString LetterJob::name()
{
	return "LetterJob";
}

QString LetterJob::description()
{
	return "Make text on the model";
}

void LetterJob::work(qtuser_core::Progressor* progressor)
{
	qtuser_core::ProgressorTracer tracer(progressor);

	std::vector<TriMeshPtr> trimesh_container;
	std::vector<trimesh::xform> ixf;
	for (auto m : m_pModelGroup->models())
	{
		if (!m)
		{
			return;
		}

		if (!m->modelNData())
		{
			tracer.failed("null data.");
			return;
		}

		if (m->modelNType() != creative_kernel::ModelNType::normal_part)
		{
			continue;
		}

		TriMeshPtr inputMesh(m->createGlobalMeshWithNormal());

		trimesh_container.push_back(inputMesh);

		trimesh::fxform xf = qtuser_3d::qMatrix2Xform(m->globalMatrix());
		ixf.push_back(trimesh::xform(trimesh::inv(xf)));
	}

	cxkernel::CameraModel camera;
	Qt3DRender::QCamera* mainCamera = creative_kernel::visCamera()->camera();
	camera.pos = qtuser_3d::qVector3D2Vec3(mainCamera->position());
	camera.up = qtuser_3d::qVector3D2Vec3(mainCamera->upVector());
	camera.center = qtuser_3d::qVector3D2Vec3(mainCamera->viewCenter());

	camera.fov = mainCamera->fieldOfView();
	camera.aspect = mainCamera->aspectRatio();
	camera.n = mainCamera->nearPlane();
	camera.f = mainCamera->farPlane();

	QSize surfSize = creative_kernel::surfaceSize();
	std::vector<TriMeshPtr> output_meshs;

	m_letterModel->_generatrGroup(trimesh_container,surfSize,camera,output_meshs,&tracer);

	for (int i = 0; i < output_meshs.size(); i++)
	{
		creative_kernel::ModelN* aModel = m_pModelGroup->models().at(i);
		if (!aModel)
			continue;

		if (output_meshs[i])
		{
			TriMeshPtr invertedMesh(new trimesh::TriMesh());
			*invertedMesh = *(output_meshs[i].get());

			trimesh::apply_xform(invertedMesh.get(), ixf[i]);

			trimesh::fxform normalXf = qtuser_3d::qMatrix2Xform(aModel->modelNormalMatrix());

			creative_kernel::changeMeshFaceNormal(invertedMesh.get(), output_meshs[i].get(), normalXf);

			trimesh::apply_xform(output_meshs[i].get(), ixf[i]);
			output_meshs[i]->clear_bbox();
			output_meshs[i]->need_bbox();

			cxkernel::MeshData* meshData = new cxkernel::MeshData(output_meshs[i], false);
			cxkernel::MeshDataPtr meshDataPtr(meshData);
			m_meshDatas.push_back(meshDataPtr);
		}
	}

}

void LetterJob::failed()
{
	emit finished();
}

void LetterJob::successed(qtuser_core::Progressor* progressor)
{
	int i = 0;
	for (auto m : m_pModelGroup->models())
	{
		if (m)
		{
			if (m->modelNType() != creative_kernel::ModelNType::normal_part)
			{
				continue;
			}

			QList<creative_kernel::ModelNPropertyMeshChange> changes;
			creative_kernel::ModelNPropertyMeshChange change;
			change.model = m;
			change.name = QStringLiteral("%1").arg(m->name());
			change.mesh = m_meshDatas[i];
			changes.append(change);
			creative_kernel::replaceModelNMeshes(changes, true);
			i++;
		}
	}

	
	emit finished();
}
