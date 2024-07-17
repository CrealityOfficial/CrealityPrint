#include "cylindercollide.h"

#include "msbase/mesh/get.h"
#include "msbase/space/intersect.h"
#include "msbase/mesh/merge.h"
#include "msbase/mesh/dumplicate.h"
#include "msbase/mesh/tinymodify.h"
#include "msbase/space/angle.h"
#include "msbase/data/conv.h"
#include "msbase/primitive/primitive.h"

#include "trimesh2/Vec3Utils.h"
#include "uniformpoints.h"
#include "trianglesplit.h"

#include <map>
#include <list>
#include <assert.h>
#include <cmath>
#include <algorithm>
#include <functional>

namespace msbase
{
	using namespace trimesh;

	void testCollide(trimesh::TriMesh* mesh, trimesh::TriMesh* cylinderMesh,
		std::vector<trimesh::vec3>& meshNormals, std::vector<trimesh::vec3>& cylinderNormals,
		std::vector<TriMesh::Face>& meshFocusFaces, std::vector<trimesh::TriMesh::Face>& cylinderFocusFaces,
		std::vector<FaceCollide>& meshTris)
	{
		int meshNum = (int)meshFocusFaces.size();
		int cylinderNum = (int)cylinderFocusFaces.size();
		for (int i = 0; i < meshNum; ++i)
			meshTris.at(i).flag = 1;

		//test inner
		for (int i = 0; i < meshNum; ++i)
		{
			TriMesh::Face& focusFace = meshFocusFaces.at(i);

			vec3& focusV1 = mesh->vertices.at(focusFace[0]);
			vec3& focusV2 = mesh->vertices.at(focusFace[1]);
			vec3& focusV3 = mesh->vertices.at(focusFace[2]);

			bool testIn = true;
			for (int j = 0; j < cylinderNum; ++j)
			{
				TriMesh::Face& cylinderFace = cylinderFocusFaces[j];
				vec3& cylinderV1 = cylinderMesh->vertices.at(cylinderFace[0]);
				vec3& n = cylinderNormals.at(j);
				float d1 = n.dot(focusV1 - cylinderV1);
				float d2 = n.dot(focusV2 - cylinderV1);
				float d3 = n.dot(focusV3 - cylinderV1);
				if (d1 > 0.0f || d2 > 0.0f || d3 > 0.0f)
				{
					testIn = false;
					break;
				}
			}

			if (testIn)
			{
				meshTris.at(i).flag = -1;
			}
		}

		struct CTemp
		{
			int index;
			int e1;
			int e2;
			dvec3 c1;
			dvec3 c2;
		};

		auto fcollid = [](const dvec3& tv, const dvec3& v1, const dvec3& v2, double dv, double d1, double d2, int index, CTemp& temp) {
			dvec3 c1 = (dv / (dv - d1)) * v1 - (d1 / (dv - d1)) * tv;
			dvec3 c2 = (dv / (dv - d2)) * v2 - (d2 / (dv - d2)) * tv;

			temp.index = index;

			temp.c1 = c1;
			temp.c2 = c2;
			temp.e1 = index;
			temp.e2 = (index + 2) % 3;
		};

		for (int i = 0; i < meshNum; ++i)
		{
			FaceCollide& fc = meshTris.at(i);
			if (fc.flag != -1)
			{
#if _DEBUG
#endif

				std::vector<TriTri>& meshTri = fc.tris;
				TriMesh::Face& focusFace = meshFocusFaces.at(i);

				dvec3 focusV1 = msbase::vec32dvec3(mesh->vertices.at(focusFace[0]));
				dvec3 focusV2 = msbase::vec32dvec3(mesh->vertices.at(focusFace[1]));
				dvec3 focusV3 = msbase::vec32dvec3(mesh->vertices.at(focusFace[2]));

				dvec3 meshNormal = msbase::vec32dvec3(meshNormals.at(i));
				for (int j = 0; j < cylinderNum; ++j)
				{
					dvec3 cylinderNormal = msbase::vec32dvec3(cylinderNormals.at(j));

					if (cylinderNormal == meshNormal)
						continue;

					TriMesh::Face& cylinderFace = cylinderFocusFaces[j];

					dvec3 cylinderV1 = msbase::vec32dvec3(cylinderMesh->vertices.at(cylinderFace[0]));
					dvec3 cylinderV2 = msbase::vec32dvec3(cylinderMesh->vertices.at(cylinderFace[1]));
					dvec3 cylinderV3 = msbase::vec32dvec3(cylinderMesh->vertices.at(cylinderFace[2]));

					double mesh2Cylinder1 = cylinderNormal.dot(focusV1 - cylinderV1);
					double mesh2Cylinder2 = cylinderNormal.dot(focusV2 - cylinderV1);
					double mesh2Cylinder3 = cylinderNormal.dot(focusV3 - cylinderV1);
					double Cylinder2mesh1 = meshNormal.dot(cylinderV1 - focusV1);
					double Cylinder2mesh2 = meshNormal.dot(cylinderV2 - focusV1);
					double Cylinder2mesh3 = meshNormal.dot(cylinderV3 - focusV1);

					std::vector<CTemp> temps;
					float l0 = mesh2Cylinder1 * mesh2Cylinder2;
					float l1 = mesh2Cylinder2 * mesh2Cylinder3;
					float l2 = mesh2Cylinder1 * mesh2Cylinder3;
					if (l0 == 0.0f && l1 == 0.0f && l2 == 0.0f)
						continue;

					CTemp temp;
					if (l0 >= 0.0f && (l1 <= 0.0f || l2 <= 0.0f))
					{
						fcollid(focusV3, focusV1, focusV2, mesh2Cylinder3, mesh2Cylinder1, mesh2Cylinder2, 2, temp);
						temps.push_back(temp);
					}
					else if (l0 < 0.0f)
					{
						if (l1 <= 0.0f)
						{
							fcollid(focusV2, focusV3, focusV1, mesh2Cylinder2, mesh2Cylinder3, mesh2Cylinder1, 1, temp);
							temps.push_back(temp);
						}
						else if (l2 <= 0.0f)
						{
							fcollid(focusV1, focusV2, focusV3, mesh2Cylinder1, mesh2Cylinder2, mesh2Cylinder3, 0, temp);
							temps.push_back(temp);
						}
					}

					l0 = Cylinder2mesh1 * Cylinder2mesh2;
					l1 = Cylinder2mesh2 * Cylinder2mesh3;
					l2 = Cylinder2mesh1 * Cylinder2mesh3;
					if (l0 == 0.0f && l1 == 0.0f && l2 == 0.0f)
						continue;

					if (l0 >= 0.0f && (l1 <= 0.0f || l2 <= 0.0f))
					{
						fcollid(cylinderV3, cylinderV1, cylinderV2, Cylinder2mesh3, Cylinder2mesh1, Cylinder2mesh2, 2, temp);
						temps.push_back(temp);
					}
					else if (l0 < 0.0f)
					{
						if (l1 <= 0.0f)
						{
							fcollid(cylinderV2, cylinderV3, cylinderV1, Cylinder2mesh2, Cylinder2mesh3, Cylinder2mesh1, 1, temp);
							temps.push_back(temp);
						}
						else if (l2 <= 0.0f)
						{
							fcollid(cylinderV1, cylinderV2, cylinderV3, Cylinder2mesh1, Cylinder2mesh2, Cylinder2mesh3, 0, temp);
							temps.push_back(temp);
						}
					}

					if (temps.size() == 2)
					{
						std::vector<dvec3> points;
						points.push_back(temps.at(0).c1);
						points.push_back(temps.at(0).c2);
						points.push_back(temps.at(1).c1);
						points.push_back(temps.at(1).c2);
						dvec3 axis = points.at(1) - points.at(0);
						normalize(axis);

						std::vector<double> diss(4);
						for (int k = 0; k < 4; ++k)
						{
							diss.at(k) = dot(points.at(k) - points.at(0), axis);
						}

						bool swapped = false;
						if (diss[3] < diss[2])
						{
							std::swap(points.at(3), points.at(2));
							std::swap(diss[3], diss[2]);
							swapped = true;
						}

						double d[3] = {};
						d[0] = mesh2Cylinder1;
						d[1] = mesh2Cylinder2;
						d[2] = mesh2Cylinder3;
						double dd[3] = {};
						dd[0] = Cylinder2mesh1;
						dd[1] = Cylinder2mesh2;
						dd[2] = Cylinder2mesh3;
						TriTri mt;
						mt.topPositive = d[temps[0].index] >= 0.0;
						mt.cylinderTopIndex = temps[1].index;
						mt.cylinderTopPositive = d[temps[1].index] >= 0.0;

						dvec3 v1;
						dvec3 v2;

						if (diss[1] >= diss[2] && diss[3] >= diss[0])
						{
							ivec2 mtIndex(-1, -1);
							ivec2 mcIndex(-1, -1);
							if (diss[2] <= diss[0])
							{
								if (diss[3] < diss[1])
								{
									v1 = points.at(0);
									v2 = points.at(3);
									mtIndex[0] = temps[0].e1;
									mcIndex[1] = temps[1].e2;
								}
								else
								{
									v1 = points.at(0);
									v2 = points.at(1);
									mtIndex[0] = temps[0].e1;
									mtIndex[1] = temps[0].e2;
								}
							}
							else if (diss[2] <= diss[1])
							{
								if (diss[3] < diss[1])
								{
									v1 = points.at(2);
									v2 = points.at(3);
									mcIndex[0] = temps[1].e1;
									mcIndex[1] = temps[1].e2;
								}
								else
								{
									v1 = points.at(2);
									v2 = points.at(1);
									mtIndex[1] = temps[0].e2;
									mcIndex[0] = temps[1].e1;
								}
							}

							mt.v1 = v1;
							mt.v2 = v2;

							mt.index = mtIndex;
							mt.cindex = mcIndex;
							meshTri.push_back(mt);
							fc.flag = 0;
						}
					}
				}
			}
		}
	}

