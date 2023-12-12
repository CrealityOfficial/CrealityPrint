#include "polygonstack.h"
#include <stack>
#include <functional>

#include "polygon.h"
#include "polygon2util.h"
#include "savepolygonstack.h"
#include "trimesh2/Vec3Utils.h"
#include <assert.h>

namespace msbase
{
	PolygonStack::PolygonStack()
		: m_currentPolygon(0)
		, m_mregeCount(0)
	{
	}
	
	PolygonStack::~PolygonStack()
	{
		clear();
	}

	void PolygonStack::clear()
	{
		for (Polygon2* poly : m_polygon2s)
			delete poly;
		m_polygon2s.clear();
		m_currentPolygon = 0;
	}

	void PolygonStack::generates(std::vector<std::vector<int>>& polygons, std::vector<trimesh::dvec2>& points, std::vector<trimesh::TriMesh::Face>& triangles, int layer)
	{
#if 0
		static int i = 0;
		char buffer[128];
		sprintf(buffer, "poly/%d_%d.poly", layer, i++);
		stackSave(buffer, polygons, points);
#endif
		prepare(polygons, points);
		generate(triangles);
	}

	void PolygonStack::generatesWithoutTree(std::vector<std::vector<int>>& polygons, std::vector<trimesh::dvec2>& points, std::vector<trimesh::TriMesh::Face>& triangles)
	{
		prepareWithoutTree(polygons, points);
		generate(triangles);
	}

	void PolygonStack::prepareWithoutTree(std::vector<std::vector<int>>& polygons, std::vector<trimesh::dvec2>& points)
	{
		size_t size = polygons.size();
		if (size > 0)
		{
			for (size_t i = 0; i < size; ++i)
			{
				std::vector<int>& polygon = polygons.at(i);
				if (polygon.size() > 0 && (polygon.at(0) != polygon.back()))
				{// loop
					polygon.push_back(polygon.at(0));
				}

				if (polygon.size() > 0)
				{
					Polygon2* poly = new Polygon2();
					poly->setup(polygon, points);
					m_polygon2s.push_back(poly);
				}
			}
		}
	}

