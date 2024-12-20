#include "qhullWrapper/hull/hullface.h"
#include "qhullWrapper/hull/meshconvex.h"
#include "trimesh2/TriMesh.h"
#include "trimesh2/XForm.h"
#include "trimesh2/quaternion.h"

#include "dumplicate.h"
#include <numeric>
#include <queue>

namespace qhullWrapper
{
	class MeshVertex
	{
	public:
		trimesh::point p;
		std::vector<uint32_t> connected_faces;

		MeshVertex(trimesh::point p) : p(p) { connected_faces.reserve(8); }
	};

	class MeshFace
	{
	public:
		int vertex_index[3] = { -1 };
		int connected_face_index[3];
		bool bUnique = true;
		bool bAdd = false;

		int index = 0;  //just for sort
	};

	class MeshTest
	{
	public:
		MeshTest() {};
		~MeshTest() {};
		std::vector<MeshVertex> vertices;
		std::vector<MeshFace> faces;
	};

	class FPoint3
	{
	public:
		float x, y, z;
		FPoint3() {}
		FPoint3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
		FPoint3(const trimesh::point& p) : x(p.x * .001), y(p.y * .001), z(p.z * .001) {}
		float vSize2() const
		{
			return x * x + y * y + z * z;
		}

		float vSize() const
		{
			return sqrt(vSize2());
		}
		bool operator==(FPoint3& p) const { return x == p.x && y == p.y && z == p.z; }
		FPoint3 operator/(const float f) const { return FPoint3(x / f, y / f, z / f); }
		FPoint3 operator*(const float f) const { return FPoint3(x * f, y * f, z * f); }
		FPoint3& operator *= (const float f) { x *= f; y *= f; z *= f; return *this; }
		FPoint3 cross(const FPoint3& p) const
		{
			return FPoint3(
				y * p.z - z * p.y,
				z * p.x - x * p.z,
				x * p.y - y * p.x);
		}
		double operator*(FPoint3 lhs)
		{
			return lhs.x * x + lhs.y * y + lhs.z * z;
		}
	};

	int getFaceIdxWithPoints(MeshTest& ameshtest, int idx0, int idx1, int notFaceIdx, int notFaceVertexIdx)
	{
		std::vector<int> candidateFaces;
		for (int f : ameshtest.vertices[idx0].connected_faces)
		{
			if (f == notFaceIdx)
			{
				continue;
			}
			if (ameshtest.faces[f].vertex_index[0] == idx1
				|| ameshtest.faces[f].vertex_index[1] == idx1
				|| ameshtest.faces[f].vertex_index[2] == idx1
				)  candidateFaces.push_back(f);

		}

		if (candidateFaces.size() == 0)
		{
			// has_disconnected_faces = true;
			return -1;
		}
		if (candidateFaces.size() == 1) { return candidateFaces[0]; }


		if (candidateFaces.size() % 2 == 0)
		{
			//cura::logDebug("Warning! Edge with uneven number of faces connecting it!(%i)\n", candidateFaces.size() + 1);
			//has_disconnected_faces = true;
		}

		FPoint3 vn = ameshtest.vertices[idx1].p - ameshtest.vertices[idx0].p;
		FPoint3 n = vn / vn.vSize(); // the normal of the plane in which all normals of faces connected to the edge lie => the normalized normal
		FPoint3 v0 = ameshtest.vertices[idx1].p - ameshtest.vertices[idx0].p;

		// the normals below are abnormally directed! : these normals all point counterclockwise (viewed from idx1 to idx0) from the face, irrespective of the direction of the face.
		FPoint3 n0 = FPoint3(ameshtest.vertices[notFaceVertexIdx].p - ameshtest.vertices[idx0].p).cross(v0);

		if (n0.vSize() <= 0)
		{
			//cura::logDebug("Face %i has zero area!", notFaceIdx);
		}

		double smallestAngle = 1000; // more then 2 PI (impossible angle)
		int bestIdx = -1;

		for (int candidateFace : candidateFaces)
		{
			int candidateVertex;
			{// find third vertex belonging to the face (besides idx0 and idx1)
				for (candidateVertex = 0; candidateVertex < 3; candidateVertex++)
					if (ameshtest.faces[candidateFace].vertex_index[candidateVertex] != idx0 && ameshtest.faces[candidateFace].vertex_index[candidateVertex] != idx1)
						break;
			}

			FPoint3 v1 = ameshtest.vertices[ameshtest.faces[candidateFace].vertex_index[candidateVertex]].p - ameshtest.vertices[idx0].p;
			FPoint3 n1 = v0.cross(v1);

			double dot = n0 * n1;
			double det = n * n0.cross(n1);
			double angle = std::atan2(det, dot);
#ifndef M_PI
#define M_PI 3.1415926
#endif
			if (angle < 0) angle += 2 * M_PI; // 0 <= angle < 2* M_PI

			if (angle == 0)
			{
				//cura::logDebug("Overlapping faces: face %i and face %i.\n", notFaceIdx, candidateFace);
				//has_overlapping_faces = true;
			}
			if (angle < smallestAngle)
			{
				smallestAngle = angle;
				bestIdx = candidateFace;
			}
		}
		if (bestIdx < 0)
		{
			//cura::logDebug("Couldn't find face connected to face %i.\n", notFaceIdx);
			//has_disconnected_faces = true;
		}
		return bestIdx;
	};

