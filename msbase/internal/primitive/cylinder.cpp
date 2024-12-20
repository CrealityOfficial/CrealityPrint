#include "msbase/primitive/cylinder.h"
#include "trimesh2/TriMesh_algo.h"

#include "msbase/data/conv.h"
#include "msbase/mesh/get.h"
#include "msbase/mesh/merge.h"
#include "msbase/mesh/dumplicate.h"
#include "msbase/mesh/tinymodify.h"

#define  PI 3.141592 

namespace msbase
{

	trimesh::TriMesh* createCylinder(float _radius, float _height)
	{
		trimesh::TriMesh* cylinderMesh = new trimesh::TriMesh();
		int degrees = 4;
		int  trianglesCount = int(360 / degrees);//每4度一个三角形，一共有90个三角形
		float radian = degrees * (PI / 180.0);//4度换算成弧度
	
		for (int i=0;i< trianglesCount;i++)
		{
			//top face
			trimesh::point point0(0, 0, _height);
			trimesh::point point1(_radius * cos(i * radian), _radius * sin(i * radian), _height);
			trimesh::point point2(_radius * cos((i+1)* radian), _radius *sin((i+1)*radian), _height);
			int v = cylinderMesh->vertices.size();
			cylinderMesh->vertices.push_back(point0);
			cylinderMesh->vertices.push_back(point1);
			cylinderMesh->vertices.push_back(point2);
			cylinderMesh->faces.push_back(trimesh::TriMesh::Face(v, v + 1, v + 2));
			//Side 1a
			point0 = trimesh::point(_radius * cos(i * radian), _radius * sin(i * radian), _height);
			point1 = trimesh::point(_radius * cos(i * radian), _radius * sin(i * radian), 0);
			point2 = trimesh::point(_radius * cos((i + 1) * radian), _radius * sin((i + 1) * radian), _height);
			v = cylinderMesh->vertices.size();
			cylinderMesh->vertices.push_back(point0);
			cylinderMesh->vertices.push_back(point1);
			cylinderMesh->vertices.push_back(point2);
			cylinderMesh->faces.push_back(trimesh::TriMesh::Face(v, v + 1, v + 2));
			//#Side 1b
			point0 = trimesh::point(_radius * cos(i * radian), _radius * sin(i * radian), 0);
			point1 = trimesh::point(_radius * cos((i + 1) * radian), _radius * sin((i + 1) * radian), 0);
			point2 = trimesh::point(_radius * cos((i + 1) * radian), _radius * sin((i + 1) * radian), _height);
			v = cylinderMesh->vertices.size();
			cylinderMesh->vertices.push_back(point0);
			cylinderMesh->vertices.push_back(point1);
			cylinderMesh->vertices.push_back(point2);
			cylinderMesh->faces.push_back(trimesh::TriMesh::Face(v, v + 1, v + 2));
			//#Bottom 
			point0 = trimesh::point(0, 0, 0);
			point1 = trimesh::point(_radius * cos(i * radian), _radius * sin(i * radian), 0);
			point2 = trimesh::point(_radius * cos((i + 1) * radian), _radius * sin((i + 1) * radian), 0);
			v = cylinderMesh->vertices.size();
			cylinderMesh->vertices.push_back(point0);
			cylinderMesh->vertices.push_back(point1);
			cylinderMesh->vertices.push_back(point2);
			cylinderMesh->faces.push_back(trimesh::TriMesh::Face(v, v + 1, v + 2));
		}
		dumplicateMesh(cylinderMesh);
		return cylinderMesh;
	}

