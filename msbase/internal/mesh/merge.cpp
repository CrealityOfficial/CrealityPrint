#include "msbase/mesh/merge.h"
#include "msbase/mesh/dumplicate.h"
#include "msbase/utils/meshtopo.h"
#include "msbase/mesh/get.h"
#include <assert.h>

#define MIN_AREA 10

namespace msbase
{
	trimesh::TriMesh* mergeMeshes(const std::vector<std::shared_ptr<trimesh::TriMesh>>& ins)
	{
		std::vector<trimesh::TriMesh*> meshes;
		for (std::shared_ptr<trimesh::TriMesh> mesh : ins)
			meshes.push_back(mesh.get());

		return mergeMeshes(meshes);
	}

	trimesh::TriMesh* mergeMeshes(const std::vector<trimesh::TriMesh*>& inMeshes)
	{
		trimesh::TriMesh* outMesh = new trimesh::TriMesh();
		size_t totalVertexSize = outMesh->vertices.size();
		size_t totalUVSize = outMesh->cornerareas.size();
		size_t totalTriangleSize = outMesh->faces.size();

		size_t addVertexSize = 0;
		size_t addTriangleSize = 0;
		size_t addUVSize = 0;

		size_t meshSize = inMeshes.size();
		for (size_t i = 0; i < meshSize; ++i)
		{
			if (inMeshes.at(i))
			{
				addVertexSize += inMeshes.at(i)->vertices.size();
				addTriangleSize += inMeshes.at(i)->faces.size();
				addUVSize += inMeshes.at(i)->cornerareas.size();
			}
		}
		totalVertexSize += addVertexSize;
		totalTriangleSize += addTriangleSize;
		totalUVSize += addUVSize;

		if (addVertexSize > 0 && addTriangleSize > 0)
		{
			outMesh->vertices.reserve(totalVertexSize);
			outMesh->cornerareas.reserve(totalUVSize);
			outMesh->faces.reserve(totalTriangleSize);

			size_t startFaceIndex = outMesh->faces.size();
			size_t startVertexIndex = outMesh->vertices.size();;
			size_t startUVIndex = outMesh->cornerareas.size();
			for (size_t i = 0; i < meshSize; ++i)
			{
				trimesh::TriMesh* mesh = inMeshes.at(i);
				if (mesh)
				{
					int vertexNum = (int)mesh->vertices.size();
					int faceNum = (int)mesh->faces.size();
					int uvNum = (int)mesh->cornerareas.size();
					if (vertexNum > 0 && faceNum > 0)
					{
						outMesh->vertices.insert(outMesh->vertices.end(), mesh->vertices.begin(), mesh->vertices.end());
						outMesh->cornerareas.insert(outMesh->cornerareas.end(), mesh->cornerareas.begin(), mesh->cornerareas.end());
						outMesh->faces.insert(outMesh->faces.end(), mesh->faces.begin(), mesh->faces.end());

						size_t endFaceIndex = startFaceIndex + faceNum;
						if (startVertexIndex > 0)
						{
							for (size_t ii = startFaceIndex; ii < endFaceIndex; ++ii)
							{
								trimesh::TriMesh::Face& face = outMesh->faces.at(ii);
								for (int j = 0; j < 3; ++j)
									face[j] += startVertexIndex;
							}
						}

						startFaceIndex += faceNum;
						startVertexIndex += vertexNum;
						startUVIndex += uvNum;

					}
				}
			}
		}

		return outMesh;
	}

	trimesh::TriMesh* partMesh(const std::vector<int>& indices, trimesh::TriMesh* inMesh)
	{
		trimesh::TriMesh* outMesh = nullptr;
		if (inMesh && indices.size() > 0 && inMesh->vertices.size() > 0)
		{
			size_t vertexSize = inMesh->vertices.size();
			outMesh = new trimesh::TriMesh();
			outMesh->faces.resize(indices.size());
			outMesh->vertices.reserve(3 * indices.size());

			std::vector<int> flags(vertexSize, -1);
			int startIndex = 0;
			int k = 0;
			for (int i : indices)
			{
				trimesh::TriMesh::Face& f = inMesh->faces.at(i);
				trimesh::TriMesh::Face& of = outMesh->faces.at(k);
				for (int j = 0; j < 3; ++j)
				{
					int index = f[j];
					if (flags.at(index) < 0)
					{
						flags.at(index) = startIndex++;
						outMesh->vertices.push_back(inMesh->vertices.at(index));
					}

					of[j] = flags.at(index);
				}

				++k;
			}
		}

		return outMesh;
	}

