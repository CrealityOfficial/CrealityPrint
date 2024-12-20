#include "qhullWrapper/hull/meshconvex.h"
#include "libqhullcpp/Qhull.h"
#include "libqhullcpp/QhullFacetList.h"
#include "libqhullcpp/QhullVertex.h"
#include "libqhullcpp/QhullVertexSet.h"

#include "trimesh2/TriMesh.h"
#include "ccglobal/tracer.h"
#include "ccglobal/log.h"

using namespace orgQhull;
namespace qhullWrapper
{
	void debug_print_qhull(Qhull& hull)
	{
#if _DEBUG
		hull.outputQhull();
		std::string message = hull.qhullMessage();
		LOGD("debug_print_qhull: %s", message.c_str());
#endif
	}

	trimesh::point calTriNormal(trimesh::point ver1, trimesh::point ver2, trimesh::point ver3)
	{
		double temp1[3], temp2[3], normal[3];
		double length = 0.0;
		temp1[0] = ver2[0] - ver1[0];
		temp1[1] = ver2[1] - ver1[1];
		temp1[2] = ver2[2] - ver1[2];
		temp2[0] = ver3[0] - ver2[0];
		temp2[1] = ver3[1] - ver2[1];
		temp2[2] = ver3[2] - ver2[2];
		//计算法线
		normal[0] = temp1[1] * temp2[2] - temp1[2] * temp2[1];
		normal[1] = -(temp1[0] * temp2[2] - temp1[2] * temp2[0]);
		normal[2] = temp1[0] * temp2[1] - temp1[1] * temp2[0];
		//法线单位化
		length = sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
		if (length == 0.0f) { length = 1.0f; }
		normal[0] /= length;
		normal[1] /= length;
		normal[2] /= length;
		trimesh::point e_normal(normal[0], normal[1], normal[2]);
		return e_normal;
	}

	void calculateConvex(trimesh::TriMesh* mesh, ccglobal::Tracer* tracer)
	{
		if (!mesh || mesh->vertices.size() == 0)
			return;

		size_t vertexSize = mesh->vertices.size();
		std::vector<bool> added(vertexSize, false);
		try
		{
			Qhull q("", 3, vertexSize, (float*)mesh->vertices.data(), "");
			QhullFacetList facets = q.facetList();

			mesh->flags.clear();
			auto fadd = [&mesh, &added](int id) {
				if (!added.at(id))
				{
					mesh->flags.push_back(id);
					added.at(id) = true;
				}
			};
			for (QhullFacet f : facets)
			{
				if (!f.isGood())
				{
					// ignore facet
				}
				else if (!f.isTopOrient() && f.isSimplicial())
				{ /* orient the vertices like option 'o' */
					QhullVertexSet vs = f.vertices();
					fadd(vs[1].point().id());
					fadd(vs[0].point().id());
					for (int i = 2; i < (int)vs.size(); ++i)
					{
						fadd(vs[i].point().id());
					}
				}
				else
				{  /* note: for non-simplicial facets, this code does not duplicate option 'o', see qh_facet3vertex and qh_printfacetNvertex_nonsimplicial */
					for (QhullVertex vertex : f.vertices())
					{
						QhullPoint p = vertex.point();
						fadd(p.id());
					}
				}
			}
		}
		catch (QhullError& e) {
		}
	}