	void traverse(const int& a,int& b,int& c)
	{
		if (a != -1)
		{
			if (b == -1)
				b = a;

			if (c == -1)
				c = a;
		}
	}

	void pointInner(std::vector<int>& pointPosition, std::vector<trimesh::TriMesh::Face>& focusFaces)
	{
		bool isHaveTrue= false;
		for (auto n: pointPosition)
		{
			if (n != -1)
			{
				isHaveTrue = true;
				break;
			}
		}
		if (!isHaveTrue)
			return;

		int n = 0;
		while (1)
		{
			if (n>= focusFaces.size())
			{
				n = 0;
			}
			TriMesh::Face& cylinderFace = focusFaces.at(n++);
			traverse(pointPosition[cylinderFace[0]], pointPosition[cylinderFace[1]], pointPosition[cylinderFace[2]]);
			traverse(pointPosition[cylinderFace[1]], pointPosition[cylinderFace[0]], pointPosition[cylinderFace[2]]);
			traverse(pointPosition[cylinderFace[2]], pointPosition[cylinderFace[0]], pointPosition[cylinderFace[1]]);

			bool bFfinished = true;
			for (int nn : pointPosition)
			{
				if (nn == -1)
				{
					bFfinished = false;
					break;
				}
			}
			if (bFfinished)
				break;
		}
		return;
	}

