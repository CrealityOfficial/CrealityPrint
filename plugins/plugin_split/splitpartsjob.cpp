#include "splitpartsjob.h"
#include "data/modeln.h"

#include "trimesh2/TriMesh_algo.h"
#include "trimesh2/XForm.h"

#include "data/modeln.h"
#include "data/modelgroup.h"
#include "mmesh/trimesh/meshtopo.h"
#include "qcxutil/trimesh2/conv.h"

#include "mmesh/trimesh/trimeshutil.h"

#include "interface/visualsceneinterface.h"
#include "interface/modelinterface.h"

using namespace creative_kernel;
using namespace qtuser_core;
SplitPartsJob::SplitPartsJob(QObject* parent)
	:Job(parent)
	, m_model(nullptr)
{
}

SplitPartsJob::~SplitPartsJob()
{
}

QString SplitPartsJob::name()
{
	return "SplitPartsJob";
}

QString SplitPartsJob::description()
{
	return "";
}

void SplitPartsJob::setModel(ModelN* model)
{
	m_model = model;
}

void SplitPartsJob::work(Progressor* progressor)
{
	trimesh::TriMesh* mesh = m_model->mesh();

	if (!mesh || mesh->faces.size() <= 0)
		return;

	mmesh::MeshTopo topo;
	topo.build(mesh);

	int faceNum = (int)mesh->faces.size();
	std::vector<bool> visitFlags(faceNum, false);

	std::vector<int> visitStack;
	std::vector<int> nextStack;

	std::vector<std::vector<int>> parts;
	parts.reserve(100);
	for (int faceID = 0; faceID < faceNum; ++faceID)
	{
		if (visitFlags.at(faceID) == false)
		{
			visitFlags.at(faceID) = true;
			visitStack.push_back(faceID);

			std::vector<int> facesChunk;
			facesChunk.push_back(faceID);
			while (!visitStack.empty())
			{
				int seedSize = (int)visitStack.size();
				for (int seedIndex = 0; seedIndex < seedSize; ++seedIndex)
				{
					int cFaceID = visitStack.at(seedIndex);
					trimesh::ivec3& oppoHalfs = topo.m_oppositeHalfEdges.at(cFaceID);
					for (int halfID = 0; halfID < 3; ++halfID)
					{
						int oppoHalf = oppoHalfs.at(halfID);
						if (oppoHalf >= 0)
						{
							int oppoFaceID = topo.faceid(oppoHalf);
							if (visitFlags.at(oppoFaceID) == false)
							{
								nextStack.push_back(oppoFaceID);
								facesChunk.push_back(oppoFaceID);
								visitFlags.at(oppoFaceID) = true;
							}
						}
					}
				}

				visitStack.swap(nextStack);
				nextStack.clear();
			}

			parts.push_back(std::vector<int>());
			parts.back().swap(facesChunk);
		}
		else
		{
			visitFlags.at(faceID) = true;
		}
	}

	size_t partSize = parts.size();
	for (size_t i = 0; i < partSize; ++i)
	{
		if (parts.at(i).size() > 10)
		{
			trimesh::TriMesh* outMesh = mmesh::partMesh(parts.at(i), mesh);
			if (outMesh) m_meshes.push_back(outMesh);
		}
	}
}

void SplitPartsJob::failed()
{

}

void SplitPartsJob::successed(Progressor* progressor)
{
	creative_kernel::ModelGroup* group = qobject_cast<creative_kernel::ModelGroup*>(m_model->parent());
	if (m_meshes.size() > 0 && group)
	{
		QList<ModelN*> newModels;
		QString name = m_model->objectName();
		int id = 0;
		for (trimesh::TriMesh* mesh : m_meshes)
		{
			if (mesh->vertices.size() == 0 || mesh->faces.size() == 0)
			{
				delete mesh;
				continue;
			}
			++id;

			creative_kernel::ModelN* m = new creative_kernel::ModelN();
			QString subName = QString("%1-split-parts-%2").arg(name).arg(id);

			trimesh::fxform xf = qcxutil::qMatrix2Xform(m_model->globalMatrix());
			size_t vertexCount = mesh->vertices.size();
			for (size_t i = 0; i < vertexCount; ++i)
			{
				trimesh::vec3 v = mesh->vertices.at(i);
				mesh->vertices.at(i) = xf * v;
			}

			mesh->need_bbox();

			TriMeshPtr ptr(mesh);
			cxkernel::ModelCreateInput input;
			input.mesh = ptr;
			input.name = subName;
			cxkernel::ModelNDataPtr data(cxkernel::createModelNData(input));

			m->setData(data);

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