	trimesh::TriMesh* convex_hull_3d(trimesh::TriMesh* inMesh)
	{
		orgQhull::Qhull qhull;
		qhull.disableOutputStream(); // we want qhull to be quiet
		std::vector<realT> src_vertices;

		if (inMesh->vertices.size() == 0)
			return nullptr;

		trimesh::TriMesh* outMesh = new trimesh::TriMesh();
		{
			qhull.runQhull("", 3, (int)inMesh->vertices.size(), (const realT*)(inMesh->vertices.front().data()), "Qt");
		}

		// Let's collect results:
		auto facet_list = qhull.facetList().toStdVector();
		for (const orgQhull::QhullFacet& facet : facet_list)// iterate through facets
		{
			orgQhull::QhullVertexSet vertices = facet.vertices();
			for (int i = 0; i < 3; ++i)// iterate through facet's vertices
			{
				orgQhull::QhullPoint p = vertices[i].point();
				const auto* coords = p.coordinates();
				outMesh->vertices.emplace_back(coords[0], coords[1], coords[2]);
			}

			unsigned int size = (unsigned int)outMesh->vertices.size();

			//the normal may be null
			if (facet.getFacetT()->normal != nullptr)
			{
				auto Qhullcoord = facet.hyperplane().coordinates();
				trimesh::vec a(Qhullcoord[0], Qhullcoord[1], Qhullcoord[2]);				
				trimesh::vec b = calTriNormal(outMesh->vertices[size - 3], outMesh->vertices[size - 2], outMesh->vertices[size - 1]);
				float d = trimesh::dot(a, b);

				if (d > 0)
				{
					outMesh->faces.emplace_back(size - 3, size - 2, size - 1);
				}
				else
				{
					outMesh->faces.emplace_back(size - 1, size - 2, size - 3);
				}
			}
			//assume the orientation of face is determine by the order of the point sequence when it's normal is null
			else
			{
				outMesh->faces.emplace_back(size - 3, size - 2, size - 1);
			}
		}
		return outMesh;
	}

	trimesh::TriMesh* convex_hull_2d(trimesh::TriMesh* inMesh)
	{
		if (inMesh->vertices.size() < 3)
		{
			return nullptr;
		}

		trimesh::TriMesh* outMesh = new trimesh::TriMesh();
		std::sort(inMesh->vertices.begin(), inMesh->vertices.end());

		outMesh->vertices.resize(3 * inMesh->vertices.size());

		// Build lower hull
		int hullNum = 0;
		for (int i = 0; i < inMesh->vertices.size(); ++i)
		{
			trimesh::point currentPoint = inMesh->vertices[i];
			while (hullNum >= 2)
			{
				trimesh::point hull_1 = outMesh->vertices[hullNum - 1];
				trimesh::point hull_2 = outMesh->vertices[hullNum - 2];

				double num = (double)(hull_2.x - hull_1.x) * (double)(currentPoint.y - hull_1.y) -
					(double)(hull_2.y - hull_1.y) * (double)(currentPoint.x - hull_1.x);
				if (num <= 0)
				{
					--hullNum;
				}
				else
				{
					break;
				}
			}
			outMesh->vertices[hullNum++] = inMesh->vertices[i];
		}

		// Build upper hull
		for (int i = inMesh->vertices.size() - 2, t = hullNum + 1; i >= 0; --i)
		{
			trimesh::point currentPoint = inMesh->vertices[i];
			while (hullNum >= t)
			{
				trimesh::point hull_1 = outMesh->vertices[hullNum - 1];
				trimesh::point hull_2 = outMesh->vertices[hullNum - 2];

				double num = (double)(hull_2.x - hull_1.x) * (double)(currentPoint.y - hull_1.y) -
					(double)(hull_2.y - hull_1.y) * (double)(currentPoint.x - hull_1.x);
				if (num <= 0)
				{
					--hullNum;
				}
				else
				{
					break;
				}
			}
			outMesh->vertices[hullNum++] = inMesh->vertices[i];
		}

		outMesh->vertices.resize(hullNum);
		if (outMesh->vertices.front() == outMesh->vertices.back())
		{
			outMesh->vertices.pop_back();
		}
		return outMesh;
	}