	void generateNewTriangles(std::vector<trimesh::TriMesh::Face>& focusFaces, trimesh::TriMesh* mesh, std::vector<trimesh::vec3>& normals,
		std::vector<FaceCollide>& tris, std::vector<trimesh::vec3>& newTriangles, bool positive, std::vector<int>& cylinderFlag, ccglobal::Tracer* tracer, DrillDebugger* debugger)
	{
		int meshNum = (int)focusFaces.size();
		std::vector<std::vector<bool>> vctInner;
		vctInner.resize(meshNum, std::vector<bool>(3,true));
#if _DEBUG
		int debug_index = 0;
#endif
		for (int i = 0; i < meshNum; ++i)
		{
			trimesh::TriMesh::Face& face = focusFaces.at(i);
			FaceCollide& fcollide = tris.at(i);
			std::vector<TriTri>& tritri = fcollide.tris;
			size_t size = tritri.size();
			if (size > 0)
			{
				std::vector<TriSegment> trisegments(size);
				for (int j = 0; j < size; ++j)
				{
					TriSegment& seg = trisegments.at(j);
					TriTri& tri = tritri.at(j);
					seg.index = tri.index;
					seg.topPositive = tri.topPositive;
					seg.v1 = tri.v1;
					seg.v2 = tri.v2;
				}
				std::vector<trimesh::vec3> triangles;
				if (splitTriangle(msbase::vec32dvec3(mesh->vertices[face[0]]),
					msbase::vec32dvec3(mesh->vertices[face[1]]), msbase::vec32dvec3(mesh->vertices[face[2]]),
					trisegments, positive, triangles, vctInner[i],tracer))
				{
					newTriangles.insert(newTriangles.end(), triangles.begin(), triangles.end());
				}
				else
				{
					if (tracer)
						tracer->failed("error split Triangle");
					assert("error split Triangle");
				}
			}
		}

		if (!positive)
		{
			std::vector<int> pointPosition;
			pointPosition.resize(mesh->vertices.size(), -1);
			//init
			for (size_t i = 0; i < tris.size(); i++)
			{
				if (tris[i].flag != 0)
					continue;
				TriMesh::Face& cylinderFace = mesh->faces.at(i);
				pointPosition[cylinderFace[0]]= vctInner[i][0];
				pointPosition[cylinderFace[1]] = vctInner[i][1];
				pointPosition[cylinderFace[2]] = vctInner[i][2];
			}
			//recursion
			//for (size_t i = 0; i < tris.size(); i++)
			//{
			//	if (tris[i].flag != 0)
			//		continue;
				pointInner(pointPosition, focusFaces);
			//}

			for (size_t i = 0; i < tris.size(); i++)
			{
				if (tris[i].flag == 0)
					continue;
				TriMesh::Face& cylinderFace = mesh->faces.at(i);
				if (pointPosition[cylinderFace[0]] ==0
					|| pointPosition[cylinderFace[1]] == 0
					|| pointPosition[cylinderFace[2]] == 0)
				{
					cylinderFlag[i] = -1;
				}		
			}
		}
	}

	void generateCylinderTriangles(trimesh::TriMesh* cylinder, std::vector<trimesh::vec3>& normals,
		std::vector<FaceCollide>& tris, std::vector<trimesh::vec3>& newTriangles, bool positive)
	{

	}
	
	trimesh::TriMesh* generatePatchMeshDrill(trimesh::TriMesh* oldMesh, FlagPatch& flagFaces, int flag)
	{

		trimesh::TriMesh* newMesh = new trimesh::TriMesh();
		newMesh->vertices = oldMesh->vertices;
		int size = (int)flagFaces.size();
		if (size > 0)
		{
			for (int i = 0; i < size; ++i)
			{
				if (flagFaces.at(i) == flag)
				{
					newMesh->faces.push_back(oldMesh->faces.at(i));
				}				
			}
		}
		return newMesh;
	}


	trimesh::TriMesh* generatePatchMesh(trimesh::TriMesh* oldMesh, FlagPatch& flagFaces, int flag)
	{
		auto fillmesh = [&oldMesh](trimesh::TriMesh* m) {
			for (trimesh::TriMesh::Face& f : m->faces)
			{
				m->vertices.push_back(oldMesh->vertices.at(f.x));
				m->vertices.push_back(oldMesh->vertices.at(f.y));
				m->vertices.push_back(oldMesh->vertices.at(f.z));
			}

			int index = 0;
			//remap
			for (trimesh::TriMesh::Face& f : m->faces)
			{
				f.x = index++;
				f.y = index++;
				f.z = index++;
			}
		};

		trimesh::TriMesh* newMesh = new trimesh::TriMesh();
		int size = (int)flagFaces.size();
		if (size > 0)
		{
			for (int i = 0; i < size; ++i)
			{
				if (flagFaces.at(i) == flag)
					newMesh->faces.push_back(oldMesh->faces.at(i));
			}
		}
		fillmesh(newMesh);
		return newMesh;
	}

	void findFocusVerticesIndex(trimesh::TriMesh* mesh, std::vector<int>& meshFocusVertices
		, const trimesh::vec3& v1, const trimesh::vec3& v2, const trimesh::vec3& v3
		,int& index1, int& index2, int& index3)
	{
		index1 = -1;
		index2 = -1;
		index3 = -1;
		for (size_t i = 0; i < meshFocusVertices.size(); i++)
		{
			if (index1 == -1
				&& mesh->vertices[meshFocusVertices[i]].x == v1.x
				&& mesh->vertices[meshFocusVertices[i]].y == v1.y
				&& mesh->vertices[meshFocusVertices[i]].z == v1.z)
			{
				index1 =  meshFocusVertices[i];
			}

			if (index2 == -1
				&& mesh->vertices[meshFocusVertices[i]].x == v2.x
				&& mesh->vertices[meshFocusVertices[i]].y == v2.y
				&& mesh->vertices[meshFocusVertices[i]].z == v2.z)
			{
				index2 = meshFocusVertices[i];
			}

			if (index3 == -1
				&& mesh->vertices[meshFocusVertices[i]].x == v3.x
				&& mesh->vertices[meshFocusVertices[i]].y == v3.y
				&& mesh->vertices[meshFocusVertices[i]].z == v3.z)
			{
				index3 = meshFocusVertices[i];
			}

			if (index1>=0 && index2 >= 0 && index3 >= 0)
				break;
		}
	}