	void PolygonStack::prepare(std::vector<std::vector<int>>& polygons, std::vector<trimesh::dvec2>& points)
	{
		size_t size = polygons.size();
		if (size > 0)
		{
			// build simple polygon
			struct SimpleInfo
			{
				double area;
				trimesh::dbox2 box;
			};
			std::list<int> candidates;
			std::vector<SimpleInfo> infos(size);
			for (size_t i = 0; i < size; ++i)
			{
				std::vector<int>& polygon = polygons.at(i);
				if (polygon.size() > 0 && (polygon.at(0) != polygon.back()))
				{// loop
					polygon.push_back(polygon.at(0));
				}

				size_t polygonSize = polygon.size();
				SimpleInfo& info = infos.at(i);
				candidates.push_back((int)i);
				info.area = 0.0;
				if (polygonSize > 1)
				{
					info.box += points.at(polygon.at(0));
					for (size_t j = 1; j < polygonSize; ++j)
					{
						trimesh::dvec2& v1 = points.at(polygon.at(j));
						trimesh::dvec2& v2 = points.at(polygon.at(j - 1));

						info.area += 0.5 * crossValue(v2, v1);
						info.box += v1;
					}
				}
			}

			candidates.sort([&infos](int i1, int i2)->bool {
					return abs(infos.at(i1).area) > abs(infos.at(i2).area);
					});

			struct TreeNode
			{
				int index;
				bool outer;
				std::list<int> temp;
				std::vector<TreeNode> children;
			};
			TreeNode rootNode;
			rootNode.outer = false;
			rootNode.index = -1;
			rootNode.temp.swap(candidates);
			std::vector<std::vector<int>> simplePolygons(size);

			std::function<void(TreeNode& node)> split;
			std::function<void(TreeNode& node)> merge;
			std::function<bool(int c, int t)> testin;
			testin = [&infos, &polygons, &points](int c, int t) ->bool{
				trimesh::dbox2 cb = infos.at(c).box;
				trimesh::dbox2 tb = infos.at(t).box;
				if(!cb.contains(tb))
					return false;

				int tindex = polygons.at(t).at(0);
				trimesh::dvec2& tvertex = points.at(tindex);
				std::vector<int>& cpolygon = polygons.at(c);
				int nvert = (int)cpolygon.size();
				int i, j, count = 0;
				for (i = 0, j = nvert - 1; i < nvert; j = i++)
				{
					trimesh::dvec2& verti = points.at(cpolygon.at(i));
					trimesh::dvec2& vertj = points.at(cpolygon.at(j));
					if (((verti.y > tvertex.y) != (vertj.y > tvertex.y)) &&
						(tvertex.x < (vertj.x - verti.x) * (tvertex.y - verti.y) / (vertj.y - verti.y) + verti.x))
						count = !count;
				}
				//return count != 0 && (infos.at(c).area * infos.at(t).area < 0.0f);
				return count != 0;
			};
			merge = [&merge, &polygons, &simplePolygons, &infos, &points, this](TreeNode& node) {
				int index = node.index;
				if (index >= 0 && infos.at(index).area > 0.0)
				{
					simplePolygons.at(index) = polygons.at(index);
					std::vector<int>& outerPolygon = simplePolygons.at(index);
					if(node.children.size() > 0)
					{
						std::vector<int> indices;
						for (TreeNode& n : node.children)
							indices.push_back(n.index);

						std::sort(indices.begin(), indices.end(), [&infos](int i, int j)->bool {
							return infos.at(i).box.max.x < infos.at(j).box.max.x;
							});

						const double EPSON = 0.00000001;

						while (indices.size() > m_mregeCount)
						{
#if _DEBUG
#endif
							int polygonIndex = indices.back();
							indices.pop_back();

							//// merge polygonIndex and index
							std::vector<int>& innerPolygon = polygons.at(polygonIndex);
							int innerSize = (int)innerPolygon.size();
							float mx = -10000000.0f;
							int vertexIndex = -1;
							for (int i = 0; i < innerSize; ++i)
							{
								trimesh::dvec2& v = points.at(innerPolygon.at(i));
								if (v.x > mx)
								{
									mx = v.x;
									vertexIndex = i;
								}
							}

							if (vertexIndex >= 0)
							{
								trimesh::dvec2& tvertex = points.at(innerPolygon.at(vertexIndex));
								int nvert = (int)outerPolygon.size();
								int i, j = 0;
								double cmx = 1000000.0;
								int cOuterIndex = -1;
								int cOuterIndex0 = -1;
								for (i = 0, j = nvert - 1; i < nvert; j = i++)
								{
									trimesh::dvec2& verti = points.at(outerPolygon.at(i));
									trimesh::dvec2& vertj = points.at(outerPolygon.at(j));
									if ((verti.y == tvertex.y) && (vertj.y == tvertex.y))
									{
										double mmx = verti.x > vertj.x ? vertj.x : verti.x;
										if (mmx > tvertex.x && mmx < cmx)
										{
											cOuterIndex = i;
											cOuterIndex0 = j;
											cmx = mmx;
										}
									}
									else if ((verti.y > tvertex.y) != (vertj.y > tvertex.y))
									{
										trimesh::dvec2 start = verti;
										trimesh::dvec2 end = vertj;
										if (outerPolygon.at(i) > outerPolygon.at(j))
											std::swap(start, end);

										double cx = (end.x - start.x)* (tvertex.y - start.y) / (end.y - start.y) + start.x;
										if (cx >= tvertex.x)  // must 
										{
											if (cx == cmx)
											{  // collide two opposite edge
												if (verti.y > vertj.y)
												{
													cOuterIndex = i;
													cOuterIndex0 = j;
													cmx = cx;
												}
											}else if(cx < cmx)
											{
												cOuterIndex = i;
												cOuterIndex0 = j;
												cmx = cx;
											}
										}
									}
								}

								int mutaulIndex = -1;
								if (cOuterIndex >= 0)
								{
#if _DEBUG
									MergeInfo info;
									info.start = outerPolygon.at(cOuterIndex0);
									info.end = outerPolygon.at(cOuterIndex);
									info.innerIndex = innerPolygon.at(vertexIndex);
#endif
									bool findUnique = false;
									if ((cmx == points.at(outerPolygon.at(cOuterIndex)).x)
										&& (tvertex.y == points.at(outerPolygon.at(cOuterIndex)).y))
									{
										mutaulIndex = cOuterIndex;
										findUnique = true;
									}
									else if ((cmx == points.at(outerPolygon.at(cOuterIndex0)).x)
										&& (tvertex.y == points.at(outerPolygon.at(cOuterIndex0)).y))
									{
										mutaulIndex = cOuterIndex0;
										findUnique = true;
									}
									else
									{
										trimesh::dvec2 M = tvertex;
										trimesh::dvec2 I = trimesh::dvec2(cmx, M.y);
										trimesh::dvec2 P0 = points.at(outerPolygon.at(cOuterIndex0));
										trimesh::dvec2 P = points.at(outerPolygon.at(cOuterIndex));
										bool useUpper = true;
										if (P.x < P0.x)
										{
											useUpper = false;
											P = P0;
										}
										if(P.y > I.y)
											std::swap(P, I);

										std::vector<int> reflexVertex;
										for (i = 0; i < nvert; ++i)
										{
											trimesh::dvec2& tv = points.at(outerPolygon.at(i));
											if (insideTriangle(M, P, I, tv))
											{
												reflexVertex.push_back(i);
											}
										}

										if (reflexVertex.size() == 0)
										{
											mutaulIndex = useUpper ? cOuterIndex : cOuterIndex0;
										}
										else
										{
											int reflexSize = reflexVertex.size();
											double minLen = 1000000.0;
											double maxDot = -10000.0;
											int minReflexIndex = 0;
											std::vector<int> minRefs;
											for (i = 0; i < reflexSize; ++i)
											{
												trimesh::dvec2 R = points.at(outerPolygon.at(reflexVertex.at(i)));
												trimesh::dvec2 MR = R - M;

												double len = trimesh::len(MR);
												trimesh::normalize(MR);
												double dot = abs(dotValue(MR, trimesh::dvec2(1.0, 0.0)));
												if (dot > maxDot || (dot == maxDot && len < minLen))
												{
													minReflexIndex = i;
													minLen = len;
													maxDot = dot;
													minRefs.clear();
													minRefs.push_back(i);
												}
												else if (dot == maxDot && len == minLen)
												{//¹²µã
													minRefs.push_back(i);
												}
											}

											if (minRefs.size() > 1)
											{// 
												double A = - 4.0 * M_PI;
												for (int z = 0; z < minRefs.size(); ++z)
												{
													int iii = minRefs.at(z);
													trimesh::dvec2 R = points.at(outerPolygon.at(reflexVertex.at(iii)));
													trimesh::dvec2 RM = M - R;
													trimesh::normalize(RM);

													int nextIndex = outerPolygon.at((reflexVertex.at(iii) + 1) % nvert);
													trimesh::dvec2 V = points.at(nextIndex);
													trimesh::dvec2 RV = V - R;
													trimesh::normalize(RV);
													double a = angle(RM, RV);
													if (a > A)
													{
														A = a;
														minReflexIndex = iii;
													}
												}
											}

											mutaulIndex = reflexVertex.at(minReflexIndex);
										}

#if _DEBUG
										info.matual = outerPolygon.at(mutaulIndex);
										m_mergeInfo.push_back(info);
#endif 
									}

									if (findUnique)
									{
										int uIndex = outerPolygon.at(mutaulIndex);
										std::vector<int> minRefs;
										for (int oIndex = 0; oIndex < nvert; ++oIndex)
											if (outerPolygon.at(oIndex) == uIndex)
												minRefs.push_back(oIndex);

										if (minRefs.size() > 1)
										{
											trimesh::dvec2 M = tvertex;
											int minReflexIndex = -1;
											double A = -4.0 * M_PI;
											for (int z = 0; z < minRefs.size(); ++z)
											{
												int iii = minRefs.at(z);
												trimesh::dvec2 R = points.at(outerPolygon.at(iii));
												trimesh::dvec2 RM = M - R;
												trimesh::normalize(RM);

												int nextIndex = outerPolygon.at((iii + 1) % nvert);
												trimesh::dvec2 V = points.at(nextIndex);
												trimesh::dvec2 RV = V - R;
												trimesh::normalize(RV);
												double a = angle(RM, RV);
												if (a > A)
												{
													A = a;
													minReflexIndex = iii;
												}
											}

											if(minReflexIndex >= 0)
												mutaulIndex = minReflexIndex;
										}
									}
								}

								if (mutaulIndex >= 0)
								{// merge mutaulIndex in outer and vertexIndex in inner
									std::vector<int> mergedPolygon;
#if _DEBUG
#endif
									for (i = 0; i < nvert; ++i) 
									{
										mergedPolygon.push_back(outerPolygon.at(i));
										if (i == mutaulIndex)
										{// insert inner
											for(j = vertexIndex; j < innerSize - 1; ++j)
												mergedPolygon.push_back(innerPolygon.at(j));
											for(j = 0; j <= vertexIndex; ++j)
												mergedPolygon.push_back(innerPolygon.at(j));
											mergedPolygon.push_back(outerPolygon.at(i));
										}
									}
									outerPolygon.swap(mergedPolygon);
								}
							}
						}
					}
				}

				for (TreeNode& cNode : node.children)
				{
					merge(cNode);
				}
			};
			split = [&split, &testin, &infos](TreeNode& node) {
				std::list<int>& candidates = node.temp;
				while (candidates.size() > 0)
				{
					int index = candidates.front();
					candidates.pop_front();

					if ((!node.outer && infos.at(index).area > 0.0) || (node.outer && infos.at(index).area < 0.0))
					{
						TreeNode rootNode;
						rootNode.outer = !node.outer;
						rootNode.index = index;

						std::vector<int> added;
						for (std::list<int>::iterator it = candidates.begin(); it != candidates.end();)
						{
							std::list<int>::iterator c = it;
							++it;
							if (testin(index, *c))
							{
								bool inAdded = false;
								for (int addedIndex : added)
								{
									if (testin(addedIndex, *c))
									{
										inAdded = true;
										break;
									}
								}
								if (!inAdded)  //test *c collide in index
								{
									added.push_back(*c);
								}

								rootNode.temp.push_back(*c);

								candidates.erase(c);
							}
						}
						node.children.push_back(rootNode);
					}
				}

				for (TreeNode& cNode : node.children)
				{
					split(cNode);
				}
			};

			split(rootNode);
			merge(rootNode);

			for (size_t i = 0; i < size; ++i)
			{
				if (simplePolygons.at(i).size() > 0)
				{
					Polygon2* poly = new Polygon2();
					poly->setup(simplePolygons.at(i), points);
					m_polygon2s.push_back(poly);
				}
			}
		}
	}

