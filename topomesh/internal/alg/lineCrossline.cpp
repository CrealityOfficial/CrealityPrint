#include "lineCrossline.h"

namespace topomesh {
	bool calculateCrossPoint4Dim2(const std::pair<trimesh::vec2, trimesh::vec2>& line1, const std::pair<trimesh::vec2, trimesh::vec2>& line2, trimesh::vec2& result)
	{
		trimesh::point b1 = trimesh::point(line1.first.x, line1.first.y, 0);
		trimesh::point b2 = trimesh::point(line1.second.x, line1.second.y, 0);
		trimesh::point a1 = trimesh::point(line2.first.x, line2.first.y, 0);
		trimesh::point a2 = trimesh::point(line2.second.x, line2.second.y, 0);
		trimesh::point a = a1 - a2;//a2->a1
		trimesh::point b = b1 - b2;//b2->b1
		trimesh::point n1 = a1 - b1;//b1->a1
		trimesh::point n2 = a1 - b2;//b2->a1
		trimesh::point n3 = a2 - b1;//b1->a2								
		if (((-a % -n2) ^ (-a % -n1)) >= 0 || ((-b % n1) ^ (-b % n3)) >= 0) return false;;
		trimesh::point m = a % b;
		trimesh::point n = n1 % a;
		float t = (m.x != 0 && n.x != 0) ? n.x / m.x : (m.y != 0 && n.y != 0) ? n.y / m.y : (m.z != 0 && n.z != 0) ? n.z / m.z : -1;
		if (t > 0 && t < 1)//b1-t*b							
		{			
			result = b1 - t * b;
			return true;
		}
		return false;
	}
}