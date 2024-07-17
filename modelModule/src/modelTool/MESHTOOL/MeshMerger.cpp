#include "MESHTOOL/MeshMerger.hpp"

namespace MESHTOOL {

	MeshMerger::MeshMerger() {}

	
	MESH::MeshPtr MeshMerger::merge(const std::vector<MESH::MeshPtr>& meshes)
	{
		if (meshes.size() == 1)
		{
			MESH::MeshPtr meshResult = std::make_shared<MESH::Mesh>(*(meshes.at(0)));
			return meshResult;
		}
		else
		{
			MESH::MeshPtr meshResult = std::make_shared<MESH::Mesh>();
			for (auto &mesh: meshes)
			{
				meshResult->addFaces(mesh->getFaces());
			}
			return meshResult;
		}
	}
	
	MESH::TopologyMeshPtr MeshMerger::merge(const std::vector<MESH::TopologyMeshPtr>& meshes)
	{
		if (meshes.size() == 1)
		{
			MESH::TopologyMeshPtr meshResult = std::make_shared<MESH::TopologyMesh>(*(meshes.at(0)));
			return meshResult;
		}
		else
		{
			MESH::TopologyMeshPtr meshResult = std::make_shared<MESH::TopologyMesh>();
			for (auto& mesh : meshes)
			{
				meshResult->addFaces(mesh->getFaces());
			}
			return meshResult;
		}
	}

	MESH::SimpleMeshPtr MeshMerger::merge(const std::vector<MESH::SimpleMeshPtr>& meshes)
	{
		if (meshes.size() == 1)
		{
			MESH::SimpleMeshPtr meshResult = std::make_shared<MESH::SimpleMesh>(*(meshes.at(0)));
			return meshResult;
		}
		else
		{
			MESH::SimpleMeshPtr meshResult = std::make_shared<MESH::SimpleMesh>();
			unsigned int index = 0;
			for (auto& mesh : meshes)
			{
				auto points = mesh->getPoints();
				auto faces = mesh->getFaces();
				for (auto& face: faces)
				{
					GEOM::Point& p0 = points.at(face.at(0));
					GEOM::Point& p1 = points.at(face.at(1));
					GEOM::Point& p2 = points.at(face.at(2));
					meshResult->addPointAndFace(p0, p1, p2);
				}
			}
			return meshResult;
		}
	}

}