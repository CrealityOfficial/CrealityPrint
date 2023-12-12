#include "msbase/primitive/primitive.h"
#include "trimesh2/quaternion.h"

namespace msbase
{
	/// <summary>
	/// sphere
	/// </summary>
	namespace CubeToSphere
	{
		static const trimesh::vec3 origins[6] =
		{
			 trimesh::vec3(-1.0, -1.0, -1.0),
			 trimesh::vec3(1.0, -1.0, -1.0),
			 trimesh::vec3(1.0, -1.0, 1.0),
			 trimesh::vec3(-1.0, -1.0, 1.0),
			 trimesh::vec3(-1.0, 1.0, -1.0),
			 trimesh::vec3(-1.0, -1.0, 1.0)
		};
		static const trimesh::vec3 rights[6] =
		{
			 trimesh::vec3(2.0, 0.0, 0.0),
			 trimesh::vec3(0.0, 0.0, 2.0),
			 trimesh::vec3(-2.0, 0.0, 0.0),
			 trimesh::vec3(0.0, 0.0, -2.0),
			 trimesh::vec3(2.0, 0.0, 0.0),
			 trimesh::vec3(2.0, 0.0, 0.0)
		};
		static const trimesh::vec3 ups[6] =
		{
			 trimesh::vec3(0.0, 2.0, 0.0),
			 trimesh::vec3(0.0, 2.0, 0.0),
			 trimesh::vec3(0.0, 2.0, 0.0),
			 trimesh::vec3(0.0, 2.0, 0.0),
			 trimesh::vec3(0.0, 0.0, 2.0),
			 trimesh::vec3(0.0, 0.0, -2.0)
		};
	};
	struct Mesh_sphere
	{
		std::vector<trimesh::vec3> vertices;
		std::vector<uint32_t> triangles;
		void addQuad(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
		{
			triangles.emplace_back(a);
			triangles.emplace_back(b);
			triangles.emplace_back(c);
			triangles.emplace_back(a);
			triangles.emplace_back(c);
			triangles.emplace_back(d);
		}

		void addQuadAlt(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
		{
			triangles.emplace_back(a);
			triangles.emplace_back(b);
			triangles.emplace_back(d);
			triangles.emplace_back(b);
			triangles.emplace_back(c);
			triangles.emplace_back(d);
		}
		trimesh::TriMesh* convert()
		{
			trimesh::TriMesh* mesh = new trimesh::TriMesh();
			for (int i = 0; i < this->triangles.size() / 3; i++)
			{
				trimesh::TriMesh::Face f(this->triangles.at(i * 3), this->triangles.at(i * 3 + 1), this->triangles.at(i * 3 + 2));
				mesh->faces.emplace_back(f);

			}
			for (int i = 0; i < this->vertices.size(); i++)
			{
				trimesh::vec3 p = this->vertices.at(i);
				mesh->vertices.emplace_back(trimesh::vec3(p.x, p.y, p.z));
			}
			return mesh;
		}
	};

	struct Edge
	{
		uint32_t v0;
		uint32_t v1;

		Edge(uint32_t v0, uint32_t v1)
			: v0(v0 < v1 ? v0 : v1)
			, v1(v0 < v1 ? v1 : v0)
		{
		}

		bool operator <(const Edge& rhs) const
		{
			return v0 < rhs.v0 || (v0 == rhs.v0 && v1 < rhs.v1);
		}
	};

	void subdivison(trimesh::TriMesh* mesh, int num_iter)
	{
		for (size_t i = 0; i < num_iter; i++)
		{
			size_t num_faces = mesh->faces.size();

			for (size_t fid = 0; fid < num_faces; fid++)
			{
				int vid1 = mesh->faces[fid][0];
				int vid2 = mesh->faces[fid][1];
				int vid3 = mesh->faces[fid][2];

				size_t num_vertices = mesh->vertices.size();
				mesh->vertices.emplace_back(
					(mesh->vertices[vid1].x + mesh->vertices[vid2].x) * 0.5f,
					(mesh->vertices[vid1].y + mesh->vertices[vid2].y) * 0.5f,
					(mesh->vertices[vid1].z + mesh->vertices[vid2].z) * 0.5f);
				mesh->vertices.emplace_back(
					(mesh->vertices[vid2].x + mesh->vertices[vid3].x) * 0.5f,
					(mesh->vertices[vid2].y + mesh->vertices[vid3].y) * 0.5f,
					(mesh->vertices[vid2].z + mesh->vertices[vid3].z) * 0.5f);
				mesh->vertices.emplace_back(
					(mesh->vertices[vid3].x + mesh->vertices[vid1].x) * 0.5f,
					(mesh->vertices[vid3].y + mesh->vertices[vid1].y) * 0.5f,
					(mesh->vertices[vid3].z + mesh->vertices[vid1].z) * 0.5f);

				// replace current face with 4 small faces
				mesh->faces.emplace_back(vid1, num_vertices, num_vertices + 2);
				mesh->faces.emplace_back(vid2, num_vertices + 1, num_vertices);
				mesh->faces.emplace_back(vid3, num_vertices + 2, num_vertices + 1);
				mesh->faces[fid].set(num_vertices, num_vertices + 1, num_vertices + 2);
			}
		}
	}
	trimesh::TriMesh* Icosahedron(int num_iter)
	{
		trimesh::TriMesh* mesh = new trimesh::TriMesh();
		const double t = (1.0 + std::sqrt(5.0)) / 2.0;

		// Vertices
		mesh->vertices.emplace_back(trimesh::normalized(trimesh::vec3(-1.0, t, 0.0)));
		mesh->vertices.emplace_back(trimesh::normalized(trimesh::vec3(1.0, t, 0.0)));
		mesh->vertices.emplace_back(trimesh::normalized(trimesh::vec3(-1.0, -t, 0.0)));
		mesh->vertices.emplace_back(trimesh::normalized(trimesh::vec3(1.0, -t, 0.0)));
		mesh->vertices.emplace_back(trimesh::normalized(trimesh::vec3(0.0, -1.0, t)));
		mesh->vertices.emplace_back(trimesh::normalized(trimesh::vec3(0.0, 1.0, t)));
		mesh->vertices.emplace_back(trimesh::normalized(trimesh::vec3(0.0, -1.0, -t)));
		mesh->vertices.emplace_back(trimesh::normalized(trimesh::vec3(0.0, 1.0, -t)));
		mesh->vertices.emplace_back(trimesh::normalized(trimesh::vec3(t, 0.0, -1.0)));
		mesh->vertices.emplace_back(trimesh::normalized(trimesh::vec3(t, 0.0, 1.0)));
		mesh->vertices.emplace_back(trimesh::normalized(trimesh::vec3(-t, 0.0, -1.0)));
		mesh->vertices.emplace_back(trimesh::normalized(trimesh::vec3(-t, 0.0, 1.0)));

		// Faces
		mesh->faces.emplace_back(trimesh::TriMesh::Face(0, 11, 5));
		mesh->faces.emplace_back(trimesh::TriMesh::Face(0, 5, 1));
		mesh->faces.emplace_back(trimesh::TriMesh::Face(0, 1, 7));
		mesh->faces.emplace_back(trimesh::TriMesh::Face(0, 7, 10));
		mesh->faces.emplace_back(trimesh::TriMesh::Face(0, 10, 11));
		mesh->faces.emplace_back(trimesh::TriMesh::Face(1, 5, 9));
		mesh->faces.emplace_back(trimesh::TriMesh::Face(5, 11, 4));
		mesh->faces.emplace_back(trimesh::TriMesh::Face(11, 10, 2));
		mesh->faces.emplace_back(trimesh::TriMesh::Face(10, 7, 6));
		mesh->faces.emplace_back(trimesh::TriMesh::Face(7, 1, 8));
		mesh->faces.emplace_back(trimesh::TriMesh::Face(3, 9, 4));
		mesh->faces.emplace_back(trimesh::TriMesh::Face(3, 4, 2));
		mesh->faces.emplace_back(trimesh::TriMesh::Face(3, 2, 6));
		mesh->faces.emplace_back(trimesh::TriMesh::Face(3, 6, 8));
		mesh->faces.emplace_back(trimesh::TriMesh::Face(3, 8, 9));
		mesh->faces.emplace_back(trimesh::TriMesh::Face(4, 9, 5));
		mesh->faces.emplace_back(trimesh::TriMesh::Face(2, 4, 11));
		mesh->faces.emplace_back(trimesh::TriMesh::Face(6, 2, 10));
		mesh->faces.emplace_back(trimesh::TriMesh::Face(8, 6, 7));
		mesh->faces.emplace_back(trimesh::TriMesh::Face(9, 8, 1));

		subdivison(mesh, num_iter);


		return mesh;
	}

	trimesh::TriMesh* SpherifiedCube(int divisions)
	{

		Mesh_sphere mesh_sphere;

		const double step = 1.0 / double(divisions);
		const trimesh::vec3 step3(step, step, step);

		for (uint32_t face = 0; face < 6; ++face)
		{
			const trimesh::vec3 origin = CubeToSphere::origins[face];
			const trimesh::vec3 right = CubeToSphere::rights[face];
			const trimesh::vec3 up = CubeToSphere::ups[face];
			for (uint32_t j = 0; j < divisions + 1; ++j)
			{
				const trimesh::vec3 j3(j, j, j);
				for (uint32_t i = 0; i < divisions + 1; ++i)
				{
					const trimesh::vec3 i3(i, i, i);
					const trimesh::vec3 p = origin + step3 * (i3 * right + j3 * up);
					const trimesh::vec3 p2 = p * p;
					const trimesh::vec3 n
					(
						p.x * std::sqrt(1.0 - 0.5 * (p2.y + p2.z) + p2.y * p2.z / 3.0),
						p.y * std::sqrt(1.0 - 0.5 * (p2.z + p2.x) + p2.z * p2.x / 3.0),
						p.z * std::sqrt(1.0 - 0.5 * (p2.x + p2.y) + p2.x * p2.y / 3.0)
					);
					mesh_sphere.vertices.emplace_back(n);
				}
			}
		}

		const uint32_t k = divisions + 1;
		for (uint32_t face = 0; face < 6; ++face)
		{
			for (uint32_t j = 0; j < divisions; ++j)
			{
				const bool bottom = j < (divisions / 2);
				for (uint32_t i = 0; i < divisions; ++i)
				{
					const bool left = i < (divisions / 2);
					const uint32_t a = (face * k + j) * k + i;
					const uint32_t b = (face * k + j) * k + i + 1;
					const uint32_t c = (face * k + j + 1) * k + i;
					const uint32_t d = (face * k + j + 1) * k + i + 1;
					if (bottom ^ left) mesh_sphere.addQuadAlt(a, c, d, b);
					else mesh_sphere.addQuad(a, c, d, b);
				}
			}
		}
		return mesh_sphere.convert();
	}

	trimesh::TriMesh* createSphere(float radius, int num_iter)
	{
		//trimesh::TriMesh * rMesh  = Icosahedron(num_iter);
		trimesh::TriMesh* rMesh = SpherifiedCube(num_iter);

		for (size_t i = 0; i < rMesh->vertices.size(); i++)
		{
			float scale = radius / trimesh::length(rMesh->vertices[i]);
			rMesh->vertices[i].x *= scale;
			rMesh->vertices[i].y *= scale;
			rMesh->vertices[i].z *= scale;
		}

		return rMesh;
	}

	/// <summary>
	/// torus
	/// </summary>
	/// <param name="middler"></param>
	/// <param name="tuber"></param>
	/// <param name="sides"></param>
	/// <param name="slices"></param>
	/// <returns></returns>
	/// 
	trimesh::TriMesh* createTorusMesh(float middler, float tuber, int sides, int slices)
	{

		Mesh_sphere mesh;

		// generate vertices
		for (size_t i = 0; i < sides; i++) {
			for (size_t j = 0; j < sides; j++) {
				float u = (float)j / sides * M_PI * 2.0;
				float v = (float)i / slices * M_PI * 2.0;
				float x = (middler + tuber * std::cos(v)) * std::cos(u);
				float y = (middler + tuber * std::cos(v)) * std::sin(u);
				float z = tuber * std::sin(v);
				mesh.vertices.push_back(trimesh::point(x, y, z));
			}
		}

		// add quad faces
		for (size_t i = 0; i < slices; i++) {
			auto i_next = (i + 1) % slices;
			for (size_t j = 0; j < sides; j++) {
				auto j_next = (j + 1) % sides;
				auto i0 = i * sides + j;
				auto i1 = i * sides + j_next;
				auto i2 = i_next * sides + j_next;
				auto i3 = i_next * sides + j;
				mesh.addQuad((i0), (i1), (i2), (i3));
			}
		}

		return mesh.convert();
	}

	/// <summary>
	/// 
	/// </summary>
	/// <param name="radius"></param>
	/// <param name="height"></param>
	/// <param name="resolution"></param>
	/// <returns></returns>
	trimesh::TriMesh* createCylinder(float radius, float height, int resolution)
	{
		Mesh_sphere  mesh;
		// generate vertices
		std::vector<size_t> bottom_vertices;
		std::vector<size_t> top_vertices;
		trimesh::vec3 top(0.0f, 0.0f, height / 2.0f);
		trimesh::vec3 bottom(0.0f, 0.0f, -height / 2.0f);

		for (size_t i = 0; i < resolution; i++) {
			float ratio = (float)(i) / (resolution);
			float r = ratio * (M_PI * 2.0);
			float x = std::cos(r) * radius;
			float y = std::sin(r) * radius;
			trimesh::vec3 v = trimesh::vec3(x, y, -height / 2.0f);
			mesh.vertices.emplace_back(v);
			v = trimesh::point(x, y, height / 2.0f);
			mesh.vertices.emplace_back(v);
		}

		// add faces around the cylinder
		for (size_t i = 0; i < resolution; i++) {
			auto ii = i * 2;
			auto jj = (ii + 2) % (resolution * 2);
			auto kk = (ii + 3) % (resolution * 2);
			auto ll = ii + 1;
			mesh.addQuad((ii), (jj), (kk), (ll));
		}


		mesh.vertices.emplace_back(bottom);

		for (size_t i = 0; i < mesh.vertices.size() - 1; i += 2)
		{
			mesh.triangles.push_back(i);
			mesh.triangles.push_back(mesh.vertices.size() - 1);
			mesh.triangles.push_back(i + 2);
		}
		mesh.triangles.push_back((mesh.vertices.size()) - 3);
		mesh.triangles.push_back(mesh.vertices.size() - 1);
		mesh.triangles.push_back(0);

		mesh.vertices.emplace_back(top);
		for (size_t i = 1; i < mesh.vertices.size() - 1; i += 2)
		{
			mesh.triangles.push_back(i + 2);
			mesh.triangles.push_back(mesh.vertices.size() - 1);
			mesh.triangles.push_back(i);
		}
		mesh.triangles.push_back(mesh.vertices.size() - 1);
		mesh.triangles.push_back((mesh.vertices.size()) - 3);
		mesh.triangles.push_back(1);


		return mesh.convert();
	}

	/// <summary>
	/// 
	/// </summary>
	/// <param name="radiusu"></param>
	/// <param name="radiusb"></param>
	/// <param name="height"></param>
	/// <param name="resolution"></param>
	/// <returns></returns>
	trimesh::TriMesh* createCCylinder(float radiusu, float radiusb, float height, int resolution)
	{
		Mesh_sphere  mesh;
		// generate vertices
		std::vector<size_t> bottom_vertices;
		std::vector<size_t> top_vertices;
		trimesh::vec3 top(0.0f, 0.0f, height / 2.0f);
		trimesh::vec3 bottom(0.0f, 0.0f, -height / 2.0f);

		for (size_t i = 0; i < resolution; i++) {
			float ratio = (float)(i) / (resolution);
			float r = ratio * (M_PI * 2.0);
			float x = std::cos(r) * radiusu;
			float y = std::sin(r) * radiusu;
			trimesh::vec3 v = trimesh::vec3(x, y, -height / 2.0f);
			mesh.vertices.emplace_back(v);
			x = std::cos(r) * radiusb;
			y = std::sin(r) * radiusb;
			v = trimesh::point(x, y, height / 2.0f);
			mesh.vertices.emplace_back(v);
		}

		// add faces around the cylinder
		for (size_t i = 0; i < resolution; i++) {
			auto ii = i * 2;
			auto jj = (ii + 2) % (resolution * 2);
			auto kk = (ii + 3) % (resolution * 2);
			auto ll = ii + 1;
			mesh.addQuad((ii), (jj), (kk), (ll));
		}


		mesh.vertices.emplace_back(bottom);

		for (size_t i = 0; i < mesh.vertices.size() - 1; i += 2)
		{
			mesh.triangles.push_back(i);
			mesh.triangles.push_back(mesh.vertices.size() - 1);
			mesh.triangles.push_back(i + 2);
		}
		mesh.triangles.push_back((mesh.vertices.size()) - 3);
		mesh.triangles.push_back(mesh.vertices.size() - 1);
		mesh.triangles.push_back(0);

		mesh.vertices.emplace_back(top);
		for (size_t i = 1; i < mesh.vertices.size() - 1; i += 2)
		{
			mesh.triangles.push_back(i + 2);
			mesh.triangles.push_back(mesh.vertices.size() - 1);
			mesh.triangles.push_back(i);
		}
		mesh.triangles.push_back(mesh.vertices.size() - 1);
		mesh.triangles.push_back((mesh.vertices.size()) - 3);
		mesh.triangles.push_back(1);

		return mesh.convert();
	}

	/// <summary>
	/// 
	/// </summary>
	/// <param name="position"></param>
	/// <param name="normal"></param>
	/// <param name="depth"></param>
	/// <param name="radius"></param>
	/// <param name="num"></param>
	/// <param name="theta"></param>
	/// <returns></returns>
	trimesh::TriMesh* createCylinderMeshFromCenter(const trimesh::vec3& position, const trimesh::vec3& normal,
		float depth, float radius, int num, float theta)
	{
		trimesh::vec3 _normal = trimesh::normalized(normal);

		trimesh::vec3 bottom = position + 2.0f * _normal;
		trimesh::vec3 top = position - (depth + 2.0f) * _normal;

		return createCylinderMesh(top, bottom, radius, num);
	}

	trimesh::TriMesh* createCylinderMesh(const trimesh::vec3& top, const trimesh::vec3& bottom, float radius, int num, float theta)
	{
		trimesh::TriMesh* mesh = new trimesh::TriMesh();

		int hPart = num;

		trimesh::vec3 start = bottom;
		trimesh::vec3 end = top;

		trimesh::vec3 dir = end - start;
		//dir.normalize();
		trimesh::normalize(dir);
		trimesh::quaternion q = trimesh::quaternion::fromDirection(dir, trimesh::vec3(0.0f, 0.0f, 1.0f));

		theta *= M_PIf / 180.0f;
		float deltaTheta = M_PIf * 2.0f / (float)(hPart);
		std::vector<float> cosValue;
		std::vector<float> sinValue;
		for (int i = 0; i < hPart; ++i)
		{
			//cosValue.push_back(qCos(deltaTheta * (float)i + theta));
			//sinValue.push_back(qSin(deltaTheta * (float)i + theta));

			cosValue.push_back(std::cos(deltaTheta * (float)i + theta));
			sinValue.push_back(std::sin(deltaTheta * (float)i + theta));
		}

		std::vector<trimesh::vec3> baseNormals;
		for (int i = 0; i < hPart; ++i)
		{
			baseNormals.push_back(trimesh::vec3(cosValue[i], sinValue[i], 0.0f));
		}

		int vertexNum = 2 * hPart;
		std::vector<trimesh::vec3> points(vertexNum);
		int faceNum = 4 * hPart - 4;
		mesh->faces.resize(faceNum);

		int vertexIndex = 0;
		for (int i = 0; i < hPart; ++i)
		{
			trimesh::vec3 n = q * baseNormals[i];
			trimesh::vec3 s = start + n * radius;
			points.at(vertexIndex) = trimesh::vec3(s.x, s.y, s.z);
			++vertexIndex;
			trimesh::vec3 e = end + n * radius;
			points.at(vertexIndex) = trimesh::vec3(e.x, e.y, e.z);
			++vertexIndex;
		}
		mesh->vertices.swap(points);

		auto fvindex = [&hPart](int layer, int index)->int {
			return layer + 2 * index;
		};

		int faceIndex = 0;
		for (int i = 0; i < hPart; ++i)
		{
			int v1 = fvindex(0, i);
			int v2 = fvindex(1, i);
			int v3 = fvindex(0, (i + 1) % hPart);
			int v4 = fvindex(1, (i + 1) % hPart);

			trimesh::TriMesh::Face& f1 = mesh->faces.at(faceIndex);
			f1[0] = v1;
			f1[1] = v3;
			f1[2] = v2;
			++faceIndex;
			trimesh::TriMesh::Face& f2 = mesh->faces.at(faceIndex);
			f2[0] = v2;
			f2[1] = v3;
			f2[2] = v4;
			++faceIndex;
		}

		for (int i = 1; i < hPart - 1; ++i)
		{
			trimesh::TriMesh::Face& f1 = mesh->faces.at(faceIndex);
			f1[0] = 0;
			f1[1] = fvindex(0, i + 1);
			f1[2] = fvindex(0, i);
			++faceIndex;
		}

		for (int i = 1; i < hPart - 1; ++i)
		{
			trimesh::TriMesh::Face& f1 = mesh->faces.at(faceIndex);
			f1[0] = 1;
			f1[1] = fvindex(1, i);
			f1[2] = fvindex(1, i + 1);
			++faceIndex;
		}

		return mesh;
	}

	/// <summary>
	/// 
	/// </summary>
	/// <param name="data"></param>
	/// <param name="radius1"></param>
	/// <param name="center1"></param>
	/// <param name="radius2"></param>
	/// <param name="center2"></param>
	/// <param name="num"></param>
	/// <param name="theta"></param>
	void fillCylinderSoup(trimesh::vec3* data, float radius1, const trimesh::vec3& center1,
		float radius2, const trimesh::vec3& center2, int num, float theta)
	{
		int hPart = num;

		trimesh::vec3 dir = center2 - center1;
		trimesh::normalize(dir);
		trimesh::quaternion q = trimesh::quaternion::fromDirection(dir, trimesh::vec3(0.0f, 0.0f, 1.0f));

		theta *= M_PIf / 180.0f;
		float deltaTheta = M_PIf * 2.0f / (float)(hPart);
		std::vector<float> cosValue;
		std::vector<float> sinValue;
		for (int i = 0; i < hPart; ++i)
		{
			cosValue.push_back(cosf(deltaTheta * (float)i + theta));
			sinValue.push_back(sinf(deltaTheta * (float)i + theta));
		}

		std::vector<trimesh::vec3> baseNormals;
		for (int i = 0; i < hPart; ++i)
		{
			baseNormals.push_back(trimesh::vec3(cosValue[i], sinValue[i], 0.0f));
		}

		int vertexNum = 2 * hPart;
		std::vector<trimesh::vec3> points(vertexNum);
		int faceNum = 4 * hPart - 4;

		int vertexIndex = 0;
		for (int i = 0; i < hPart; ++i)
		{
			trimesh::vec3 n = q * baseNormals[i];
			trimesh::vec3 s = center1 + n * radius1;
			points.at(vertexIndex) = trimesh::vec3(s.x, s.y, s.z);
			++vertexIndex;
			trimesh::vec3 e = center2 + n * radius2;
			points.at(vertexIndex) = trimesh::vec3(e.x, e.y, e.z);
			++vertexIndex;
		}

		auto fvindex = [&hPart](int layer, int index)->int {
			return layer + 2 * index;
		};

		for (int i = 0; i < hPart; ++i)
		{
			int v1 = fvindex(0, i);
			int v2 = fvindex(1, i);
			int v3 = fvindex(0, (i + 1) % hPart);
			int v4 = fvindex(1, (i + 1) % hPart);
			*data++ = points.at(v1);
			*data++ = points.at(v3);
			*data++ = points.at(v2);
			*data++ = points.at(v2);
			*data++ = points.at(v3);
			*data++ = points.at(v4);
		}

		for (int i = 1; i < hPart - 1; ++i)
		{
			*data++ = points.at(0);
			*data++ = points.at(fvindex(0, i + 1));
			*data++ = points.at(fvindex(0, i));
		}

		for (int i = 1; i < hPart - 1; ++i)
		{
			*data++ = points.at(1);
			*data++ = points.at(fvindex(1, i));
			*data++ = points.at(fvindex(1, i + 1));
		}
	}

	/// <summary>
	/// 
	/// </summary>
	/// <param name="x"></param>
	/// <param name="y"></param>
	/// <param name="z"></param>
	/// <returns></returns>
	trimesh::TriMesh* createCuboid(float x, float y, float z)
	{
		trimesh::TriMesh* boxMesh = new trimesh::TriMesh();
		trimesh::vec v0(-x / 2., -y / 2., -z / 2.);
		trimesh::vec v1(x / 2., y / 2., z / 2.);
		trimesh::box3 bb(v0, v1);
		trimesh::point point0(bb.min[0], bb.min[1], bb.min[2]);
		trimesh::point point1(bb.max[0], bb.min[1], bb.min[2]);
		trimesh::point point2(bb.max[0], bb.min[1], bb.max[2]);
		trimesh::point point3(bb.min[0], bb.min[1], bb.max[2]);
		trimesh::point point4(bb.min[0], bb.max[1], bb.min[2]);
		trimesh::point point5(bb.max[0], bb.max[1], bb.min[2]);
		trimesh::point point6(bb.max[0], bb.max[1], bb.max[2]);
		trimesh::point point7(bb.min[0], bb.max[1], bb.max[2]);
		boxMesh->vertices.push_back(point0);
		boxMesh->vertices.push_back(point1);
		boxMesh->vertices.push_back(point2);
		boxMesh->vertices.push_back(point3);
		boxMesh->vertices.push_back(point4);
		boxMesh->vertices.push_back(point5);
		boxMesh->vertices.push_back(point6);
		boxMesh->vertices.push_back(point7);

		//正面add face 1
		boxMesh->faces.push_back(trimesh::TriMesh::Face(0, 1, 2));
		//正面 add face 2
		boxMesh->faces.push_back(trimesh::TriMesh::Face(2, 3, 0));

		//右侧面add face 1
		boxMesh->faces.push_back(trimesh::TriMesh::Face(1, 5, 6));
		//右侧面 add face 2
		boxMesh->faces.push_back(trimesh::TriMesh::Face(6, 2, 1));

		//背面add face 1
		boxMesh->faces.push_back(trimesh::TriMesh::Face(4, 6, 5));
		//背面 add face 2
		boxMesh->faces.push_back(trimesh::TriMesh::Face(6, 4, 7));

		//左侧面add face 1
		boxMesh->faces.push_back(trimesh::TriMesh::Face(4, 0, 3));
		//左侧面 add face 2
		boxMesh->faces.push_back(trimesh::TriMesh::Face(3, 7, 4));

		//顶面add face 1
		boxMesh->faces.push_back(trimesh::TriMesh::Face(3, 2, 6));
		//顶面 add face 2
		boxMesh->faces.push_back(trimesh::TriMesh::Face(6, 7, 3));

		//底面add face 1
		boxMesh->faces.push_back(trimesh::TriMesh::Face(5, 1, 0));
		//底面 add face 2
		boxMesh->faces.push_back(trimesh::TriMesh::Face(5, 0, 4));

		return boxMesh;
	}

	/// <summary>
	/// 
	/// </summary>
	/// <param name="resolution"></param>
	/// <param name="radius"></param>
	/// <param name="height"></param>
	/// <returns></returns>
	trimesh::TriMesh* createCone(size_t resolution, float radius, float height)
	{

		Mesh_sphere  mesh;
		//generate vertices
		//add vertices subdividing a circle
	   //std::vector<Vertex> base_vertices;

	   // add the tip of the cone
		auto v0 = trimesh::point(0.0, 0.0, height / 2.0);
		auto vb = trimesh::point(0.0, 0.0, -height / 2.0);
		mesh.vertices.push_back(v0);
		for (size_t i = 0; i < resolution; i++) {
			float ratio = static_cast<float>(i) / (resolution);
			float r = ratio * (M_PI * 2.0);
			float x = std::cos(r) * radius;
			float y = std::sin(r) * radius;
			auto v = trimesh::point(x, y, -height / 2.0);
			mesh.vertices.push_back(v);
		}

		// generate triangular faces
		for (size_t i = 1; i < resolution; i++)
		{
			mesh.triangles.push_back(i);
			mesh.triangles.push_back(i + 1);
			mesh.triangles.push_back(0);
		}
		mesh.triangles.push_back(1);
		mesh.triangles.push_back(0);
		mesh.triangles.push_back(resolution);


		mesh.vertices.emplace_back(vb);

		for (size_t i = 1; i < resolution + 1; i++)
		{
			mesh.triangles.push_back(i + 1);
			mesh.triangles.push_back(i);
			mesh.triangles.push_back(resolution + 1);
		}
		mesh.triangles.push_back(resolution);
		mesh.triangles.push_back(resolution + 1);
		mesh.triangles.push_back(1);

		return mesh.convert();
	}
}