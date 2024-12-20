#pragma once

#include "MESHTOOL/MeshExplorer.hpp"

namespace MESHTOOL {

	MeshExplorer::MeshExplorer() {}

	std::unordered_set<MESH::VertexPtr> MeshExplorer::borderVertices(const MESH::MeshPtr& mesh)
	{
		std::unordered_set<MESH::VertexPtr> vertexSet;
		std::vector<MESH::VertexPtr> vertices = mesh->getVertices();
		for (auto& vertex : vertices)
		{
			if (vertex->isBorder())
			{
				vertexSet.insert(vertex);
			}
		}
		return vertexSet;
	}

}  // namespace MESHTOOL