	void PolygonStack::generate(std::vector<trimesh::TriMesh::Face>& triangles)
	{
		for (Polygon2* poly : m_polygon2s)
		{
			poly->earClipping(triangles);
		}
	}

	bool PolygonStack::earClipping(trimesh::TriMesh::Face& face, std::vector<int>* earIndices)
	{
		if (m_currentPolygon >= (int)m_polygon2s.size() || m_currentPolygon < 0)
			return false;

#if 0
		if (m_currentPolygon >= 1)
			return false;
#endif

		Polygon2* poly = m_polygon2s.at(m_currentPolygon);
		if (!poly->earClipping(face, earIndices))
		{
			++m_currentPolygon;
		}

		return true;
	}

	void PolygonStack::getEars(std::vector<int>* earIndices)
	{
		Polygon2* poly = nullptr;
		if (m_currentPolygon >= 0 && m_currentPolygon < (int)m_polygon2s.size())
		{
			poly = m_polygon2s.at(m_currentPolygon);
		}
		if (poly) poly->getEars(earIndices);
	}

	Polygon2* PolygonStack::validIndexPolygon(int index)
	{
		if (index >= 0 && index < (int)m_polygon2s.size())
		{
			return m_polygon2s.at(index);
		}
		return nullptr;
	}

	int PolygonStack::validPolygon()
	{
		return (int)m_polygon2s.size();
	}