	trimesh::TriMesh* createSoupObliqueCylinder(int count, const trimesh::vec2& v1, const trimesh::vec2& v2, float _radius)
	{
		int  trianglesCount = count;
		if (trianglesCount < 3)
			trianglesCount = 3;
		trimesh::TriMesh* cylinderMesh = new trimesh::TriMesh();
		float radian = 2.0f * PI / (float)trianglesCount;//4度换算成弧度	
		float _height = trimesh::dist(v1, v2);
		//float _height = 200;
		trimesh::vec3 top(v1[0], v1[1], _radius);
		trimesh::vec3 bottom(v2[0], v2[1], _radius);
		trimesh::vec3 offset(0.0f, _height, 0.0f );
		std::vector<trimesh::vec3> circles(trianglesCount, trimesh::vec3());
		std::vector<trimesh::vec3> circlesup(trianglesCount, trimesh::vec3());
		for (int i = 0; i < trianglesCount; ++i)
		{
			circlesup[i] = trimesh::vec3( _radius * cos(i * radian) + v2[0], v2[1],
				_radius * sin(i * radian) + _radius);

			circles[i] = trimesh::vec3(_radius * cos(i * radian) + v1[0], v1[1],
				_radius * sin(i * radian) + _radius);
		}

		circles.push_back(circles[0]);
		circlesup.push_back(circlesup[0]);
		cylinderMesh->vertices.resize(12 * trianglesCount);
		int index = 0;

		auto f = [&index, &cylinderMesh](const trimesh::vec3& v1, const trimesh::vec3& v2, const trimesh::vec3& v3) {
			cylinderMesh->vertices[index++] = v1;
			cylinderMesh->vertices[index++] = v2;
			cylinderMesh->vertices[index++] = v3;
		};
		for (int i = 0; i < trianglesCount; ++i)
		{
			f(top, circles[i + 1], circles[i]);
			f(circlesup[i] , circles[i], circlesup[i + 1] );
			f(circlesup[i + 1] , circles[i], circles[i + 1]);
			f(circlesup[i], circlesup[i + 1], bottom);
		}

		fillTriangleSoupFaceIndex(cylinderMesh);
		return cylinderMesh;

	}

	//trimesh::TriMesh* createSoupObliqueCylinder(int count, const trimesh::vec2& v1, const trimesh::vec2& v2, float _radius)
	//{
	//	int  trianglesCount = count;
	//	if (trianglesCount < 3)
	//		trianglesCount = 3;
	//	trimesh::TriMesh* cylinderMesh = new trimesh::TriMesh();
	//	float radian = 2.0f * PI / (float)trianglesCount;//4度换算成弧度	
	//	float _height = trimesh::dist(v1, v2);
	//	//float _height = 200;
	//	trimesh::vec3 top(v1[0], v1[1], _height / 2.0f);
	//	trimesh::vec3 bottom(v2[0], v2[1], -_height / 2.0f);
	//	trimesh::vec3 offset(0.0f, 0.0f, _height);
	//	std::vector<trimesh::vec3> circles(trianglesCount, trimesh::vec3());
	//	std::vector<trimesh::vec3> circlesup(trianglesCount, trimesh::vec3());
	//	for (int i = 0; i < trianglesCount; ++i)
	//	{
	//		circlesup[i] = trimesh::vec3(_radius * cos(i * radian) + v1[0],
	//			_radius * sin(i * radian) + v1[1], -_height / 2.0f);
	//		circles[i] = trimesh::vec3(_radius * cos(i * radian) + v2[0],
	//			_radius * sin(i * radian) + v2[1], -_height / 2.0f);
	//	}

	//	circles.push_back(circles[0]);
	//	circlesup.push_back(circlesup[0]);
	//	cylinderMesh->vertices.resize(12 * trianglesCount);
	//	int index = 0;

	//	auto f = [&index, &cylinderMesh](const trimesh::vec3& v1, const trimesh::vec3& v2, const trimesh::vec3& v3) {
	//		cylinderMesh->vertices[index++] = v1;
	//		cylinderMesh->vertices[index++] = v2;
	//		cylinderMesh->vertices[index++] = v3;
	//	};
	//	for (int i = 0; i < trianglesCount; ++i)
	//	{
	//		f(top, circlesup[i] + offset, circlesup[i + 1] + offset);
	//		f(circlesup[i] + offset, circles[i], circlesup[i + 1] + offset);
	//		f(circlesup[i + 1] + offset, circles[i], circles[i + 1]);
	//		f(bottom, circles[i + 1], circles[i]);
	//	}

