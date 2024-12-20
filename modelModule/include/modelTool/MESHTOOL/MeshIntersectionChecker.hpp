#pragma once

#include "MESHTOOL/MeshExplorer.hpp"

namespace MESHTOOL {

	class MeshIntersectionChecker {
	public:

		MeshIntersectionChecker();

		// 判断两个mesh是否边界碰撞
		bool borderContact(const MESH::MeshPtr &mesh1, const MESH::MeshPtr& mesh2);

	private:

	};

}  // namespace MESHTOOL