	trimesh::TriMesh* getNewOriginMehsh(trimesh::TriMesh* oldMesh, FlagPatch& flagFaces, int flag, ccglobal::Tracer* tracer)
	{
		if (!oldMesh)
		{
			if (tracer)
				tracer->failed("Mesh is empty");
			return nullptr;
		}

		trimesh::TriMesh* newMesh = generatePatchMeshDrill(oldMesh, flagFaces, flag);
		//trimesh::TriMesh* newMesh = generatePatchMesh(oldMesh, flagFaces, flag);
		return newMesh;
	}

	trimesh::TriMesh* getNewFocusMehsh(TriPatch& newTriangles)
	{
		trimesh::TriMesh* newMesh = new trimesh::TriMesh();

		auto addmesh = [](trimesh::TriMesh* mesh, const trimesh::vec3& v1,
			const trimesh::vec3& v2, const trimesh::vec3& v3) {
				int index = (int)mesh->vertices.size();
				mesh->vertices.push_back(v1);
				mesh->vertices.push_back(v2);
				mesh->vertices.push_back(v3);

				mesh->faces.push_back(trimesh::TriMesh::Face(index, index + 1, index + 2));
		};

		int newVertexSize = (int)(newTriangles.size() / 3);
		for (int i = 0; i < newVertexSize; ++i)
			addmesh(newMesh, newTriangles.at(3 * i), newTriangles.at(3 * i + 1), newTriangles.at(3 * i + 2));

		return newMesh;


	}
	
	trimesh::TriMesh* generateAppendMeshWithDumplicate(trimesh::TriMesh* oldMesh, std::vector<int>& meshFocusVertices, trimesh::TriMesh* drillMesh, ccglobal::Tracer* tracer)
	{
		if (!oldMesh)
		{
			if (tracer)
				tracer->failed("Mesh is empty");
			return nullptr;
		}

		trimesh::TriMesh* newMesh = new trimesh::TriMesh();
		newMesh->vertices = oldMesh->vertices;
		newMesh->faces = oldMesh->faces;

		auto addmesh = [](trimesh::TriMesh* mesh, std::vector<int>& meshFocusVertices, const trimesh::vec3& v1,
			const trimesh::vec3& v2, const trimesh::vec3& v3) {
				
				int index1, index2, index3;
				findFocusVerticesIndex(mesh, meshFocusVertices, v1, v2, v3, index1, index2, index3);

				if (index1 == -1)
				{
					index1 = (int)mesh->vertices.size();
					mesh->vertices.push_back(v1);
					meshFocusVertices.push_back(index1);
				}

				if (index2 == -1)
				{
					index2 = (int)mesh->vertices.size();
					mesh->vertices.push_back(v2);
					meshFocusVertices.push_back(index2);
				}

				if (index3 == -1)
				{
					index3 = (int)mesh->vertices.size();
					mesh->vertices.push_back(v3);
					meshFocusVertices.push_back(index3);
				}

				mesh->faces.push_back(trimesh::TriMesh::Face(index1, index2, index3));
		};

		for (int i = 0; i < drillMesh->faces.size(); ++i)
		{
			TriMesh::Face& face = drillMesh->faces.at(i);
			addmesh(newMesh, meshFocusVertices, drillMesh->vertices[face.x], drillMesh->vertices[face.y], drillMesh->vertices[face.z]);
		}

		if(drillMesh)
			delete drillMesh;

		return newMesh;
	}

	trimesh::TriMesh* generateAppendMesh(trimesh::TriMesh* oldMesh, FlagPatch& flagFaces,
		TriPatch& newTriangles, int flag, ccglobal::Tracer* tracer)
	{
		if (!oldMesh)
		{
			if (tracer)
				tracer->failed("Mesh is empty");
			return nullptr;
		}

		trimesh::TriMesh* newMesh = generatePatchMesh(oldMesh, flagFaces, flag);
		
		auto addmesh = [](trimesh::TriMesh* mesh, const trimesh::vec3& v1,
			const trimesh::vec3& v2, const trimesh::vec3& v3) {
				int index = (int)mesh->vertices.size();
				mesh->vertices.push_back(v1);
				mesh->vertices.push_back(v2);
				mesh->vertices.push_back(v3);
		
				mesh->faces.push_back(trimesh::TriMesh::Face(index, index + 1, index + 2));
		};
		
		int newVertexSize = (int)(newTriangles.size() / 3);
		for (int i = 0; i < newVertexSize; ++i)
			addmesh(newMesh, newTriangles.at(3 * i), newTriangles.at(3 * i + 1), newTriangles.at(3 * i + 2));

		return newMesh;
	}

	OptimizeCylinderCollide::OptimizeCylinderCollide(trimesh::TriMesh* mesh, trimesh::TriMesh* cylinder,
		ccglobal::Tracer* tracer, DrillDebugger* debugger)
		:m_mesh(mesh)
		, focusTriangle(0)
		, m_cylinderResolution(0)
		, m_tracer(tracer)
		, m_debugger(debugger)
		, cylinderTriangles(0)
		, m_cylinderRadius(0.0f)
		, m_cylinderDepth(0.0f)
	{
		m_cylinder = new trimesh::TriMesh();
		*m_cylinder = *cylinder;
		if (m_mesh && m_cylinder && m_mesh->faces.size() > 0)
			calculate();
	}
	