	//	fillTriangleSoupFaceIndex(cylinderMesh);
	//	return cylinderMesh;
	//
	//}

	trimesh::TriMesh* createSoupObliqueCylinder(int count, float _radius, float _height)
	{
		int  trianglesCount = count;
		if (trianglesCount < 3)
			trianglesCount = 3;
		trimesh::TriMesh* cylinderMesh = new trimesh::TriMesh();
		float radian = 2.0f * PI / (float)trianglesCount;//4度换算成弧度

		trimesh::vec3 top(20.0f, 0.0f, _height / 2.0f);
		trimesh::vec3 bottom(0.0f, 0.0f, -_height / 2.0f);
		trimesh::vec3 offset(0.0f, 0.0f, _height);
		std::vector<trimesh::vec3> circles(trianglesCount, trimesh::vec3());
		std::vector<trimesh::vec3> circlesup(trianglesCount, trimesh::vec3());
		for (int i = 0; i < trianglesCount; ++i)
		{
			circles[i] = trimesh::vec3(_radius * cos(i * radian),
				_radius * sin(i * radian), -_height / 2.0f);
			circlesup[i] = trimesh::vec3(_radius * cos(i * radian) + 20.0f,
				_radius * sin(i * radian), -_height / 2.0f);
		}

		circles.push_back(circles[0]);
		circlesup.push_back(circlesup[0]);
		cylinderMesh->vertices.resize(12 * trianglesCount);
		int index = 0;

		auto f = [&index, &cylinderMesh](const trimesh::vec3& v1, const trimesh::vec3& v2, const trimesh::vec3& v3) {
			cylinderMesh->vertices[index++] = v1;
			cylinderMesh->vertices[index++] = v2;
			cylinderMesh->vertices[index++] = v3;
		};
		for (int i = 0; i < trianglesCount; ++i)
		{
			f(top, circlesup[i] + offset, circlesup[i + 1] + offset);
			f(circlesup[i] + offset, circles[i], circlesup[i + 1] + offset);
			f(circlesup[i + 1] + offset, circles[i], circles[i + 1]);
			f(bottom, circles[i + 1], circles[i]);
		}

		fillTriangleSoupFaceIndex(cylinderMesh);
		return cylinderMesh;
	}

	trimesh::TriMesh* createSoupCylinder(int count, float _radius, float _height, bool offsetOnZero)
	{
		int  trianglesCount = count;
		if (trianglesCount < 3)
			trianglesCount = 3;
		trimesh::TriMesh* cylinderMesh = new trimesh::TriMesh();
		float radian = 2.0f * PI / (float)trianglesCount;//4度换算成弧度

		trimesh::vec3 top(0.0f, 0.0f, _height / 2.0f);
		trimesh::vec3 bottom(0.0f, 0.0f, - _height / 2.0f);
		trimesh::vec3 offset(0.0f, 0.0f, _height);
		std::vector<trimesh::vec3> circles(trianglesCount, trimesh::vec3());
		for (int i = 0; i < trianglesCount; ++i)
			circles[i] = trimesh::vec3(_radius * cos(i * radian),
				_radius * sin(i * radian), -_height / 2.0f);
		circles.push_back(circles[0]);
		cylinderMesh->vertices.resize(12 * trianglesCount);
		int index = 0;

		auto f = [&index, &cylinderMesh](const trimesh::vec3& v1, const trimesh::vec3& v2, const trimesh::vec3& v3) {
			cylinderMesh->vertices[index++] = v1;
			cylinderMesh->vertices[index++] = v2;
			cylinderMesh->vertices[index++] = v3;
		};
		for (int i = 0; i < trianglesCount; ++i)
		{
			f(top, circles[i] + offset, circles[i + 1] + offset);
			f(circles[i] + offset, circles[i], circles[i + 1] + offset);
			f(circles[i + 1] + offset, circles[i], circles[i + 1]);
			f(bottom, circles[i + 1], circles[i]);
		}

		fillTriangleSoupFaceIndex(cylinderMesh);

		if(offsetOnZero)
			trimesh::apply_xform(cylinderMesh, trimesh::xform::trans(0.0f, 0.0f, _height / 2.0f));
		return cylinderMesh;
	}

