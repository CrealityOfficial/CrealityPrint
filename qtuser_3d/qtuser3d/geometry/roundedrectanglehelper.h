#ifndef QTUSER_3D_ROUNDED_RECTANGLE_HELPER_202311101628_H
#define QTUSER_3D_ROUNDED_RECTANGLE_HELPER_202311101628_H

#include "qtuser3d/qtuser3dexport.h"
#include <vector>
#include "trimesh2/Vec.h"

namespace qtuser_3d {

	QTUSER_3D_API void makeQuad(trimesh::vec3 a, trimesh::vec3 b, std::vector<trimesh::vec3>* vertex);

	QTUSER_3D_API void makeCircle(trimesh::vec3 o, float radius, std::vector<trimesh::vec3>* vertex);

	/*
	o为原点，size是vertex的分布区域，生成uv坐标
	*/
	QTUSER_3D_API void makeTextureCoordinate(const std::vector<trimesh::vec3>* vertex, trimesh::vec3 o, trimesh::vec2 size, std::vector<trimesh::vec2>* uvs);

	class QTUSER_3D_API RoundedRectangle {
	public:
		enum Corner {
			TopLeft = 1,
			TopRight = 2,
			BottomLeft = 4,
			BottomRight = 8
		}; 

		typedef int RoundedRectangleCorners;

		static void makeRoundedRectangle(trimesh::vec3 origin, float width, float height, RoundedRectangleCorners corners, float radius, std::vector<trimesh::vec3>* vertex, std::vector<trimesh::vec2>* uvs);
		
	};
	
};


#endif //QTUSER_3D_ROUNDED_RECTANGLE_HELPER_202311101628_H