	OptimizeCylinderCollide::OptimizeCylinderCollide(trimesh::TriMesh* mesh,
		int resolution, double radius, double depth, trimesh::point pointStart, trimesh::point dir,
		ccglobal::Tracer* tracer, DrillDebugger* debugger)
		:m_mesh(mesh)
		, m_cylinder(nullptr)
		, focusTriangle(0)
		, m_tracer(tracer)
		, m_debugger(debugger)
		, cylinderTriangles(0)
		, m_cylinderResolution(resolution)
		, m_cylinderRadius(radius)
		, m_cylinderDepth(depth)
		, m_cylinderPointStart(pointStart)
		, m_cylinderDir(dir)
	{
		if (m_mesh && m_mesh->faces.size() > 0)
			mycalculate();
	}

	OptimizeCylinderCollide::~OptimizeCylinderCollide()
	{
		//delete
		if (m_cylinder && m_cylinderResolution > 0)
		{
			delete m_cylinder;
			m_cylinder = nullptr;
		}
	}

	void OptimizeCylinderCollide::calculate()
	{
		m_cylinder->need_bbox();
		box3 cylinderBox = m_cylinder->bbox;

		int faces = (int)m_mesh->faces.size();
		totalMeshFlag.resize(faces, 0);

		for (int i = 0; i < faces; ++i)
		{
			TriMesh::Face& face = m_mesh->faces.at(i);

			box3 b;
			for (int j = 0; j < 3; ++j)
			{
				b += m_mesh->vertices.at(face[j]);
			}

			if (cylinderBox.intersects(b))
			{
				meshFocusFacesMapper.push_back(i);
				meshFocusFaces.push_back(face);
				meshFocusVertices.push_back(face.x);
				meshFocusVertices.push_back(face.y);
				meshFocusVertices.push_back(face.z);
			}
			else
			{
				totalMeshFlag.at(i) = 1;
			}
		}

		focusTriangle = (int)meshFocusFaces.size();
		cylinderTriangles = (int)m_cylinder->faces.size();
		focusNormals.resize(focusTriangle);
		cylinderNormals.resize(cylinderTriangles);
		cylinderFlag.resize(cylinderTriangles);
		for (int j = 0; j < cylinderTriangles; ++j)
		{
			TriMesh::Face& cylinderFace = m_cylinder->faces.at(j);

			vec3& cylinderV1 = m_cylinder->vertices.at(cylinderFace[0]);
			vec3& cylinderV2 = m_cylinder->vertices.at(cylinderFace[1]);
			vec3& cylinderV3 = m_cylinder->vertices.at(cylinderFace[2]);

			vec3 n = (cylinderV2 - cylinderV1) TRICROSS(cylinderV3 - cylinderV1);
			cylinderNormals.at(j) = normalized(n);
		}

		for (int j = 0; j < focusTriangle; ++j)
		{
			TriMesh::Face& focusFace = meshFocusFaces.at(j);

			vec3& meshV1 = m_mesh->vertices.at(focusFace[0]);
			vec3& meshV2 = m_mesh->vertices.at(focusFace[1]);
			vec3& meshV3 = m_mesh->vertices.at(focusFace[2]);

			vec3 n = (meshV2 - meshV1) TRICROSS(meshV3 - meshV1);
			focusNormals.at(j) = normalized(n);
		}

		SAFE_TRACER(m_tracer, "OptimizeCylinderCollide meshFocus [%d], cylinder [%d]",
			focusTriangle, cylinderTriangles);
		if (m_debugger)
			m_debugger->onCylinderBoxFocus(meshFocusFaces);

		meshTris.resize(focusTriangle);
		cylinderTris.resize(cylinderTriangles);

		testCollide(m_mesh, m_cylinder, focusNormals, cylinderNormals,
			meshFocusFaces, m_cylinder->faces, meshTris);

		testCollide(m_cylinder, m_mesh, cylinderNormals, focusNormals,
			m_cylinder->faces, meshFocusFaces, cylinderTris);

		for (int i = 0; i < focusTriangle; ++i)
		{
			int index = meshFocusFacesMapper.at(i);
			totalMeshFlag.at(index) = meshTris.at(i).flag;
		}
		for (int i = 0; i < cylinderTriangles; ++i)
		{
			cylinderFlag.at(i) = cylinderTris.at(i).flag;
		}

		if (m_debugger)
		{
			//m_debugger->onMeshOuter(generatePatchMesh(m_mesh, totalMeshFlag, CylinderCollideOuter));
			//m_debugger->onMeshCollide(generatePatchMesh(m_mesh, totalMeshFlag, CylinderCollideCollide));
			//m_debugger->onMeshInner(generatePatchMesh(m_mesh, totalMeshFlag, CylinderCollideInner));

			m_debugger->onMeshOuter(generatePatchMesh(m_cylinder, cylinderFlag, CylinderCollideOuter));
			m_debugger->onMeshCollide(generatePatchMesh(m_cylinder, cylinderFlag, CylinderCollideCollide));
			m_debugger->onMeshInner(generatePatchMesh(m_cylinder, cylinderFlag, CylinderCollideInner));
		}
	}

