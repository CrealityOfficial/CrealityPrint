#include "letterjob.h"
#include "letterop.h"

#include "data/modeln.h"
#include "data/modelgroup.h"
#include "interface/visualsceneinterface.h"
#include "interface/modelinterface.h"

#include "utils/convexhullcalculator.h"

#include "qtusercore/module/progressortracer.h"

#include "mmesh/trimesh/trimeshutil.h"

#ifdef NOUSE_IGL
#include "cmesh/mesh/boolean.h"
#else
#include "cmesh/igl/booleanigl.h"
#endif

LetterJob::LetterJob(LetterOp* theOp, QObject* parent)
{
	m_pModel = nullptr;
	m_pResultMesh = nullptr;
	m_pOp = theOp;
	m_bIsTextOutside = true;
}

LetterJob::~LetterJob()
{

}

void LetterJob::SetModel(creative_kernel::ModelN* model)
{
	m_pModel = model;
}

void LetterJob::SetTextMeshs(std::vector<trimesh::TriMesh*>& theTextMeshs, std::vector<QMatrix4x4>& theTextMeshPoses)
{
	m_vTextMeshs = theTextMeshs;
	m_vTextMeshPoses = theTextMeshPoses;
}

void LetterJob::SetIsTextOutside(bool value)
{
	m_bIsTextOutside = value;
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
	if (m_pModel)
	{
        if (progressor)
        {
            progressor->progress(0.2);
        }
		if (m_bIsTextOutside)
		{
			m_pResultMesh = new trimesh::TriMesh();
			TransformMesh(m_pModel->mesh(), m_pModel->globalMatrix(), m_pResultMesh);

			trimesh::TriMesh theLetterMesh_global;

			for (size_t i = 0; i < m_vTextMeshs.size(); i++)
			{
				TransformMesh(m_vTextMeshs[i], m_vTextMeshPoses[i], &theLetterMesh_global);

				int vertex_id_start = m_pResultMesh->vertices.size();

				for (size_t j = 0; j < theLetterMesh_global.vertices.size(); j++)
				{
					auto& v = theLetterMesh_global.vertices[j];
					m_pResultMesh->vertices.emplace_back(v[0], v[1], v[2]);
				}

				for (size_t j = 0; j < theLetterMesh_global.faces.size(); j++)
				{
					auto& f = theLetterMesh_global.faces[j];
					m_pResultMesh->faces.emplace_back(vertex_id_start + f[0], vertex_id_start + f[1], vertex_id_start + f[2]);
				}

				progressor->progress(float(i) / m_vTextMeshs.size());
			}
		}
		else
		{
			trimesh::TriMesh theModelMesh_global;
			trimesh::TriMesh theLetterMesh_global;
			m_pResultMesh = &theModelMesh_global;

			TransformMesh(m_pModel->mesh(), m_pModel->globalMatrix(), &theModelMesh_global);

            int meshnums = m_vTextMeshs.size();
            float byte = 0.7 / meshnums;

            std::vector<trimesh::TriMesh*> meshs(meshnums,nullptr);
            for (size_t i = 0; i < meshnums; i++)
            {
                meshs[i] = new trimesh::TriMesh();
                TransformMesh(m_vTextMeshs[i], m_vTextMeshPoses[i], meshs[i]);
             }
            mmesh::mergeTriMesh( &theLetterMesh_global, meshs);

            for (size_t i = 0; i < meshnums; i++)
            {
                if (meshs[i])
                {
                    delete meshs[i];
                    meshs[i] = nullptr;
                }
            }
            meshs.clear();

            try
            {
#ifdef NOUSE_IGL
                m_pResultMesh = cmesh::cxBooleanOperateMeshObj(m_pResultMesh, &theLetterMesh_global,
                    m_bIsTextOutside ? cmesh::CMeshBooleanType::CBT_UNION
                    : cmesh::CMeshBooleanType::CBT_TM1_MINUS_TM2);
#else
                qtuser_core::ProgressorTracer tracer(progressor);
                m_pResultMesh = cmesh::cxBooleanOperateMeshIGLObj(m_pResultMesh, &theLetterMesh_global, cmesh::MESH_BOOLEAN_TYPE_MINUS, &tracer);
#endif        
                //m_pResultMesh = cmesh::cxBooleanOperateMeshObj(m_pResultMesh, &theLetterMesh_global,
                //	m_bIsTextOutside ? cmesh::CMeshBooleanType::CBT_UNION
                //			: cmesh::CMeshBooleanType::CBT_TM1_MINUS_TM2);

                //qtuser_core::ProgressorTracer tracer(progressor);
                //m_pResultMesh = cmesh::cxBooleanOperateMeshIGLObj(m_pResultMesh, &theLetterMesh_global, cmesh::MESH_BOOLEAN_TYPE_MINUS, &tracer);
            }
            catch (...)
            {
                m_pResultMesh = nullptr;
            }

            if (!m_pResultMesh)
            {
                printf("failed to make text on model: 'mmesh::cxBooleanOperateMeshObj' returns null or has exception!\n");
                return;
            }

            if (progressor)
            {
                progressor->progress(1.0);
            }
		}
		
	}

	progressor->progress(1.0f);
}