	std::vector<MergeInfo> PolygonStack::mergeInfo()
	{
		return m_mergeInfo;
	}

	void PolygonStack::setMergeCount(int count)
	{
		m_mregeCount = count;
	}

	void transform3to2(std::vector<trimesh::dvec3>& d3points, const trimesh::dvec3& normal, std::vector<trimesh::dvec2>& d2points)
	{
		int size = (int)d3points.size();
		if (size <= 0)
			return;

		d2points.resize(size);
		trimesh::dvec3 zn = trimesh::dvec3(0.0, 0.0, 1.0);

		trimesh::vec3 znn = trimesh::vec3(0.0, 0.0, 1.0);
		trimesh::vec3 nnn(normal.x, normal.y, normal.z);
		double angle = trimesh::vv_angle(nnn, znn);
		trimesh::dvec3 axis = normal TRICROSS zn;
		if (angle >= 3.141592f)
			axis = trimesh::vec3(1.0f, 0.0f, 0.0f);

		trimesh::xform r = trimesh::xform::rot((double)angle, axis);
		for (size_t i = 0; i < d3points.size(); ++i)
		{
			trimesh::dvec3 v = d3points.at(i);
			trimesh::dvec3 dv = trimesh::dvec3(v.x, v.y, v.z);
			trimesh::dvec3 p = r * dv;
			d2points.at(i) = trimesh::dvec2(p.x, p.y);
		}
	}