	void OptimizeCylinderCollide::mycalculate()
	{
		int faces = (int)m_mesh->faces.size();
		totalMeshFlag.resize(faces, 0);
		//m_cylinder->need_bbox();
		//box3 cylinderBox_old = m_cylinder->bbox;

		trimesh::point ZAXIS(0.0f,0.0f,1.0f);
		trimesh::vec3 ax = m_cylinderDir;
		trimesh::quaternion quat = trimesh::quaternion::rotationTo(ax, ZAXIS);

		trimesh::fxform xf = msbase::fromQuaterian(quat);

		m_cylinderPointStart = m_cylinderPointStart + 1.0f * (-m_cylinderDir);
		m_mesh->need_bbox();
		
		trimesh::vec3 newStartPos = xf * m_cylinderPointStart;
		
		trimesh::vec3 boxCenterWorld = xf * m_mesh->bbox.center();
		double cylinderInitDepth = trimesh::dist(boxCenterWorld, newStartPos) + m_mesh->bbox.radius() + 2.0;
		double boxRadius = m_cylinderRadius;
		trimesh::vec3 boxMin = trimesh::vec3(newStartPos.x - boxRadius, newStartPos.y - boxRadius, newStartPos.z);
		trimesh::vec3 boxMax = trimesh::vec3(newStartPos.x + boxRadius, newStartPos.y + boxRadius, newStartPos.z + cylinderInitDepth);
		box3 cylinderBox_new(boxMin, boxMax);

		for (int i = 0; i < faces; ++i)
		{
			TriMesh::Face& face = m_mesh->faces.at(i);
			box3 b;
			for (int j = 0; j < 3; ++j)
			{
				b += xf * m_mesh->vertices.at(face[j]);
			}

			if (cylinderBox_new.intersects(b))
			{
				meshFocusFacesMapper.push_back(i);
				meshFocusFaces.push_back(face);
				meshFocusVertices.push_back(face.x);
				meshFocusVertices.push_back(face.y);
				meshFocusVertices.push_back(face.z);
			}
			else
			{
				totalMeshFlag.at(i) = 1;
			}
		}

		// 确定打孔实际深度，生成圆柱体网格
		m_cylinderDepth = getDrillDepth();
		if (m_cylinderDepth <= 0)
			return;

		m_cylinderDepth += 2.0;

		m_cylinder = msbase::createCylinderMesh(m_cylinderPointStart + m_cylinderDepth * m_cylinderDir,
			m_cylinderPointStart, m_cylinderRadius, m_cylinderResolution, 0.0);

		focusTriangle = (int)meshFocusFaces.size();
		cylinderTriangles = (int)m_cylinder->faces.size();
		focusNormals.resize(focusTriangle);
		cylinderNormals.resize(cylinderTriangles);
		cylinderFlag.resize(cylinderTriangles);
		for (int j = 0; j < cylinderTriangles; ++j)
		{
			TriMesh::Face& cylinderFace = m_cylinder->faces.at(j);

			vec3& cylinderV1 = m_cylinder->vertices.at(cylinderFace[0]);
			vec3& cylinderV2 = m_cylinder->vertices.at(cylinderFace[1]);
			vec3& cylinderV3 = m_cylinder->vertices.at(cylinderFace[2]);

			vec3 n = (cylinderV2 - cylinderV1) TRICROSS(cylinderV3 - cylinderV1);
			cylinderNormals.at(j) = normalized(n);
		}

		for (int j = 0; j < focusTriangle; ++j)
		{
			TriMesh::Face& focusFace = meshFocusFaces.at(j);

			vec3& meshV1 = m_mesh->vertices.at(focusFace[0]);
			vec3& meshV2 = m_mesh->vertices.at(focusFace[1]);
			vec3& meshV3 = m_mesh->vertices.at(focusFace[2]);

			vec3 n = (meshV2 - meshV1) TRICROSS(meshV3 - meshV1);
			focusNormals.at(j) = normalized(n);
		}

		SAFE_TRACER(m_tracer, "OptimizeCylinderCollide meshFocus [%d], cylinder [%d]",
			focusTriangle, cylinderTriangles);
		if (m_debugger)
			m_debugger->onCylinderBoxFocus(meshFocusFaces);


		meshTris.resize(focusTriangle);
		cylinderTris.resize(cylinderTriangles);

		testCollide(m_mesh, m_cylinder, focusNormals, cylinderNormals,
			meshFocusFaces, m_cylinder->faces, meshTris);

		testCollide(m_cylinder, m_mesh, cylinderNormals, focusNormals,
			m_cylinder->faces, meshFocusFaces, cylinderTris);

		for (int i = 0; i < focusTriangle; ++i)
		{
			int index = meshFocusFacesMapper.at(i);
			totalMeshFlag.at(index) = meshTris.at(i).flag;
		}
		for (int i = 0; i < cylinderTriangles; ++i)
		{
			cylinderFlag.at(i) = cylinderTris.at(i).flag;
		}

		if (m_debugger)
		{
			//m_debugger->onMeshOuter(generatePatchMesh(m_mesh, totalMeshFlag, CylinderCollideOuter));
			//m_debugger->onMeshCollide(generatePatchMesh(m_mesh, totalMeshFlag, CylinderCollideCollide));
			//m_debugger->onMeshInner(generatePatchMesh(m_mesh, totalMeshFlag, CylinderCollideInner));

			m_debugger->onMeshOuter(generatePatchMesh(m_cylinder, cylinderFlag, CylinderCollideOuter));
			m_debugger->onMeshCollide(generatePatchMesh(m_cylinder, cylinderFlag, CylinderCollideCollide));
			m_debugger->onMeshInner(generatePatchMesh(m_cylinder, cylinderFlag, CylinderCollideInner));
		}
	}

