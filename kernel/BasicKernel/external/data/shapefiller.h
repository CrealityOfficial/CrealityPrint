#ifndef CREATIVE_KERNEL_SHAPEFILLER_1595391864496_H
#define CREATIVE_KERNEL_SHAPEFILLER_1595391864496_H
#include "basickernelexport.h"
#include "trimesh2/Vec.h"
#include "trimesh2/TriMesh.h"

namespace creative_kernel
{
	class BASIC_KERNEL_API ShapeFiller : public QObject
	{
		Q_OBJECT
	public:
		ShapeFiller(QObject* parent = nullptr);
		virtual ~ShapeFiller();

		static int fillSphere(trimesh::vec3* data, float radius, trimesh::vec3 center);
		static int fillLink(trimesh::vec3* data, float radius1, trimesh::vec3& center1, float radius2, trimesh::vec3& center2);
		static int fillCylinder(trimesh::vec3* data, float radius1, trimesh::vec3& center1, float radius2, trimesh::vec3& center2
			, int n, float theta);
	};

	class BASIC_KERNEL_API ShapeCreator : public QObject
	{
	public:
		ShapeCreator(QObject* parent = nullptr);
		virtual ~ShapeCreator();

		/* coordinates of the center: (0, 0, 0) */
		static void createTextMesh(QString text, QString font, float height, float thickness,
			std::vector<float>& widths, std::vector<trimesh::TriMesh*>& meshs);
	};
}
#endif // CREATIVE_KERNEL_SHAPEFILLER_1595391864496_H