void LetterJob::failed()
{
	if (m_pOp)
	{
		m_pOp->SetMode(0);
	}
}

void LetterJob::successed(qtuser_core::Progressor* progressor)
{
	creative_kernel::ModelGroup* group = qobject_cast<creative_kernel::ModelGroup*>(m_pModel->parent());
	if (!group || ! m_pResultMesh || m_pResultMesh->vertices.size() == 0 || m_pResultMesh->faces.size() == 0)
	{
		if (m_pOp)
		{
			m_pOp->SetMode(0);
		}
		return;
	}

	QString name = m_pModel->objectName().left(m_pModel->objectName().lastIndexOf("."));
	QString suffix = m_pModel->objectName().right(m_pModel->objectName().length() - m_pModel->objectName().lastIndexOf("."));
	if (!name.endsWith("-lettered"))
	{
		name += "-lettered";
	}

	name += suffix;

	m_pResultMesh->need_bbox();

	creative_kernel::TriMeshPtr meshptr(m_pResultMesh);
	mmesh::dumplicateMesh(m_pResultMesh);
	ConvexHullCalculator::calculate(m_pResultMesh, nullptr);
	trimesh::vec3 offset = mmesh::moveTrimesh2Center(m_pResultMesh);
	creative_kernel::ModelN* newModel = new creative_kernel::ModelN();

	cxkernel::ModelCreateInput input;
	cxkernel::ModelNDataCreateParam param;
	param.toCenter = false;
	input.mesh = meshptr;
	input.name = name;
	cxkernel::ModelNDataPtr data(cxkernel::createModelNData(input, nullptr, param));

	newModel->setData(data);
	newModel->setLocalPosition(-QVector3D(offset.x, offset.y, offset.z));
	newModel->SetInitPosition(m_pModel->localPosition());

	QList<creative_kernel::ModelN*> removes;
	QList<creative_kernel::ModelN*> adds;

	removes.append(m_pModel);
	adds.append(newModel);
	creative_kernel::modifySpace(removes, adds, true);

	m_pModel = nullptr;
	m_pResultMesh = nullptr;
	creative_kernel::requestVisUpdate(true);

	if (m_pOp)
	{
		m_pOp->SetMode(0);
	}
}

void LetterJob::TransformMesh(trimesh::TriMesh* original_mesh, QMatrix4x4 pose, trimesh::TriMesh* transformed_mesh)
{
	transformed_mesh->clear();
	transformed_mesh->vertices = original_mesh->vertices;
	transformed_mesh->faces = original_mesh->faces;

	trimesh::xform xf_pose;

	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			xf_pose(i, j) = pose(i, j);
		}
	}

	trimesh::apply_xform(transformed_mesh, xf_pose);
}
