#include "letterjob.h"

#include "data/modeln.h"
#include "interface/visualsceneinterface.h"
#include "interface/modelinterface.h"
#include "interface/spaceinterface.h"
#include "interface/selectorinterface.h"

#include "qtusercore/module/progressortracer.h"

#include <Qt3DRender/QCamera>
#include "interface/camerainterface.h"
#include "qtuser3d/camera/screencamera.h"
#include "qtuser3d/trimesh2/conv.h"
#include "us/usettings.h"

LetterJob::LetterJob(QObject* parent)
	:m_letterModel(nullptr)
{
	m_pModel = nullptr;
	m_pResultMesh = nullptr;
}

LetterJob::~LetterJob()
{

}

void LetterJob::SetModel(creative_kernel::ModelN* model)
{
	m_pModel = model;
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

	if (!m_pModel)
	{
		return;
	}

	if (!m_pModel->modelNData())
	{
		tracer.failed("null data.");
		return;
	}

	trimesh::fxform xf = qtuser_3d::qMatrix2Xform(m_pModel->globalMatrix());
	trimesh::xform ixf = trimesh::xform(trimesh::inv(xf));
	TriMeshPtr inputMesh(m_pModel->createGlobalMesh());

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

	TriMeshPtr mesh = m_letterModel->_generate(inputMesh, surfSize, camera, &tracer);

	if(mesh)
	{
		trimesh::apply_xform(mesh.get(), ixf);
		mesh->need_bbox();

		cxkernel::ModelCreateInput input;
		input.mesh = mesh;
		input.defaultColor = m_pModel->defaultColorIndex();
		input.name = m_pModel->objectName();

		cxkernel::ModelNDataCreateParam param;
		param.toCenter = false;
		m_newMeshdata = cxkernel::createModelNData(input, nullptr, param);
	}
}

void LetterJob::failed()
{
	emit finished();
}

void LetterJob::successed(qtuser_core::Progressor* progressor)
{
	if (m_newMeshdata && m_pModel)
	{
		QList<creative_kernel::ModelN*> models;
		models.push_back(m_pModel);

		creative_kernel::ModelN* newModel = creative_kernel::createModelFromData(m_newMeshdata);
		creative_kernel::setModelPosition(newModel, m_pModel->localPosition(), false);
		creative_kernel::setModelRotation(newModel, m_pModel->localQuaternion(), false);
		creative_kernel::setModelScale(newModel, m_pModel->localScale(), true);

		QList<creative_kernel::ModelN*> newModels;
		newModels.push_back(newModel);
		newModel->setting()->merge(m_pModel->setting());

		creative_kernel::modifySpace(models, newModels, true);
		creative_kernel::selectOne(newModel);
	}
	emit finished();
}