	std::vector < std::vector<BaseMeshPtr>> meshSplit(const std::vector<BaseMeshPtr>& meshes, ccglobal::Tracer* progressor)
	{
		std::vector < std::vector<BaseMeshPtr>> outMeshes;
		outMeshes.reserve(meshes.size());

		for (auto mesh : meshes)
		{
			int originV = mesh->vertices.size();
			originV = originV >= 0 ? originV : 1;
			
			dumplicateMesh(mesh.get(), nullptr);;

			if (!mesh || mesh->faces.size() <= 0)
			{
				continue;
			}

			if (progressor)
			{
				progressor->progress(0.1f);
			}

			MeshTopo topo;
			topo.build(mesh.get());
			if (progressor)
			{
				progressor->progress(0.3f);

				if (progressor->interrupt())
				{
					return outMeshes;
				}
			}

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

				if ((faceID + 1) % 100 == 0)
				{
					if (progressor->interrupt())
					{
						return outMeshes;
					}
				}

			}

			std::vector<trimesh::TriMesh*> meshes;
			int partSize = parts.size();
			for (int i = 0; i < partSize; ++i)
			{
				if (parts.at(i).size() > 10)
				{
					trimesh::TriMesh* outMesh = partMesh(parts.at(i), mesh.get());
					if (outMesh) meshes.push_back(outMesh);
				}
				else
				{
					double s = 0.0f;
					for (int j = 0; j < parts.at(i).size(); j++)
					{
						s += msbase::getFacetArea(mesh.get(), parts[i][j]);
					}
					if (s > MIN_AREA)
					{
						trimesh::TriMesh* outMesh = partMesh(parts.at(i), mesh.get());
						if (outMesh) meshes.push_back(outMesh);
					}
				}
			}

			//merge small ones
			int tSize = (int)meshes.size();
			int maxCount = 0;
			for (int i = 0; i < tSize; ++i)
			{
				if (maxCount < (int)meshes.at(i)->vertices.size())
					maxCount = (int)meshes.at(i)->vertices.size();
			}

			//int smallCount = (int)((float)maxCount * 0.05f);
			const int smallV = 150;
			const int samllF = 50;
			std::vector<trimesh::TriMesh*> allInOne;
			//std::vector<trimesh::TriMesh*> validMeshes;
			std::vector<BaseMeshPtr> validMeshes;
			for (int i = 0; i < tSize; ++i)
			{
				//if ((int)meshes.at(i)->vertices.size() < smallCount)
				float ratio = meshes.at(i)->vertices.size() * 1.0f / originV;
				if ((int)meshes.at(i)->vertices.size() < smallV
					&& (int)meshes.at(i)->faces.size() < samllF
					&& ratio < 0.1f)
					allInOne.push_back(meshes.at(i));
				else
					validMeshes.push_back(BaseMeshPtr(meshes.at(i)));
			}

			if (allInOne.size() > 0)
			{
				trimesh::TriMesh* newMesh = mergeMeshes(allInOne);
				validMeshes.push_back(BaseMeshPtr(newMesh));

				for (trimesh::TriMesh* m : allInOne)
					delete m;
				allInOne.clear();
			}
			outMeshes.push_back(validMeshes);
		}

		return outMeshes;
	}

	void copyTrimesh2Trimesh(trimesh::TriMesh* source, trimesh::TriMesh* dest)
	{
		if (source && dest)
		{
			dest->vertices = source->vertices;
			dest->faces = source->faces;
			dest->bbox = source->bbox;
			dest->flags = source->flags;

		}
	}
}