	double OptimizeCylinderCollide::getDrillDepth()
	{
		// focus 集合与打洞方向向量求交，并确定三角面到打洞起点的距离，以及法线与打洞方向是否同向
		float t, u, v;
		trimesh::dvec3 intersectedPos(NAN, NAN, NAN);
		std::map<int, double> faceDisMap;
		std::map<int, bool> directionFlagMap;
		std::vector<int> focusFacesIntersected;
		std::vector<int> focusFacesNoIntersected;
		for (int i = 0; i < meshFocusFacesMapper.size(); i++)
		{
			int faceIndex = meshFocusFacesMapper[i];
			trimesh::TriMesh::Face& face = m_mesh->faces[faceIndex];
			bool isIntersected = rayIntersectTriangle(m_cylinderPointStart, m_cylinderDir, m_mesh->vertices[face[0]], m_mesh->vertices[face[1]], m_mesh->vertices[face[2]],
				&t, &u, &v);
			if (isIntersected)
			{
				trimesh::vec3 faceNormal = trimesh::normalized((m_mesh->vertices[face[1]] - m_mesh->vertices[face[0]]) TRICROSS(m_mesh->vertices[face[2]] - m_mesh->vertices[face[0]]));
				directionFlagMap[faceIndex] = faceNormal.dot(m_cylinderDir) < 0;

				trimesh::vec3 vertices1 = m_mesh->vertices[face[0]];
				trimesh::vec3 vertices2 = m_mesh->vertices[face[1]];
				trimesh::vec3 vertices3 = m_mesh->vertices[face[2]];

				intersectedPos = m_cylinderPointStart + t * m_cylinderDir;

				double depth2 = trimesh::dist2(intersectedPos, trimesh::dvec3(m_cylinderPointStart));
				faceDisMap[faceIndex] = depth2;

				focusFacesIntersected.push_back(faceIndex); 
			}
			else
			{
				focusFacesNoIntersected.push_back(faceIndex);
			}
		}

		// 按照打洞起点到三角面距离排序
		std::function<bool(int, int)> compareFunc = [&faceDisMap](int A, int B)->bool
		{
			double dis2A = faceDisMap[A];
			double dis2B = faceDisMap[B];

			return dis2A < dis2B;
		};
		std::sort(focusFacesIntersected.begin(), focusFacesIntersected.end(), compareFunc);

		// 确定打洞深度
		int i;   
		int layerFlag = 0;
		double drillDepth = 0.0;
		double presetDepth2 = m_cylinderDepth * m_cylinderDepth;
		for (i = 0; i < focusFacesIntersected.size(); i++)
		{
			int faceIndex = focusFacesIntersected[i];
			bool dirFlag = directionFlagMap[faceIndex];
			if (dirFlag)
				layerFlag = 1;
			else if (layerFlag > 0)
				layerFlag = -1;

			double depth2 = faceDisMap[faceIndex];

			if (m_cylinderDepth > 0)
			{
				if (depth2 < presetDepth2 && layerFlag == -1)
				{
					drillDepth = depth2;
					layerFlag = 0;
				}
				else if (depth2 >= presetDepth2)
					break;
			}
			else if (layerFlag == -1)
			{
				drillDepth = depth2;
				break;
			}
		}

		if (drillDepth > 0)
		{
			drillDepth = sqrt(drillDepth);

			// 剔除距离比打孔深度长的三角面（暂不启用）
			{
				//double offset = 100;
				//double farthestFaceDis = drillDepth;
				//do
				//{
				//	i++;
				//} while (i < meshFocusFacesMapper.size() && fabs(faceDisMap[meshFocusFacesMapper[i]] - farthestFaceDis) < offset);

				//if (i < meshFocusFacesMapper.size())
				//{
				//	for (int j = i; j < meshFocusFacesMapper.size(); j++)
				//	{
				//		totalMeshFlag[meshFocusFacesMapper[j]] = 1;
				//	}

				//	meshFocusFacesMapper.erase(meshFocusFacesMapper.begin() + i, meshFocusFacesMapper.end());
				//}
			}

			meshFocusFaces.clear();
			for (i = 0; i < meshFocusFacesMapper.size(); i++)
			{
				meshFocusFaces.push_back(m_mesh->faces[meshFocusFacesMapper[i]]);
			}
		}

		return drillDepth;
	}

	bool OptimizeCylinderCollide::lineCollideTriangle(trimesh::dvec3 linePos, trimesh::dvec3 lineDir, trimesh::dvec3 A, trimesh::dvec3 B, trimesh::dvec3 C, trimesh::dvec3& intersectedPos)
	{
		trimesh::dvec3 faceNormal = trimesh::normalized((B - A) TRICROSS(C - A));

		double temp = lineDir.x * faceNormal.x + lineDir.y * faceNormal.y + lineDir.z * faceNormal.z;
		if (temp == 0)
			return false;

		double t = ((A.x - linePos.x) * faceNormal.x + (A.y - linePos.y) * faceNormal.y + (A.z - linePos.z) * faceNormal.z) / temp;

		trimesh::dvec3 intersecedPosTemp(lineDir.x * t + linePos.x, lineDir.y * t + linePos.y, lineDir.z * t + linePos.z);

		trimesh::dvec3 v0 = C - A;
		trimesh::dvec3 v1 = B - A;
		trimesh::dvec3 v2 = intersecedPosTemp - A;
		double dot00 = v0 DOT v0;
		double dot01 = v0 DOT v1;
		double dot02 = v0 DOT v2;
		double dot11 = v1 DOT v1;
		double dot12 = v1 DOT v2;

		double bottom = 1.0 / (dot00 * dot11 - dot01 * dot01);
		double u = (dot11 * dot02 - dot01 * dot12) * bottom;
		double v = (dot00 * dot12 - dot01 * dot02) * bottom;
		if (u >= 0 && v >= 0 && u + v <= 1)
		{
			intersectedPos = intersecedPosTemp;
			return true;
		}

		return false;
	}

	bool OptimizeCylinderCollide::valid()
	{
		return focusTriangle > 0;
	}

	trimesh::TriMesh* OptimizeCylinderCollide::drilldrill(ccglobal::Tracer* tracer)
	{
		TriPatch newMeshTriangles;
		generateNewTriangles(meshFocusFaces, m_mesh, focusNormals, meshTris, newMeshTriangles, true, totalMeshFlag, tracer, m_debugger);
		if (tracer)
		{
			tracer->progress(0.2f);
		}
		TriPatch newCylinderTriangles;
		generateNewTriangles(m_cylinder->faces, m_cylinder, cylinderNormals, cylinderTris, newCylinderTriangles, false, cylinderFlag, tracer, nullptr);
		if (tracer)
		{
			tracer->progress(0.4f);
		}
		trimesh::TriMesh* originMesh = getNewOriginMehsh(m_mesh, totalMeshFlag, CylinderCollideOuter, tracer);
		if (tracer)
		{
			tracer->progress(0.6f);
		}
		trimesh::TriMesh* Mout = getNewFocusMehsh(newMeshTriangles);
		if (tracer)
		{
			tracer->progress(0.8f);
		}
		trimesh::TriMesh* Cyin = generateAppendMesh(m_cylinder, cylinderFlag, newCylinderTriangles, CylinderCollideInner, tracer);
		if (tracer)
		{
			tracer->progress(0.9f);
		}

		trimesh::TriMesh* drillMesh = postProcessDrill(Mout, Cyin);

		return generateAppendMeshWithDumplicate(originMesh, meshFocusVertices, drillMesh, tracer);
	}