	void trimesh2meshtest(trimesh::TriMesh* currentMesh, MeshTest& ameshTest)
	{
		for (trimesh::point apoint : currentMesh->vertices)
		{
			ameshTest.vertices.push_back(apoint);
		}

		for (unsigned int i = 0; i < currentMesh->faces.size(); i++)
		{
			trimesh::TriMesh::Face& face = currentMesh->faces[i];
			ameshTest.faces.emplace_back();
			ameshTest.faces[i].vertex_index[0] = face.at(0);
			ameshTest.faces[i].vertex_index[1] = face.at(1);
			ameshTest.faces[i].vertex_index[2] = face.at(2);

			ameshTest.vertices[face.at(0)].connected_faces.push_back(i);
			ameshTest.vertices[face.at(1)].connected_faces.push_back(i);
			ameshTest.vertices[face.at(2)].connected_faces.push_back(i);
		}

		for (unsigned int i = 0; i < currentMesh->faces.size(); i++)
		{
			trimesh::TriMesh::Face& face = currentMesh->faces[i];
			// faces are connected via the outside
			ameshTest.faces[i].connected_face_index[0] = getFaceIdxWithPoints(ameshTest, face.at(0), face.at(1), i, face.at(2));
			ameshTest.faces[i].connected_face_index[1] = getFaceIdxWithPoints(ameshTest, face.at(1), face.at(2), i, face.at(0));
			ameshTest.faces[i].connected_face_index[2] = getFaceIdxWithPoints(ameshTest, face.at(2), face.at(0), i, face.at(1));
		}
	};