	void generateTriangleSoup(std::vector<trimesh::dvec3>& points, const trimesh::dvec3& normal, std::vector<std::vector<int>>& polygons,
		std::vector<trimesh::vec3>& newTriangles)
	{
		std::vector<trimesh::dvec2> d2points;
		transform3to2(points, normal, d2points);

		msbase::PolygonStack pstack;
		std::vector<trimesh::TriMesh::Face> faces;
		pstack.generates(polygons, d2points, faces, 0);

		for (int j = 0; j < (int)faces.size(); ++j)
		{
			trimesh::TriMesh::Face& face = faces.at(j);

			for (int k = 0; k < 3; ++k)
			{
				trimesh::dvec3& p = points.at(face[k]);
				newTriangles.push_back(trimesh::vec3((float)p.x, (float)p.y, (float)p.z));
			}
		}
	}

	void generateTriangleSoup(std::vector<trimesh::dvec3>& points, std::vector<trimesh::dvec2>& d2points, std::vector<std::vector<int>>& polygons,
		std::vector<trimesh::vec3>& newTriangles)
	{
		msbase::PolygonStack pstack;
		std::vector<trimesh::TriMesh::Face> faces;
		pstack.generates(polygons, d2points, faces, 0);

		for (int j = 0; j < (int)faces.size(); ++j)
		{
			trimesh::TriMesh::Face& face = faces.at(j);

			for (int k = 0; k < 3; ++k)
			{
				trimesh::dvec3& p = points.at(face[k]);
				newTriangles.push_back(trimesh::vec3((float)p.x, (float)p.y, (float)p.z));
			}
		}
	}
}