	trimesh::TriMesh* createSoupCylinder(int count, float _radius, float _height,
		const trimesh::vec3& centerPoint, const trimesh::vec3& normal)
	{
		trimesh::TriMesh* mesh = createSoupCylinder(count, _radius, _height);

		const trimesh::vec3 cyOriginNormal(0.0f, 0.0f, 1.0f);
		trimesh::quaternion q = trimesh::quaternion::rotationTo(normal, cyOriginNormal);
		trimesh::fxform xf = trimesh::fxform::trans(centerPoint) * msbase::fromQuaterian(q);
		trimesh::apply_xform(mesh, trimesh::xform(xf));

		return mesh;
	}

	trimesh::TriMesh* createCylinderBasepos(int count, float radius, float height, const trimesh::vec3& sp, const trimesh::vec3& normal)
	{

		trimesh::TriMesh* cylinderMesh = new trimesh::TriMesh();

		auto steps = int(count);
		auto& points = cylinderMesh->vertices;
		auto& indices = cylinderMesh->faces;
		points.reserve(2 * count);
		double a = 2 * PI / steps;

		trimesh::vec3 jp = sp;
		trimesh::vec3 endp (sp.x, sp.y, sp.z + height);

		// Upper circle points
		for (int i = 0; i < steps; ++i) {
			double phi = i * a;
			double ex = endp.x + radius * std::cos(phi);
			double ey = endp.y + radius * std::sin(phi);
			points.emplace_back(ex, ey, endp.z);
		}

		// Lower circle points
		for (int i = 0; i < steps; ++i) {
			double phi = i * a;
			double x = jp.x + radius * std::cos(phi);
			double y = jp.y + radius * std::sin(phi);
			points.emplace_back(x, y, jp.z);
		}

		// Now create long triangles connecting upper and lower circles
		indices.reserve(2 * count);
		auto offs = steps;
		for (int i = 0; i < steps - 1; ++i) {
			indices.emplace_back(i, i + offs, offs + i + 1);
			indices.emplace_back(i, offs + i + 1, i + 1);
		}

		// Last triangle connecting the first and last vertices
		auto last = steps - 1;
		indices.emplace_back(0, last, offs);
		indices.emplace_back(last, offs + last, offs);

		// According to the slicing algorithms, we need to aid them with generating
		// a watertight body. So we create a triangle fan for the upper and lower
		// ending of the cylinder to close the geometry.
		points.emplace_back(jp); int ci = int(points.size() - 1);
		for (int i = 0; i < steps - 1; ++i)
			indices.emplace_back(i + offs + 1, i + offs, ci);

		indices.emplace_back(offs, steps + offs - 1, ci);

		points.emplace_back(endp); ci = int(points.size() - 1);
		for (int i = 0; i < steps - 1; ++i)
			indices.emplace_back(ci, i, i + 1);

		indices.emplace_back(steps - 1, 0, ci);
		dumplicateMesh(cylinderMesh);

		const trimesh::vec3 cydestNormal(0.0f, 0.0f, 1.0f);
		trimesh::quaternion q = trimesh::quaternion::rotationTo(trimesh::normalized(normal), cydestNormal);
		trimesh::fxform xf = msbase::fromQuaterian(q);
		trimesh::apply_xform(cylinderMesh, trimesh::xform(xf));

		return cylinderMesh;
	}
	