	trimesh::fxform fromQuaterianEX(const trimesh::quaternion& q)
	{
		float x2 = q.xp * q.xp;
		float y2 = q.yp * q.yp;
		float z2 = q.zp * q.zp;
		float xy = q.xp * q.yp;
		float xz = q.xp * q.zp;
		float yz = q.yp * q.zp;
		float wx = q.wp * q.xp;
		float wy = q.wp * q.yp;
		float wz = q.wp * q.zp;


		// This calculation would be a lot more complicated for non-unit length quaternions
		// Note: The constructor of Matrix4 expects the Matrix in column-major format like expected by
		//   OpenGL
		return trimesh::fxform(1.0f - 2.0f * (y2 + z2), 2.0f * (xy - wz), 2.0f * (xz + wy), 0.0f,
			2.0f * (xy + wz), 1.0f - 2.0f * (x2 + z2), 2.0f * (yz - wx), 0.0f,
			2.0f * (xz - wy), 2.0f * (yz + wx), 1.0f - 2.0f * (x2 + y2), 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
	}

	double det(trimesh::point& p0, trimesh::point& p1, trimesh::point& p2)
	{
		trimesh::vec3 a = p0 - p1;
		trimesh::vec3 b = p1 - p2;
		return sqrt(pow((a.y * b.z - a.z * b.y), 2) + pow((a.z * b.x - a.x * b.z), 2)
			+ pow((a.x * b.y - a.y * b.x), 2)) / 2.0f;
	}

	double  getArea(std::vector<trimesh::point>& inVertices)
	{
		double sum = 0.0f;
		for (int n = 1; n < inVertices.size() - 1; n++)
		{
			sum += det(inVertices[0], inVertices[n], inVertices[n + 1]);
		}
		return abs(sum);
	}
	trimesh::point normalized(trimesh::point& apoint)
	{
		float  divisor = sqrt(apoint.x * apoint.x + apoint.y * apoint.y + apoint.z * apoint.z);
		return apoint / divisor;
	};

	std::vector<trimesh::TriMesh*> hullFacesFromMesh(trimesh::TriMesh* mesh)
	{
		std::vector<trimesh::TriMesh*> meshes;
		if (mesh)
		{
			trimesh::TriMesh* convex = qhullWrapper::convex_hull_3d(mesh);
            qhullWrapper::dumplicateMesh(convex);
			meshes = hullFacesFromConvexMesh(convex);
		}
		return meshes;
	}

	void getNormalFace(MeshTest& meshT, trimesh::point& Nnormal,int n, trimesh::TriMesh* aface)
	{
		for (int k = 0; k < 3; k++)
		{
			int faceIndex = meshT.faces[n].connected_face_index[k];
			if (meshT.faces[faceIndex].bAdd)
			{
				continue;//�ų��Ѿ���ӹ���face
			}
			int index0 = meshT.faces[faceIndex].vertex_index[0];
			int index1 = meshT.faces[faceIndex].vertex_index[1];
			int index2 = meshT.faces[faceIndex].vertex_index[2];
			trimesh::point thisNormal = trimesh::normalized(trimesh::trinorm(meshT.vertices[index0].p, meshT.vertices[index1].p, meshT.vertices[index2].p));
			if (std::abs(Nnormal.at(0) - thisNormal.at(0)) < 0.01
				&& std::abs(Nnormal.at(1) - thisNormal.at(1)) < 0.01
				&& std::abs(Nnormal.at(2) - thisNormal.at(2)) < 0.01)
			{
				aface->vertices.push_back(meshT.vertices[index0].p);
				aface->vertices.push_back(meshT.vertices[index1].p);
				aface->vertices.push_back(meshT.vertices[index2].p);
				meshT.faces[faceIndex].bAdd = true;
				getNormalFace(meshT, Nnormal, faceIndex, aface);
			}
		}
	}

	std::vector<trimesh::TriMesh*> hullFacesFromConvexMesh(trimesh::TriMesh* mesh)
	{
		std::vector<trimesh::TriMesh*> meshes;

		if (!mesh)
			return meshes;

		MeshTest meshT;
		trimesh2meshtest(mesh, meshT);

		//�������meshT ��������
		int faceNum = (int)meshT.faces.size();
		if (faceNum <= 0)
			return meshes;

		//zenggui
		std::vector<float> areas(faceNum, 0.0f);
		std::vector<trimesh::vec3> normals(faceNum);
		for (int i = 0; i < faceNum; ++i)
		{
			MeshFace& currentMeshFace = meshT.faces[i];
			currentMeshFace.index = i;
			int currentVertIndex0 = currentMeshFace.vertex_index[0];
			int currentVertIndex1 = currentMeshFace.vertex_index[1];
			int currentVertIndex2 = currentMeshFace.vertex_index[2];
			areas.at(i) = det(meshT.vertices[currentVertIndex0].p, meshT.vertices[currentVertIndex1].p, meshT.vertices[currentVertIndex2].p);

			normals.at(i) = trimesh::normalized(trimesh::trinorm(meshT.vertices[currentVertIndex0].p,
				meshT.vertices[currentVertIndex1].p, meshT.vertices[currentVertIndex2].p));
		}

		std::sort(meshT.faces.begin(), meshT.faces.end(), [&meshT, areas](const MeshFace& face1, const MeshFace& face2)->bool {
			return areas[face1.index] > areas[face2.index];
			});

		for (int n = 0; n < faceNum; n++)
		{
			MeshFace& faceN = meshT.faces[n];
			if (faceN.bUnique == false)
			{
				continue;
			}
		
			trimesh::TriMesh* aface = new trimesh::TriMesh();
			trimesh::point Nnormal = normals.at(faceN.index); 
			for (int m = n + 1; m < faceNum; m++)
			{
				MeshFace& faceM = meshT.faces[m];
				trimesh::point Mnormal = normals.at(faceM.index);
				if (std::abs(Nnormal.at(0) - Mnormal.at(0)) < 0.005
					&& std::abs(Nnormal.at(1) - Mnormal.at(1)) < 0.005
					&& std::abs(Nnormal.at(2) - Mnormal.at(2)) < 0.005)
				{
					aface->vertices.push_back(meshT.vertices[faceM.vertex_index[0]].p);
					aface->vertices.push_back(meshT.vertices[faceM.vertex_index[1]].p);
					aface->vertices.push_back(meshT.vertices[faceM.vertex_index[2]].p);
					faceM.bUnique = false;
					faceM.bAdd = true;
				}
			}
		
			if (faceN.bUnique)
			{
				aface->normals.push_back(Nnormal);
				aface->vertices.push_back(meshT.vertices[faceN.vertex_index[0]].p);
				aface->vertices.push_back(meshT.vertices[faceN.vertex_index[1]].p);
				aface->vertices.push_back(meshT.vertices[faceN.vertex_index[2]].p);
				faceN.bAdd = true;
				getNormalFace(meshT, Nnormal, n, aface); //�ݹ� �ϲ� �����ߵ�������
				meshes.push_back(aface);
			}
			else
			{
				delete aface;
			}
			if (meshes.size() > 500)
			{
				break;
			}
		}
		//wang wenbin
		//for (int n = 1; n < meshT.faces.size(); n++)
		//{
		//	int preIndex = n - 1;
		//	MeshFace currentMeshFace = meshT.faces[n];
		//	int currentVertIndex0 = meshT.faces[n].vertex_index[0];
		//	int currentVertIndex1 = meshT.faces[n].vertex_index[1];
		//	int currentVertIndex2 = meshT.faces[n].vertex_index[2];
		//	int currentArea = det(meshT.vertices[currentVertIndex0].p, meshT.vertices[currentVertIndex1].p, meshT.vertices[currentVertIndex2].p);
		//	while (preIndex >= 0 && det(meshT.vertices[meshT.faces[preIndex].vertex_index[0]].p,
		//		meshT.vertices[meshT.faces[preIndex].vertex_index[1]].p,
		//		meshT.vertices[meshT.faces[preIndex].vertex_index[2]].p) <
		//		currentArea)
		//	{
		//		meshT.faces[preIndex + 1] = meshT.faces[preIndex];
		//		preIndex--;
		//	}
		//	meshT.faces[preIndex + 1] = currentMeshFace;
		//}

		//��ȡ�������Ҳ������ߵ�500�������棬���ϲ����乲���ߵ���
		//for (int n = 0; n < faceNum; n++)
		//{
		//	if (meshT.faces[n].bUnique==false)
		//	{
		//		continue;
		//	}
		//	trimesh::TriMesh* aface = new trimesh::TriMesh();
		//	int index0 = meshT.faces[n].vertex_index[0];
		//	int index1 = meshT.faces[n].vertex_index[1];
		//	int index2 = meshT.faces[n].vertex_index[2];
		//	trimesh::point Nnormal = trimesh::normalized(trimesh::trinorm(meshT.vertices[index0].p, meshT.vertices[index1].p, meshT.vertices[index2].p));
		//	for (int m = n + 1; m < faceNum; m++)
		//	{
		//		int index0 = meshT.faces[m].vertex_index[0];
		//		int index1 = meshT.faces[m].vertex_index[1];
		//		int index2 = meshT.faces[m].vertex_index[2];
		//		trimesh::point Mnormal = trimesh::normalized(trimesh::trinorm(meshT.vertices[index0].p, meshT.vertices[index1].p, meshT.vertices[index2].p));
		//		if (std::abs(Nnormal.at(0) - Mnormal.at(0)) < 0.005
		//			&& std::abs(Nnormal.at(1) - Mnormal.at(1)) < 0.005
		//			&& std::abs(Nnormal.at(2) - Mnormal.at(2)) < 0.005)
		//		{
		//			aface->vertices.push_back(meshT.vertices[meshT.faces[m].vertex_index[0]].p);
		//			aface->vertices.push_back(meshT.vertices[meshT.faces[m].vertex_index[1]].p);
		//			aface->vertices.push_back(meshT.vertices[meshT.faces[m].vertex_index[2]].p);
		//			meshT.faces[m].bUnique = false;
		//			meshT.faces[m].bAdd = true;
		//		}
		//	}
		//
		//	if (meshT.faces[n].bUnique)
		//	{
		//		aface->normals.push_back(Nnormal);
		//		aface->vertices.push_back(meshT.vertices[meshT.faces[n].vertex_index[0]].p);
		//		aface->vertices.push_back(meshT.vertices[meshT.faces[n].vertex_index[1]].p);
		//		aface->vertices.push_back(meshT.vertices[meshT.faces[n].vertex_index[2]].p);
		//		meshT.faces[n].bAdd = true;
		//		getNormalFace(meshT, Nnormal, n, aface); //�ݹ� �ϲ� �����ߵ�������
		//		meshes.push_back(aface);
		//	}
		//	if (meshes.size() > 500)
		//	{
		//		break;
		//	}
		//}

		//�������polygon ��������ֻ�����������20��polygon:
		std::sort(meshes.rbegin(), meshes.rend(), [](trimesh::TriMesh* a, trimesh::TriMesh* b)
			{
				return getArea(a->vertices) < getArea(b->vertices);
			});
		meshes.resize(std::min((int)meshes.size(), 20));

		//��ʼ�������еĶ���Σ�����ת����xyƽ��:
		for (unsigned int polygon_id = 0; polygon_id < meshes.size(); ++polygon_id)
		{
			trimesh::TriMesh*& currentMesh = meshes[polygon_id];
			trimesh::vec3 normal = currentMesh->normals[0];

			//������΢̧�𣬷�ֹ��ģ�͵����ص�
			for (trimesh::point& apoint : currentMesh->vertices)
			{
				apoint += normal * 0.1;
			}

			//��z��y��ת��ʹƽ���ƽ
			const trimesh::vec3 XYnormal(0.0f, 0.0f, 1.0f);
			trimesh::quaternion q = q.rotationTo(normal, XYnormal);
			trimesh::fxform xf = fromQuaterian(q);
			for (trimesh::point& apoint : currentMesh->vertices)
			{
				apoint = (xf * apoint);
			}
			currentMesh = qhullWrapper::convex_hull_2d(currentMesh);
			currentMesh->normals.push_back(normal);
			std::vector<trimesh::point>& polygon = currentMesh->vertices;

			//������ε��ڽǣ���������С��������ֵ�Ķ����
			static constexpr double PI = 3.141592653589793238;
			bool discard = false;
			const double angle_threshold = ::cos(10.0 * (double)PI / 180.0);
			for (unsigned int i = 0; i < polygon.size(); ++i)
			{
				const trimesh::point& prec = polygon[(i == 0) ? polygon.size() - 1 : i - 1];
				const trimesh::point& curr = polygon[i];
				const trimesh::point& next = polygon[(i == polygon.size() - 1) ? 0 : i + 1];
				if (normalized(prec - curr).dot(normalized(next - curr)) > angle_threshold)
				{
					discard = true;
					break;
				}
			}
			if (discard)
			{
				meshes.erase(meshes.begin() + (polygon_id--));
				continue;
			}

			//�����������һ�㣬��ֹ����ģ�͵ı�Ե:
			trimesh::point centroid = std::accumulate(polygon.begin(), polygon.end(), trimesh::point(0.0, 0.0, 0.0));
			centroid /= (double)polygon.size();
			for (auto& vertex : polygon)
			{
				vertex = 0.9f * vertex + 0.1f * centroid;
			}

			//����������Ǽ򵥺�͹�ģ����ǽ�ʹ��Ǳ�Բ����������������
			//���㷨ȡһ�����㣬������Աߵ��м�ֵ��Ȼ���ƶ�����
			//�ӽ�ƽ��ˮƽ(�ɡ�aggressivity������)���ظ�k�Ρ�
			const unsigned int k = 10; // number of iterations
			const float aggressivity = 0.2f;  // agressivity
			const unsigned int N = polygon.size();
			std::vector<std::pair<unsigned int, unsigned int>> neighbours;
			if (k != 0)
			{
				std::vector<trimesh::point> points_out(2 * k * N); // vector long enough to store the future vertices
				for (unsigned int j = 0; j < N; ++j) {
					points_out[j * 2 * k] = polygon[j];
					neighbours.push_back(std::make_pair((int)(j * 2 * k - k) < 0 ? (N - 1) * 2 * k + k : j * 2 * k - k, j * 2 * k + k));
				}

				for (unsigned int i = 0; i < k; ++i) {
					// Calculate middle of each edge so that neighbours points to something useful:
					for (unsigned int j = 0; j < N; ++j)
						if (i == 0)
							points_out[j * 2 * k + k] = 0.5f * (points_out[j * 2 * k] + points_out[j == N - 1 ? 0 : (j + 1) * 2 * k]);
						else {
							float r = 0.2 + 0.3 / (k - 1) * i; // the neighbours are not always taken in the middle
							points_out[neighbours[j].first] = r * points_out[j * 2 * k] + (1 - r) * points_out[neighbours[j].first - 1];
							points_out[neighbours[j].second] = r * points_out[j * 2 * k] + (1 - r) * points_out[neighbours[j].second + 1];
						}
					// Now we have a triangle and valid neighbours, we can do an iteration:
					for (unsigned int j = 0; j < N; ++j)
						points_out[2 * k * j] = (1 - aggressivity) * points_out[2 * k * j] +
						aggressivity * 0.5f * (points_out[neighbours[j].first] + points_out[neighbours[j].second]);

					for (auto& n : neighbours) {
						++n.first;
						--n.second;
					}
				}
				polygon = points_out; // replace the coarse polygon with the smooth one that we just created
			}
			//ͨ�������ת������ά����
			for (trimesh::point& apoint : meshes[polygon_id]->vertices)
			{
				apoint = trimesh::inv(xf) * apoint;
			}
		}

		//���ǻ�
		for (trimesh::TriMesh* amesh : meshes)
		{
			std::vector<trimesh::point> apoints = amesh->vertices;
			amesh->vertices.clear();
			if (apoints.size() < 3)
			{
				continue;
			}
			//��������
			trimesh::vec3 originNormal = amesh->normals[0];
			trimesh::vec3 newNormal = trimesh::normalized(trimesh::trinorm(apoints[0], apoints[1], apoints[2]));
			bool isReverse = false;
			if ((originNormal DOT newNormal)<0)
			{
				isReverse = true;
			}

			for (int n = 2; n < apoints.size(); n++)
			{
				int index = amesh->vertices.size();
				amesh->vertices.push_back(apoints[0]);
				amesh->vertices.push_back(apoints[n - 1]);
				amesh->vertices.push_back(apoints[n]);
				if (isReverse)
				{
					amesh->faces.push_back(trimesh::TriMesh::Face(index+2, index + 1, index));
				}
				else
				{
					amesh->faces.push_back(trimesh::TriMesh::Face(index, index + 1, index + 2));
				}
			}
		}
		return meshes;
	}

	void FaceGenerateMesh(const trimesh::TriMesh* inputMesh, std::vector<int>& inputface, trimesh::TriMesh& newMesh)
	{
		const auto& faces = inputMesh->faces;
		const auto& points = inputMesh->vertices;
		const size_t nums = inputface.size();
		newMesh.vertices.reserve(3 * nums);
		newMesh.faces.reserve(nums);
		for (int i = 0; i < nums; ++i) {
			const auto& f = faces[inputface[i]];
			newMesh.vertices.emplace_back(points[f[0]]);
			newMesh.vertices.emplace_back(points[f[1]]);
			newMesh.vertices.emplace_back(points[f[2]]);
			newMesh.faces.emplace_back(trimesh::vec3(3 * i, 3 * i + 1, 3 * i + 2));
		}
		qhullWrapper::dumplicateMesh(&newMesh);
	}

    void hullFacesFromConvexMesh(trimesh::TriMesh* convexMesh, std::vector<HullFace>& hullFaces, float thresholdNormal)
    {
        qhullWrapper::dumplicateMesh(convexMesh);
        //convexMesh->write("test/convex.stl");
        const auto& faces = convexMesh->faces;
        const auto& vertexs = convexMesh->vertices;
        const int facenums = faces.size();
        std::vector<trimesh::point> normals;
        normals.reserve(facenums);
        std::vector<float> areas;
        areas.reserve(facenums);
        std::vector<trimesh::point> areanormals;
        areanormals.reserve(facenums);
        for (const auto& f : faces) {
            const auto& a = vertexs[f[0]];
            const auto& b = vertexs[f[1]];
            const auto& c = vertexs[f[2]];
            const auto& n = 0.5 * (b - a)TRICROSS(c - a);
            const auto & area = len(n);
            areas.emplace_back(area);
            areanormals.emplace_back(n);
            normals.emplace_back(n / area);
        }
        convexMesh->need_across_edge();
        auto neighbors = convexMesh->across_edge;
        std::vector<bool> masks(facenums, true);
        for (int f = 0; f < facenums; ++f) {
            if (masks[f]) {
                float sumArea = 0.f;
                trimesh::vec3 sumNormal;
                const auto& nf = normals[f];
                std::vector<int>currentFaces;
                std::queue<int>currentQueue;
                currentQueue.emplace(f);
                currentFaces.emplace_back(f);
                sumNormal += areanormals[f];
                sumArea += areas[f];
                masks[f] = false;
                while (!currentQueue.empty()) {
                    auto fr = currentQueue.front();
                    currentQueue.pop();
                    const auto& neighbor = neighbors[fr];
                    for (const auto& fa : neighbor) {
                        if (fa < 0) continue;
                        if (masks[fa]) {
                            const auto& na = normals[fa];
                            const auto& nr = normals[fr];
                            //const auto& dir = sumNormal / sumArea;
                            //if ((nr DOT dir) >= thresholdNormal && (dir DOT na) >= thresholdNormal)
                            if (std::abs(nr.at(0) - na.at(0)) < 0.001
                                && std::abs(nr.at(1) - na.at(1)) < 0.001
                                && std::abs(nr.at(2) - na.at(2)) < 0.001)
                            {
                                currentQueue.emplace(fa);
                                currentFaces.emplace_back(fa);
                                sumNormal += areanormals[fa];
                                sumArea += areas[fa];
                                masks[fa] = false;
                            }
                        }
                    }
                }
                for (int i = 0; i < currentFaces.size();) {
                    const auto& f = currentFaces[i];
                    const auto& n = normals[f];
                    if ((n DOT(sumNormal / sumArea)) < thresholdNormal) {
                        sumNormal -= areanormals[f];
                        sumArea -= areas[f];
                        currentFaces.erase(currentFaces.begin() + i);
                    } else {
                        ++i;
                    }
                }
                HullFace regionMesh;
                trimesh::TriMesh* mesh = regionMesh.mesh.get();
                const size_t num = currentFaces.size();
                mesh->faces.reserve(num);
                mesh->vertices.reserve(3 * num);
                trimesh::point normal;
                for (size_t i = 0; i < num; ++i) {
                    const auto& f = currentFaces[i];
                    regionMesh.hullarea += areas[f];
                    normal += areanormals[f];
                    for (int j = 0; j < 3; ++j) {
                        mesh->vertices.emplace_back(vertexs[faces[f][j]]);
                    }
                    mesh->faces.emplace_back(std::move(trimesh::TriMesh::Face(3 * i, 3 * i + 1, 3 * i + 2)));
                }
                if (num > 1) qhullWrapper::dumplicateMesh(mesh);
                regionMesh.normal = trimesh::normalized(normal);
                hullFaces.emplace_back(std::move(regionMesh));
            }
        }
        std::sort(hullFaces.begin(), hullFaces.end(), [&](HullFace & hulla, HullFace & hullb) {
            return hulla.hullarea > hullb.hullarea;
        });
        hullFaces.resize(std::min(50, (int)hullFaces.size()));
        auto mergeMesh = [&](trimesh::TriMesh * mesh, const trimesh::TriMesh * other) {
            const auto& afaces = other->faces;
            const auto& apoints = other->vertices;
            const int pn = mesh->vertices.size();
            const int fn = mesh->faces.size();
            mesh->faces.reserve(fn + afaces.size());
            mesh->vertices.insert(mesh->vertices.end(), apoints.begin(), apoints.end());
            for (const auto& f : afaces) {
                mesh->faces.emplace_back(trimesh::ivec3(pn + f[0], pn + f[1], pn + f[2]));
            }
        };
        for (int i = 0; i < hullFaces.size(); ++i) {
            const auto& hulla = hullFaces[i];
            for (int j = i + 1; j < hullFaces.size();) {
                const auto& hullb = hullFaces[j];
                if ((hulla.normal DOT hullb.normal) >= thresholdNormal) {
                    hulla.mesh->need_bbox(), hullb.mesh->need_bbox();
                    const auto& boxa = hulla.mesh->bbox;
                    const auto& boxb = hullb.mesh->bbox;
                    const float dist = trimesh::len(boxa.center() - boxb.center());
                    if (dist < boxa.radius() && dist < boxb.radius()) {
                        mergeMesh(hulla.mesh.get(), hullb.mesh.get());
                        hullFaces.erase(hullFaces.begin() + j);
                    } else {
                        ++j;
                    }
                } else {
                    ++j;
                }
            }
        }
		auto flatHullFaces = [&](HullFace& hullFace) {
			trimesh::TriMesh* inmesh = hullFace.mesh.get();
			trimesh::vec3 normal = hullFace.normal;
			//hullFace->write("test/before.stl");
            const auto& apoints = inmesh->vertices;
            float d = std::numeric_limits<float>::lowest();
            for (const auto& p : apoints) {
                const auto& project = (p DOT normal);
                if (project > d) {
					hullFace.pt = p;
					d = project;
                }
            }
            std::vector<trimesh::vec3> newPts;
            newPts.reserve(apoints.size());
            for (const auto& p : apoints) {
                const auto& dist = (p DOT normal) - d;
                newPts.emplace_back(p - (normal * dist));
            }
            inmesh->vertices.swap(newPts);
			//hullFace->write("test/after.stl");
        };
        for (auto& hullFace : hullFaces) {
            flatHullFaces(hullFace);
        }
        /*for (int i = 0; i < hullFaces.size(); ++i) {
            hullFaces[i].mesh->write("test/separates" + std::to_string(i) + ".stl");
        }*/
        auto adjustmentMesh = [&](trimesh::TriMesh * inmesh, trimesh::vec3 & normal) {
            //将面轻微抬起，防止与模型的面重叠
            for (trimesh::point& apoint : inmesh->vertices) {
                apoint += normal * 0.2;
            }
            //绕z和y旋转，使平面变平
            const trimesh::vec3 XYnormal(0.0f, 0.0f, 1.0f);
            trimesh::fxform xf = trimesh::fxform::rot_into(normal, XYnormal);
            for (trimesh::point& apoint : inmesh->vertices) {
                apoint = (xf * apoint);
            }
            return xf;
        };
        auto checkMesh = [&](const trimesh::TriMesh* inmesh) {
            const auto& polygon = inmesh->vertices;
            //检查多边形的内角，并丢弃角小于以下阈值的多边形
            static constexpr double PI = 3.141592653589793238;
            const double angle_threshold = ::cos(10.0 * (double)PI / 180.0);
            for (unsigned int i = 0; i < polygon.size(); ++i) {
                const trimesh::point& prec = polygon[(i == 0) ? polygon.size() - 1 : i - 1];
                const trimesh::point & curr = polygon[i];
                const trimesh::point & next = polygon[(i == polygon.size() - 1) ? 0 : i + 1];
                if (normalized(prec - curr).dot(normalized(next - curr)) > angle_threshold) {
                    return true;
                }
            }
            return false;
        };
        auto indentationMesh = [&](trimesh::TriMesh* inmesh) {
            auto polygon = inmesh->vertices;
            trimesh::point centroid = std::accumulate(polygon.begin(), polygon.end(), trimesh::point(0.0, 0.0, 0.0));
            centroid /= (double)polygon.size();
            for (auto& vertex : polygon) {
                vertex = 0.9f * vertex + 0.1f * centroid;
            }
            //多边形现在是简单和凸的，我们将使尖角变圆，这样看起来更好
            //该算法取一个顶点，计算各自边的中间值，然后移动顶点
            //接近平均水平(由“aggressivity”控制)。重复k次。
            const size_t k = 10; // number of iterations
            const float aggressivity = 0.2f;  // agressivity
            const size_t N = polygon.size();
            std::vector<std::pair<unsigned int, unsigned int>> neighbours;
            if (k != 0) {
                std::vector<trimesh::point> points_out(2 * k * N); // vector long enough to store the future vertices
                for (size_t j = 0; j < N; ++j) {
                    points_out[j * 2 * k] = polygon[j];
                    neighbours.push_back(std::make_pair((int)(j * 2 * k - k) < 0 ? (N - 1) * 2 * k + k : j * 2 * k - k, j * 2 * k + k));
                }

                for (size_t i = 0; i < k; ++i) {
                    // Calculate middle of each edge so that neighbours points to something useful:
                    for (unsigned int j = 0; j < N; ++j)
                        if (i == 0)
                            points_out[j * 2 * k + k] = 0.5f * (points_out[j * 2 * k] + points_out[j == N - 1 ? 0 : (j + 1) * 2 * k]);
                        else {
                            float r = 0.2 + 0.3 / (k - 1) * i; // the neighbours are not always taken in the middle
                            points_out[neighbours[j].first] = r * points_out[j * 2 * k] + (1 - r) * points_out[neighbours[j].first - 1];
                            points_out[neighbours[j].second] = r * points_out[j * 2 * k] + (1 - r) * points_out[neighbours[j].second + 1];
                        }
                    // Now we have a triangle and valid neighbours, we can do an iteration:
                    for (unsigned int j = 0; j < N; ++j)
                        points_out[2 * k * j] = (1 - aggressivity) * points_out[2 * k * j] +
                        aggressivity * 0.5f * (points_out[neighbours[j].first] + points_out[neighbours[j].second]);

                    for (auto& n : neighbours) {
                        ++n.first;
                        --n.second;
                    }
                }
                // replace the coarse polygon with the smooth one that we just created
                inmesh->vertices.swap(points_out);
            }
        };
        auto triangularization = [&](trimesh::TriMesh * inmesh) {
            //三角化
            const auto apoints = inmesh->vertices;
            const int anums = apoints.size();
            if (anums < 3) {
                return;
            }
            trimesh::point acenter;
            for (int i = 0; i < anums; ++i) {
                acenter += apoints[i] / anums;
            }
            inmesh->vertices.emplace_back(acenter);
            for (size_t inx = 0; inx < anums; ++inx) {
                const auto& ninx = (inx + 1 == anums) ? 0 : (inx + 1);
                inmesh->faces.emplace_back(trimesh::TriMesh::Face(anums, ninx, inx));
            }
        };
        for (int polygon_id = 0; polygon_id < hullFaces.size(); ++polygon_id) {
            trimesh::TriMesh* hullmesh = hullFaces[polygon_id].mesh.get();
            trimesh::vec3 normal = hullFaces[polygon_id].normal;
            trimesh::fxform xf = adjustmentMesh(hullmesh, normal);
            trimesh::TriMesh* hull = qhullWrapper::convex_hull_2d(hullmesh);
            if (checkMesh(hull)) {
                hullFaces.erase(hullFaces.begin() + (polygon_id--));
                continue;
            }
            indentationMesh(hull);
            //通过逆矩阵转换回三维坐标
            for (trimesh::point& apoint : hull->vertices) {
                apoint = trimesh::inv(xf) * apoint;
            }
            triangularization(hull);
            hullFaces[polygon_id].mesh.reset(hull);
            //hull->write("test/hull" + std::to_string(polygon_id) + ".stl");
        }
        /*for (int i = 0; i < hullFaces.size(); ++i) {
            hullFaces[i].mesh->write("test/hullfaces" + std::to_string(i) + ".stl");
        }*/
    }
	
	void hullFacesFromMeshNear(const HMeshPtr& mesh, std::vector<HullFace>& hullFaces, float thresholdNormal, float dmin)
	{
		//mesh->write("boat.stl");
		const auto& faces = mesh->faces;
		const auto& points = mesh->vertices;
		const int facenums = faces.size();
		std::vector<trimesh::vec3> normals;
		std::vector<trimesh::vec3> centers;
		std::vector<float> areas;
		normals.reserve(facenums);
		centers.reserve(facenums);
		areas.reserve(facenums);
		for (const auto& f : faces) {
			const auto& a = points[f[0]];
			const auto& b = points[f[1]];
			const auto& c = points[f[2]];
			trimesh::vec3 n = 0.5 * ((b - a)TRICROSS(c - a));
			centers.emplace_back((a + b + c) / 3.0);
			areas.emplace_back(trimesh::len(n));
			trimesh::normalize(n);
			normals.emplace_back(n);
		}
		std::vector<bool> flags(facenums, true);
		for (auto& hullFace : hullFaces) {
			std::vector<int> selectFaces;
			selectFaces.reserve(facenums);
			const auto& pt = hullFace.pt;
			const auto& dir = hullFace.normal;
			for (int i = 0; i < normals.size(); ++i) {
				if (flags[i] && ((normals[i] DOT dir) >= thresholdNormal)) {
					flags[i] = false;
					const auto& pc = centers[i];
					float d = (pt - pc) DOT dir;
					if (std::fabs(d) <= dmin) {
						selectFaces.emplace_back(i);
					}
				}
			}
			if (selectFaces.empty()) {
				hullFace.hulldist = std::numeric_limits<float>::max();
			} else {
				std::vector<int> largestPart;
				auto selectLargestPart = [&centers, &areas, &dir, &pt, &dmin](const std::vector<int>& inputFace, std::vector<int>& largestPart) {
					std::vector<std::vector<int>> parts;
					const int nums = inputFace.size();
					std::queue<int> inputQueues;
					parts.reserve(100);
					for (const auto& f : inputFace) {
						inputQueues.emplace(f);
					}
					while (!inputQueues.empty()) {
						std::vector<int> currentFaces;
						currentFaces.reserve(nums);
						int fr = inputQueues.front();
						const auto& pr = centers[fr];
						inputQueues.pop();
						float d = (pt - pr) DOT dir;
						if (std::fabs(d) > dmin) continue;
						currentFaces.emplace_back(fr);
						int count = inputQueues.size();
						int curTime = 0;
						while (!inputQueues.empty()) {
							int fa = inputQueues.front();
							const auto& pa = centers[fa];
							float dist = (pr - pa) DOT dir;
							inputQueues.pop();
							if (std::fabs(dist) <= dmin) {
								currentFaces.emplace_back(fa);
								curTime = 0;
								--count;
							} else {
								inputQueues.emplace(fa);
								++curTime;
							}
							if (curTime >= count) {
								break;
							}
						}
						parts.emplace_back(currentFaces);
					}
					largestPart.clear();
					float maxArea = 0.0f;
					for (auto& part : parts) {
						float sumArea = 0.0f;
						for (const auto& f : part) {
							sumArea += areas[f];
						}
						if (sumArea > maxArea) {
							maxArea = sumArea;
							largestPart.swap(part);
						}
					}
				};
				
				selectLargestPart(selectFaces, largestPart);
				//trimesh::TriMesh selectMesh, partMesh;
				//FaceGenerateMesh(mesh.get(), selectFaces, selectMesh);
				//hullFace.mesh->write("test/hullmesh.stl");
				//selectMesh.write("test/selectMesh.stl");
				//FaceGenerateMesh(mesh.get(), largestPart, partMesh);
				//partMesh.write("test/partMesh.stl");
				float partArea = 0.0f, partDist = 0.0f;
				for (const auto& f : largestPart) {
					const auto& area = areas[f];
					const auto& pc = centers[f];
					float d = std::fabs((pt - pc) DOT dir);
					partDist += area * d;
					partArea += area;
				}
				hullFace.hulldist = partDist / partArea;
				hullFace.hullarea = partArea;
			}
		}
		std::sort(hullFaces.begin(), hullFaces.end(), [=](HullFace& hulla, HullFace& hullb) {
			if (hulla.hulldist <= dmin && hullb.hulldist <= dmin) return hulla.hullarea > hullb.hullarea;
			else return hulla.hulldist < hullb.hulldist;
		});
	}
}