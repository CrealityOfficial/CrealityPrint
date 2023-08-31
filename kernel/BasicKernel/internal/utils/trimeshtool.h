#ifndef CREATIVE_KERNEL_TRIMESHTOOL_1677123542670_H
#define CREATIVE_KERNEL_TRIMESHTOOL_1677123542670_H
#include "trimesh2/Vec.h"
#include <vector>

namespace creative_kernel
{
	float getAngelOfTwoVector(const trimesh::vec& pt1, const trimesh::vec& pt2, const trimesh::vec& c);
	void getDevidePoint(const trimesh::vec& p0, const trimesh::vec& p1,
		std::vector<trimesh::vec>& out, float theta, bool clockwise);
}

#endif // CREATIVE_KERNEL_TRIMESHTOOL_1677123542670_H