	trimesh::TriMesh* createCylinderWallBasepos(int count, float radius, float height, const trimesh::vec3& sp)
	{

		trimesh::TriMesh* cylinderMesh = new trimesh::TriMesh();

		auto steps = int(count);
		auto& points = cylinderMesh->vertices;
		auto& indices = cylinderMesh->faces;
		points.reserve(2 * count);
		double a = 2 * PI / steps;

		trimesh::vec3 jp = sp;
		trimesh::vec3 endp(sp.x, sp.y, sp.z + height);

		// Upper circle points
		for (int i = 0; i < steps; ++i) {
			double phi = i * a;
			double ex = endp.x + radius * std::cos(phi);
			double ey = endp.y + radius * std::sin(phi);
			points.emplace_back(ex, ey, endp.z);
		}

		// Lower circle points
		for (int i = 0; i < steps; ++i) {
			double phi = i * a;
			double x = jp.x + radius * std::cos(phi);
			double y = jp.y + radius * std::sin(phi);
			points.emplace_back(x, y, jp.z);
		}

		// Now create long triangles connecting upper and lower circles
		indices.reserve(2 * count);
		auto offs = steps;
		for (int i = 0; i < steps - 1; ++i) {
			indices.emplace_back(i, i + offs, offs + i + 1);
			indices.emplace_back(i, offs + i + 1, i + 1);
		}

		// Last triangle connecting the first and last vertices
		auto last = steps - 1;
		indices.emplace_back(0, last, offs);
		indices.emplace_back(last, offs + last, offs);

		return cylinderMesh;
	}
	trimesh::TriMesh* createHollowCylinder(trimesh::TriMesh* wallOutter, trimesh::TriMesh* wallInner, int sectionCount, const trimesh::vec3& normal)
	{
		//mmesh::reverseTriMesh(cylinderMeshInner);
		int vertexsizeOutter = wallOutter->vertices.size();

		std::vector<trimesh::TriMesh*> meshes;
		meshes.push_back(wallOutter);
		meshes.push_back(wallInner);
		trimesh::TriMesh* mergedMesh = mergeMeshes(meshes);
		
		// Now create upper triangles connecting outter and inner circles wall
		auto offs = vertexsizeOutter;
		for (int i = 0; i < sectionCount - 1; ++i) {
			int k0 = i;
			int k1 = k0 + offs;
			mergedMesh->faces.emplace_back(  k1, k0, k1 + 1);
			mergedMesh->faces.emplace_back(k0 + 1, k1 + 1, k0);
		}

		// Last upper triangle connecting the first and last vertices
		auto last = sectionCount - 1;
		mergedMesh->faces.emplace_back(offs, last,0 );
		mergedMesh->faces.emplace_back(offs, offs + last, last);

		// Now create lower triangles connecting outter and inner circles wall
		offs = vertexsizeOutter;
		for (int i = sectionCount; i < vertexsizeOutter - 1; ++i) {
			int k0 = i;
			int k1 = k0 + offs;
			mergedMesh->faces.emplace_back(k0, k1, k1 + 1);
			mergedMesh->faces.emplace_back(k0, k1 + 1, k0 + 1);
		}

		// Last lower triangle connecting the first and last vertices
		last = vertexsizeOutter - 1;

		mergedMesh->faces.emplace_back(sectionCount, last, sectionCount +offs);
		mergedMesh->faces.emplace_back(sectionCount + offs,  last, last+offs);


		dumplicateMesh(mergedMesh);

		const trimesh::vec3 cydestNormal(0.0f, 0.0f, 1.0f);
		trimesh::quaternion q = trimesh::quaternion::rotationTo(trimesh::normalized(normal), cydestNormal);
		trimesh::fxform xf = msbase::fromQuaterian(q);
		trimesh::apply_xform(mergedMesh, trimesh::xform(xf));

		return mergedMesh;
	}

}