	trimesh::TriMesh* OptimizeCylinderCollide::drill(ccglobal::Tracer* tracer)
	{
		TriPatch newMeshTriangles;
		generateNewTriangles(meshFocusFaces, m_mesh, focusNormals, meshTris, newMeshTriangles, true, totalMeshFlag, tracer, m_debugger);
		if (tracer)
		{
			tracer->progress(0.4f);
		}
		TriPatch newCylinderTriangles;
		generateNewTriangles(m_cylinder->faces, m_cylinder, cylinderNormals, cylinderTris, newCylinderTriangles, false, cylinderFlag, tracer, nullptr);
		if (tracer)
		{
			tracer->progress(0.6f);
		}

		trimesh::TriMesh* Mout = generateAppendMesh(m_mesh, totalMeshFlag, newMeshTriangles, CylinderCollideOuter, tracer);
		if (tracer)
		{
			tracer->progress(0.8f);
		}

		trimesh::TriMesh* Cyin = generateAppendMesh(m_cylinder, cylinderFlag, newCylinderTriangles, CylinderCollideInner, tracer);
		if (tracer)
		{
			tracer->progress(0.9f);
		}

		return postProcess(Mout, Cyin);
	}

	trimesh::TriMesh* OptimizeCylinderCollide::postProcess(trimesh::TriMesh* Mout, trimesh::TriMesh* Cin)
	{
		if (Cin)
		{
			SAFE_TRACER(m_tracer, "drill reverseTrimesh.");
			msbase::reverseTriMesh(Cin);
		}

		SAFE_TRACER(m_tracer, "drill mergeTriMesh.");

		std::vector<trimesh::TriMesh*> meshes;
		if(Mout)
			meshes.push_back(Mout);
		if(Cin)
			meshes.push_back(Cin);
		trimesh::TriMesh* drillMesh = mergeMeshes(meshes);

		for (trimesh::TriMesh* mesh : meshes)
			delete mesh;
		meshes.clear();

		SAFE_TRACER(m_tracer, "drill dumplicateMesh.");
		dumplicateMesh(drillMesh, nullptr);

		if (m_tracer)
		{
			m_tracer->progress(1.0f);
			m_tracer->success();
		}
		return drillMesh;
	}

	trimesh::TriMesh* OptimizeCylinderCollide::postProcessDrill(trimesh::TriMesh* Mout, trimesh::TriMesh* Cin)
	{
		if (Cin)
		{
			SAFE_TRACER(m_tracer, "drill reverseTrimesh.");
			msbase::reverseTriMesh(Cin);
		}

		SAFE_TRACER(m_tracer, "drill mergeTriMesh.");

		std::vector<trimesh::TriMesh*> meshes;
		if (Mout)
			meshes.push_back(Mout);
		if (Cin)
			meshes.push_back(Cin);
		trimesh::TriMesh* drillMesh = mergeMeshes(meshes);

		for (trimesh::TriMesh* mesh : meshes)
			delete mesh;
		meshes.clear();

		//SAFE_TRACER(m_tracer, "drill dumplicateMesh.");
		//dumplicateMesh(drillMesh, m_tracer);

		if (m_tracer)
			m_tracer->success();
		return drillMesh;
	}

	trimesh::TriMesh* getNewMesh(trimesh::TriMesh* mesh,
		int resolution, double radius, double depth, trimesh::point pointStart, trimesh::point dir,
		ccglobal::Tracer* tracer, DrillDebugger* debugger)
	{
		TriMesh* meshnew = new TriMesh();
		*meshnew = *mesh;

		int faces = (int)meshnew->faces.size();
		//m_cylinder->need_bbox();
		//box3 cylinderBox_old = m_cylinder->bbox;

		trimesh::point ZAXIS(0.0f, 0.0f, 1.0f);
		trimesh::vec3 ax = dir;
		trimesh::quaternion quat = trimesh::quaternion::fromDirection(ax, ZAXIS);

		trimesh::fxform xf = msbase::fromQuaterian(quat);

		meshnew->need_bbox();
		trimesh::point  m_cylinderPointStart = pointStart + 2 * (-dir);
		double cylinderInitDepth = 2 * meshnew->bbox.radius() + 2;

		//box3 cylinderBox_new;
		//int cylinderVertexNum = (int)m_cylinder->vertices.size();
		//for (int i = 0; i < cylinderVertexNum; ++i)
		//{
		//	trimesh::vec3 v = xf * m_cylinder->vertices.at(i);
		//	cylinderBox_new += v;
		//}

		trimesh::vec3 newStartPos = xf * m_cylinderPointStart;
		trimesh::vec3 boxMin = trimesh::vec3(newStartPos.x - radius, newStartPos.y - radius, newStartPos.z);
		trimesh::vec3 boxMax = trimesh::vec3(newStartPos.x + radius, newStartPos.y + radius, newStartPos.z + cylinderInitDepth);
		box3 cylinderBox_new(boxMin, boxMax);

		FacePatch meshFocusFaces;
		for (int i = 0; i < faces; ++i)
		{
			TriMesh::Face& face = meshnew->faces.at(i);
			box3 b;
			for (int j = 0; j < 3; ++j)
			{
				b += xf * meshnew->vertices.at(face[j]);
			}

			if (cylinderBox_new.intersects(b))
			{
				meshFocusFaces.push_back(face);
			}
		}
		meshnew->faces.swap(meshFocusFaces);
		return meshnew;
	}
}