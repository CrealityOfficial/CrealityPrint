#pragma once

#include "MESHTOOL/MeshIntersectionChecker.hpp"

namespace MESHTOOL {

	MeshIntersectionChecker::MeshIntersectionChecker() {}

	bool MeshIntersectionChecker::borderContact(const MESH::MeshPtr& mesh1, const MESH::MeshPtr& mesh2)
	{
		MeshExplorer meshExplorer;
		auto borderVertices1 = meshExplorer.borderVertices(mesh1);
		auto borderVertices2 = meshExplorer.borderVertices(mesh2);

		std::unordered_set<MESH::VertexHash> hashSet1;
		std::unordered_set<MESH::VertexHash> hashSet2;
		for (auto &vertex: borderVertices1)
		{
			hashSet1.insert(vertex->getId());
		}
		for (auto& vertex : borderVertices2)
		{
			hashSet2.insert(vertex->getId());
		}
		std::unordered_set<MESH::VertexHash> intersection;
		for (const auto& value : hashSet1) {
			if (hashSet2.find(value) != hashSet2.end()) {
				intersection.insert(value);
			}
		}
		
		if (intersection.size() > 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

}  // namespace MESHTOOL