	trimesh::TriMesh* convex2DPolygon(const float* vertex, int count)
	{
		if (!vertex || (count <= 0))
			return nullptr;

		trimesh::TriMesh* mesh = new trimesh::TriMesh();
		using namespace orgQhull;

		struct line
		{
		    int start;
		    int end;
		};
		
		auto fvertex = [&vertex](int index)->trimesh::vec3 {
			return trimesh::vec3(*(vertex + 2 * index),
				*(vertex + 2 * index + 1), 0.0f);
		};

		try
		{
		    Qhull q("", 2, count, vertex, "");
			debug_print_qhull(q);

#if 0
			QhullFacetList facets = q.facetList();
			for (QhullFacet f : facets)
			{
				if (!f.isGood())
					continue;

				QhullVertexSet vs = f.vertices();

				line l;
				l.start = vs[1].point().id();
				l.end = vs[0].point().id();
				fadd(l);
			}
#else
		    QhullFacetList facets = q.facetList();
		
			std::vector<line> polygons;
		    auto fadd = [&polygons](const line& l) {
		        polygons.push_back(l);
		    };
		    for (QhullFacet f : facets)
		    {
		        if (!f.isGood())
		        {
		            // ignore facet
		        }
		        else if (!f.isTopOrient() && f.isSimplicial())
		        { /* orient the vertices like option 'o' */
		            QhullVertexSet vs = f.vertices();
		
		            line l;
		            l.start = vs[1].point().id();
		            l.end = vs[0].point().id();
		            fadd(l);
		        }
		        else
		        {  /* note: for non-simplicial facets, this code does not duplicate option 'o', see qh_facet3vertex and qh_printfacetNvertex_nonsimplicial */
		
		            int c = 0;
		            line l;
		            for (QhullVertex vertex : f.vertices())
		            {
		                QhullPoint p = vertex.point();
		                if (c == 0)
		                    l.start = p.id();
		                else if (c == 1)
		                    l.end = p.id();
		                ++c;
		            }
		
		            if(c == 2)
		                fadd(l);
		        }
		    }
		
			std::sort(polygons.begin(), polygons.end(), [](const line& l1, const line& l2) {
				return l1.start < l2.start;
				});

			std::vector<int> result;
			std::vector<bool> visited;
			int size = (int)polygons.size();
			if (size > 0)
			{
				visited.resize(size, false);
				line findLine;

				int index = -1;
				while (true)
				{
					if (index < 0)
					{
						findLine = polygons.at(0);
						index = findLine.end;
						visited.at(0) = true;
					}
					else
					{
						int findIndex = -1;
						bool revert = false;
						for (int i = 0; i < size; ++i)
						{
							if (visited.at(i) == false)
							{
								if (index == polygons.at(i).start)
								{
									findIndex = i;
									break;
								}
								if (index == polygons.at(i).end)
								{
									findIndex = i;
									revert = true;
									break;
								}
							}
						}
						if (findIndex == -1)
							break;

						findLine = polygons.at(findIndex);
						if (revert)
							std::swap(findLine.start, findLine.end);

						index = findLine.end;
						visited.at(findIndex) = true;
					}

					result.push_back(findLine.start);
				}
			}

		    for (int l : result)
		    {
		        mesh->vertices.push_back(fvertex(l));
		    }
#endif

		}
		catch (QhullError& e) {
			delete mesh;
			mesh = nullptr;
		}

		return mesh;
	}

	trimesh::TriMesh* convex2DPolygon(trimesh::TriMesh* mesh, const trimesh::fxform& xf, ccglobal::Tracer* tracer)
	{
		if (!mesh)
			return nullptr;

		if (mesh->flags.size() == 0)
			calculateConvex(mesh, tracer);

		if (mesh->flags.size() == 0)
			return nullptr;

		int size = (int)mesh->flags.size();

		std::vector<trimesh::vec2> hullPoints2D;
		hullPoints2D.resize(size);
		for (int i = 0; i < size; ++i)
		{
			trimesh::vec3 v = xf * mesh->vertices.at(mesh->flags.at(i));

			hullPoints2D[i] = trimesh::vec2(v.x, v.y);
		}

		return convex2DPolygon((float*)hullPoints2D.data(), size);
	}
}