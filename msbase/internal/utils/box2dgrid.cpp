#include "msbase/utils/box2dgrid.h"
#include "msbase/space/intersect.h"

#include <unordered_map>
#include <float.h>
#include <stack>
#include <math.h>

using namespace trimesh;

namespace msbase
{
	Box2DGrid::Box2DGrid()
		:m_mesh(nullptr)
		, m_gridSize(2.0f)
		, m_width(0)
		, m_height(0)
		, m_globalBuilded(false)
	{
	}

	Box2DGrid::~Box2DGrid()
	{
	}

	void Box2DGrid::build(TriMesh* mesh, fxform xf, bool isFanZhuan)
	{
		m_mesh = mesh;
		m_xf = xf;

		if (m_mesh)
		{
			int vertexNum = (int)m_mesh->vertices.size();
			int faceNum = (int)m_mesh->faces.size();

			if (vertexNum && faceNum)
			{
				m_vertexes.resize(vertexNum);
				for (int i = 0; i < vertexNum; ++i)
				{
					m_vertexes.at(i) = m_xf * m_mesh->vertices.at(i);
				}

				m_dotValues.resize(faceNum);
				m_faceNormals.resize(faceNum);
				for (int i = 0; i < faceNum; ++i)
				{
					TriMesh::Face& f = m_mesh->faces.at(i);

					int fv1 = 1, fv2 = 2;
					if (isFanZhuan)
					{
						fv1 = 2;
						fv2 = 1;
					}

					vec3& v0 = m_vertexes.at(f[0]);
					vec3& v1 = m_vertexes.at(f[fv1]);
					vec3& v2 = m_vertexes.at(f[fv2]);

					vec3 v01 = v1 - v0;
					vec3 v02 = v2 - v0;
					vec3 n = v01 TRICROSS v02;
					normalize(n);

					m_faceNormals.at(i) = n;
					m_dotValues.at(i) = trimesh::dot(n, vec3(0.0f, 0.0f, 1.0f));

					//if (m_logCallback)
					//{
					//	m_logCallback->log("faceNormal %f %f %f", n.x, n.y, n.z);
					//	m_logCallback->log("%d dotValue %f", i, m_dotValues.at(i));
					//}
				}

				m_boxes.resize(faceNum);
				m_indexes.resize(faceNum);
				m_depths.resize(faceNum, vec2(FLT_MAX, -FLT_MAX));
				for (int i = 0; i < faceNum; ++i)
				{
					TriMesh::Face& f = m_mesh->faces.at(i);
					box2& b = m_boxes.at(i);
					vec2& depth = m_depths.at(i);
					for (int j = 0; j < 3; ++j)
					{
						vec3& v = m_vertexes.at(f[j]);
						b += vec2(v.x, v.y);

						if (v.z <= depth.x) depth.x = v.z;
						if (v.z >= depth.y) depth.y = v.z;
					}

					m_globalBox += b;
				}

				for (int i = 0; i < faceNum; ++i)
				{
					box2& b = m_boxes.at(i);

					ivec2 imin = index(b.min);
					ivec2 imax = index(b.max);
					m_indexes.at(i) = ivec4(imin.x, imin.y, imax.x, imax.y);
				}

				vec2 globalSize = m_globalBox.size();
				m_width = (int)(ceilf(globalSize.x / m_gridSize) + 1);
				m_height = (int)(ceilf(globalSize.y / m_gridSize) + 1);

				if (m_width > 0 && m_height > 0)
					m_cells.resize(m_width * m_height);

				for (size_t i = 0; i < faceNum; ++i)
				{
					ivec4& idx = m_indexes.at(i);
					for (int x = idx[0]; x <= idx[2]; ++x)
					{
						for (int y = idx[1]; y <= idx[3]; ++y)
						{
							int iindex = x + y * m_width;
							m_cells.at(iindex).push_back((int)i);
						}
					}
				}
			}
		}
	}

	void Box2DGrid::buildGlobalProperties()
	{
		if (m_mesh && !m_globalBuilded)
		{
			int vertexNum = (int)m_mesh->vertices.size();
			int faceNum = (int)m_mesh->faces.size();
			//build topo
			if (vertexNum) m_outHalfEdges.resize(vertexNum);
			if (faceNum) m_oppositeHalfEdges.resize(faceNum, ivec3(-1, -1, -1));
			for (int i = 0; i < faceNum; ++i)
			{
				TriMesh::Face& f = m_mesh->faces.at(i);
				bool old[3] = { false };
				for (int j = 0; j < 3; ++j)
				{
					std::vector<int>& outHalfs = m_outHalfEdges.at(f[j]);
					old[j] = outHalfs.size() > 0;
				}

				ivec3& halfnew = m_oppositeHalfEdges.at(i);
				for (int j = 0; j < 3; ++j)
				{
					int start = j;
					int end = (j + 1) % 3;

					int v2 = f[start];
					int v1 = f[end];

					std::vector<int>& halfs1 = m_outHalfEdges.at(v1);

					int nh = halfcode(i, j);
					if (old[start] && old[end])
					{
						int hsize = (int)halfs1.size();
						for (int k = 0; k < hsize; ++k)
						{
							int h = halfs1.at(k);
							if (endvertexid(h) == v2)
							{
								halfnew.at(j) = h;

								int faceid = 0;
								int idxid = 0;
								halfdecode(h, faceid, idxid);
								m_oppositeHalfEdges.at(faceid)[idxid] = nh;
								break;
							}
						}
					}
				}

				for (int j = 0; j < 3; ++j)
				{
					std::vector<int>& outHalfs = m_outHalfEdges.at(f[j]);
					outHalfs.push_back(halfcode(i, j));
				}
			}
			m_globalBuilded = true;
		}
	}

	void Box2DGrid::clear()
	{
		m_boxes.clear();
		m_depths.clear();
		m_indexes.clear();

		m_cells.clear();

		m_vertexes.clear();
		m_outHalfEdges.clear();
		m_oppositeHalfEdges.clear();
	}

	bool Box2DGrid::contain(vec2 xy)
	{
		if (m_globalBox.valid) return m_globalBox.contains(xy);
		return false;
	}

	void Box2DGrid::checkDown(std::vector<CollideResult>& outCollides, trimesh::vec3& c)
	{
		std::vector<VerticalCollide> collides;
		check(collides, c, CheckDir::eDown, -1, false);

		std::sort(collides.begin(), collides.end(), [](VerticalCollide& r1, VerticalCollide& r2)->bool {
			return r1.z < r2.z;
			});

		for (VerticalCollide& collide : collides)
		{
			CollideResult cr;
			cr.c = vec3(collide.xy.x, collide.xy.y, collide.z);
			cr.n = m_faceNormals.at(collide.faceid);
			outCollides.push_back(cr);
		}
	}

	void Box2DGrid::autoCheckDown(DLPSource& source, DLPResult& result, float len)
	{
		std::vector<DLPSource> sources;
		sources.push_back(source);
		std::vector<DLPResult> results;
		autoCheckDown(sources, results, len);
		result = results.at(0);
	}

	void Box2DGrid::autoDLPCheckDown(std::vector<DLPResult>& results, float len)
	{
		std::vector<DLPSource> sources;
		dlpSource(sources);
		autoCheckDown(sources, results, len);
	}

	void Box2DGrid::dlpSource(std::vector<DLPSource>& sources)
	{
		GenSource source;
		genSource(source);

		addVertexSources(sources, source);
		addEdgeSources(sources, source);
		addFaceSources(sources, source);
	}

	void Box2DGrid::autoCheckDown(std::vector<DLPSource>& sources, std::vector<DLPResult>& results, float len)
	{
		size_t size = sources.size();
		if (size > 0) results.resize(size);
		for (size_t i = 0; i < size; ++i)
		{
			DLPSource& source = sources.at(i);
			DLPResult& result = results.at(i);

			trimesh::vec3 top = source.position;
			trimesh::vec3 middle = top + len * source.normal;
			if (middle.z <= 1.0f) middle = top;

			std::vector<VerticalCollide> collides;
			check(collides, middle, CheckDir::eDown, -1, false);

			std::sort(collides.begin(), collides.end(), [](VerticalCollide& r1, VerticalCollide& r2)->bool {
				return r1.z < r2.z;
				});

			if (collides.size() == 0)
			{
				trimesh::vec3 bottom(middle.x, middle.y, 0.0f);
				result.bottom = bottom;
				result.top = top;
				result.middle = middle;

				result.platform = true;
			}
			else
			{
				VerticalCollide& topResult = collides.back();
				trimesh::vec3 bottom = trimesh::vec3(topResult.xy.x, topResult.xy.y, topResult.z);

				result.bottom = bottom;
				result.top = top;
				result.middle = middle;

				result.platform = false;
			}
		}
	}

	void Box2DGrid::check(std::vector<VerticalSeg>& segments, vec3& c, int faceID)
	{
		segments.clear();
		segments.reserve(100);

		std::vector<VerticalCollide> collides;
		vec3 start(c.x, c.y, 0.0f);
		check(collides, start, CheckDir::eUp, faceID);

		{
			VerticalCollide bottom;
			bottom.xy = vec2(c.x, c.y);
			bottom.z = 0.0f;
			bottom.faceid = -1;
			collides.push_back(bottom);
		}

		std::sort(collides.begin(), collides.end(), [](VerticalCollide& r1, VerticalCollide& r2)->bool {
			return r1.z < r2.z;
			});

		int resultSize = (int)collides.size();
		if (resultSize > 1)
		{
			int searchIndex = 0;
			bool in = false;
			int mindiff = faceID - collides.at(0).faceid;
			for (int i = 0; i < resultSize; ++i)
			{
				if (abs(faceID - collides.at(i).faceid) < mindiff)
				{
					searchIndex = i;
					in = true;
					mindiff = abs(faceID - collides.at(i).faceid);
				}

				//if (collides.at(i).faceid == faceID)
				//{
				//	searchIndex = i;
				//	in = true;
				//	break;
				//}
			}

			if (in)
			{
				VerticalCollide& q = collides.at(searchIndex);
				trimesh::vec3 cc = vec3(q.xy.x, q.xy.y, q.z);
				float dotValue = q.faceid >= 0 ? m_dotValues.at(q.faceid) : 1.0f;
				if ((dotValue > 0.0f) && (searchIndex < resultSize - 1))
				{
					int upIndex = searchIndex + 1;
					VerticalCollide& upQuery = collides.at(upIndex);
					float upValue = upQuery.faceid >= 0 ? m_dotValues.at(upQuery.faceid) : 1.0f;
					if (upValue < 0.0f)
					{
						VerticalSeg seg;
						seg.b = cc;
						seg.t = vec3(upQuery.xy.x, upQuery.xy.y, upQuery.z);
						segments.push_back(seg);
					}
				}
				else if ((dotValue < 0.0f) && (searchIndex > 0))
				{
					int lowerIndex = searchIndex - 1;
					VerticalCollide& lQuery = collides.at(lowerIndex);
					float lValue = lQuery.faceid >= 0 ? m_dotValues.at(lQuery.faceid) : 1.0f;
					if (lValue > 0.0f)
					{
						VerticalSeg seg;
						seg.t = cc;
						seg.b = vec3(lQuery.xy.x, lQuery.xy.y, lQuery.z);
						segments.push_back(seg);
					}
				}
			}
		}
	}

	void Box2DGrid::check(std::vector<VerticalCollide>& collides, vec3& c, CheckDir dir, int id, bool useFace, bool skip)
	{
		collides.clear();

		vec2 xy = vec2(c.x, c.y);
		ivec2 idx = index(xy);

		//if (m_logCallback)
		//{
		//	m_logCallback->log("idx [%d , %d]", idx.x, idx.y);
		//}

		vec3 n = vec3(0.0f, 0.0f, 1.0f);
		if (dir == CheckDir::eDown) n = vec3(0.0f, 0.0f, -1.0f);
		if (idx.x >= 0 && idx.x < m_width && idx.y >= 0 && idx.y < m_height)
		{
			int iindex = idx.x + idx.y * m_width;
			std::vector<int>& cells = m_cells.at(iindex);
			if (cells.size() > 0) collides.reserve(cells.size());

			for (int i : cells)
			{
				box2& b = m_boxes.at(i);
				vec2& depth = m_depths.at(i);
				if (xy.x >= b.min.x && xy.x <= b.max.x && xy.y >= b.min.y && xy.y <= b.max.y)
				{
					if ((dir == CheckDir::eDown) && (c.z < depth.x))
					{
						continue;
					}

					if ((dir == CheckDir::eUp) && (c.z > depth.y))
					{
						continue;
					}
					TriMesh::Face& f = m_mesh->faces.at(i);
					if (skip)
					{
						if (useFace && (id == i))
						{
							continue;
						}
						else if (!useFace && (id == f[0] || id == f[1] || id == f[2]))
						{
							continue;
						}
					}

					//if (m_logCallback)
					//{
					//	m_logCallback->log("test cell %d : dotValue %f", i, m_dotValues.at(i));
					//}

					if (abs(m_dotValues.at(i)) > -0.001f)
					{
						vec3& v0 = m_vertexes.at(f[0]);
						vec3& v1 = m_vertexes.at(f[1]);
						vec3& v2 = m_vertexes.at(f[2]);

						float t, u, v;
						if (rayIntersectTriangle(c, n, v0, v1, v2, &t, &u, &v) && t >= -0.001f)
						{
							VerticalCollide result;
							result.faceid = i;
							result.xy = xy;
							result.z = c.z + t * n.z;
							collides.push_back(result);
						}
					}
				}
			}
		}
	}

	ivec2 Box2DGrid::index(vec2& xy)
	{
		int nx = (int)floorf((xy.x - m_globalBox.min.x) / m_gridSize);
		int ny = (int)floorf((xy.y - m_globalBox.min.y) / m_gridSize);
		return ivec2(nx, ny);
	}

	bool Box2DGrid::checkFace(int primitiveID)
	{
		trimesh::TriMesh::Face& f = m_mesh->faces.at(primitiveID);
		trimesh::vec3 v0 = m_xf * m_mesh->vertices.at(f[0]);
		trimesh::vec3 v1 = m_xf * m_mesh->vertices.at(f[1]);
		trimesh::vec3 v2 = m_xf * m_mesh->vertices.at(f[2]);

		trimesh::vec3 v01 = v1 - v0;
		trimesh::vec3 v02 = v2 - v0;
		trimesh::vec3 n = v01 TRICROSS v02;
		trimesh::normalize(n);
		return abs(trimesh::dot(n, vec3(0.0f, 0.0f, 1.0f))) > 0.001f;
	}

	trimesh::vec3 Box2DGrid::checkFaceNormal(int primitiveID)
	{
		trimesh::TriMesh::Face& f = m_mesh->faces.at(primitiveID);
		trimesh::vec3 v0 = m_xf * m_mesh->vertices.at(f[0]);
		trimesh::vec3 v1 = m_xf * m_mesh->vertices.at(f[1]);
		trimesh::vec3 v2 = m_xf * m_mesh->vertices.at(f[2]);

		trimesh::vec3 v01 = v1 - v0;
		trimesh::vec3 v02 = v2 - v0;
		trimesh::vec3 n = v01 TRICROSS v02;
		trimesh::normalize(n);
		return n;
	}

	void mergeSupport(std::vector<msbase::VerticalSeg>& supports, std::vector<msbase::VerticalSeg> supportsOfVecAndEdge, float supportSize)
	{
		for (msbase::VerticalSeg vs : supportsOfVecAndEdge)
		{
			bool repeat = false;
			for (msbase::VerticalSeg vs_face : supports)
			{
				trimesh::vec3 diff = vs_face.t - vs.t;
				diff[2] = 0;
				if (len(diff) < supportSize && vs_face.b[2] == vs.b[2])
				{
					repeat = true;
					break;
				}
			}
			if (!repeat)
			{
				supports.push_back(vs);
			}
		}
	}
	void Box2DGrid::autoSupportOfVecAndSeg(std::vector<VerticalSeg>& supports, float size, bool platform, bool uplight)
	{
		std::vector<msbase::VerticalSeg> supportsOfVecAndEdge, supportsOfVecAndEdge_blk;
		GenSource source;
		genSourceOfVecAndSeg(source);
		addVertexSupports(supportsOfVecAndEdge, source);
		addEdgeSupports(supportsOfVecAndEdge, source, size);
		
		supportsOfVecAndEdge_blk.reserve(supportsOfVecAndEdge.size());
		for (VerticalSeg seg : supportsOfVecAndEdge)
		{
			if (platform && seg.b.z>0.f)
			{
				continue;
			}
			if (uplight)
			{
				//稍微抬高支撑使支撑更稳定
				seg.t[2] += 2.0f;
			}
			supportsOfVecAndEdge_blk.push_back(seg);
		}
		supportsOfVecAndEdge_blk.swap(supportsOfVecAndEdge);
		mergeSupport(supports, supportsOfVecAndEdge, size);
	}

	void Box2DGrid::autoSupport(std::vector<VerticalSeg>& supports, float size, float angle, bool platform)
	{
		supports.clear();

		float delta = size;
		float start = size / 2.0f;
		vec2 globalSize = m_globalBox.size();

		std::vector<vec2> samples;
		vec2 dmin = m_globalBox.min + vec2(start, start);
		ivec4 clamps(0, 0, 0, 0);
		clamps[1] = (int)floorf((globalSize.x - start) / delta);
		clamps[3] = (int)floorf((globalSize.y - start) / delta);
		for (int x = 0; x <= clamps[1]; ++x)
		{
			for (int y = 0; y <= clamps[3]; ++y)
			{
				vec2 offset = delta * vec2((float)x, (float)y);
				samples.push_back(dmin + offset);
			}
		}

		auto testOneSample = [this, &angle, &platform](vec2& vv, std::vector<VerticalSeg>& supps) {
			supps.clear();
			supps.reserve(10);

			std::vector<VerticalCollide> collides;

			vec3 start(vv.x,vv.y, 0.0f);
			check(collides, start, CheckDir::eUp, -1);

			size_t resultSize = collides.size();

			if (resultSize == 0)
				return;

			{
				VerticalCollide bottom;
				bottom.xy = vec2(vv.x, vv.y);
				bottom.z = 0.0f;
				bottom.faceid = -1;
				collides.push_back(bottom);
			}

			std::sort(collides.begin(), collides.end(), [](VerticalCollide& r1, VerticalCollide& r2)->bool {
				return r1.z < r2.z;
				});

			size_t startIndex = 0;
			float cosValue = cosf(M_PIf * (90.0f - angle) / 180.0f);

			while (startIndex < resultSize)
			{
				VerticalCollide& cResult = collides.at(startIndex);
				float dotu = cResult.faceid >= 0 ? m_dotValues.at(cResult.faceid) : 1.0f;
				if ((dotu > 0.0f) && (startIndex < resultSize - 1))
				{
					VerticalCollide& nResult = collides.at(startIndex + 1);
					float dotd = nResult.faceid >= 0 ? m_dotValues.at(nResult.faceid) : 1.0f;
					if ((dotu * dotd < 0.0f) && (abs(dotd) >= cosValue) && (abs(nResult.z - cResult.z) >= 0.5f))
					{
						VerticalSeg seg;
						seg.t = vec3(nResult.xy.x, nResult.xy.y, nResult.z);
						seg.b = vec3(cResult.xy.x, cResult.xy.y, cResult.z);

						supps.push_back(seg);
					}
				}

				if (startIndex == 0 && platform)
				{
					break;
				}
				++startIndex;
			}
		};

		size_t sampleSize = samples.size();
		for (size_t i = 0; i < sampleSize; ++i)
		{
			vec2 v = samples.at(i);

			std::vector<VerticalSeg> supps;
			testOneSample(v, supps);
			if (supps.size() > 0)
			{
				supports.insert(supports.end(), supps.begin(), supps.end());
			}
		}
	}

	void Box2DGrid::genSource(GenSource& source)
	{
		int vertexNum = (int)m_mesh->vertices.size();
		int faceNum = (int)m_mesh->faces.size();

		if (vertexNum == 0 || faceNum == 0)
			return;

		source.vertexNum = vertexNum;
		source.faceNum = faceNum;

		std::vector<int>& vertexFlags = source.vertexFlags;
		vertexFlags.resize(vertexNum, 0);

		std::vector<int>& supportVertexes = source.supportVertexes;
		for (int vertexID = 0; vertexID < vertexNum; ++vertexID)
		{
			std::vector<int>& vertexHalfs = m_outHalfEdges.at(vertexID);
			int halfSize = (int)vertexHalfs.size();
			vec3& vertex = m_vertexes.at(vertexID);

			bool lowest = true;
			for (int halfIndex = 0; halfIndex < halfSize; ++halfIndex)
			{
				int endVertexID = endvertexid(vertexHalfs.at(halfIndex));
				if (m_vertexes.at(endVertexID).z < vertex.z)
				{
					lowest = false;
					break;
				}
			}
			if (lowest)
			{
				supportVertexes.push_back(vertexID);
				vertexFlags.at(vertexID) = 1;
			}
		}

		std::vector<ivec2>& supportEdges = source.supportEdges;
		std::vector<ivec3> edgesFlags(faceNum, ivec3(0, 0, 0));
		float edgeCosValue = cosf(M_PIf * 70.0f / 180.0f);
		float edgeFaceCosValue = cosf(M_PIf * 60.0f / 180.0f);
		for (int faceID = 0; faceID < faceNum; ++faceID)
		{
			ivec3& oppoHalfs = m_oppositeHalfEdges.at(faceID);
			ivec3& edgeFlag = edgesFlags.at(faceID);
			TriMesh::Face& tFace = m_mesh->faces.at(faceID);
			vec3& faceNormal = m_faceNormals.at(faceID);

			bool faceSupport = m_dotValues.at(faceID) < (-edgeFaceCosValue);
			for (int edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
			{
				if (edgeFlag[edgeIndex] == 0)
				{
					edgeFlag[edgeIndex] = 1;

					int vertexID1 = tFace[edgeIndex];
					int vertexID2 = tFace[(edgeIndex + 1) % 3];
					vec3 edge = m_vertexes.at(vertexID1) - m_vertexes.at(vertexID2);
					vec3 nedge = normalized(edge);

					if (abs(trimesh::dot(nedge, vec3(0.0f, 0.0f, 1.0f))) < edgeCosValue)
					{
						int oppoHalf = oppoHalfs.at(edgeIndex);
						bool shouldAdd = false;
						if (oppoHalf >= 0)
						{
							int oppoFaceID;
							int edgeID;
							halfdecode(oppoHalf, oppoFaceID, edgeID);
							edgesFlags.at(oppoFaceID)[edgeID] = 1;

							vec3& oppoFaceNormal = m_faceNormals.at(oppoFaceID);
							bool oppoFaceSupport = m_dotValues.at(oppoFaceID) < (-edgeFaceCosValue);
							if (oppoFaceSupport && faceSupport)
							{

								if (trimesh::dot(faceNormal, oppoFaceNormal) < 0.0f)
								{
									shouldAdd = true;
								}
							}
						}
						else  // hole edge
						{
							shouldAdd = faceSupport;
						}

						if (shouldAdd)
						{
							ivec2 edgeID(vertexID1, vertexID2);
							supportEdges.push_back(edgeID);
						}
					}
				}
			}
		}

		std::vector<bool> visitFlags(faceNum, false);
		std::vector<int> visitStack;
		std::vector<int> nextStack;
		std::vector<std::vector<int>>& supportFaces = source.supportFaces;
		float faceCosValue = cosf(M_PIf * 30.0f / 180.0f);
		for (int faceID = 0; faceID < faceNum; ++faceID)
		{
			if (m_dotValues.at(faceID) < -faceCosValue && visitFlags.at(faceID) == false)
			{
				visitFlags.at(faceID) = true;
				visitStack.push_back(faceID);

				std::vector<int> facesChunk;
				facesChunk.push_back(faceID);
				while (!visitStack.empty())
				{
					int seedSize = (int)visitStack.size();
					for (int seedIndex = 0; seedIndex < seedSize; ++seedIndex)
					{
						int cFaceID = visitStack.at(seedIndex);
						ivec3& oppoHalfs = m_oppositeHalfEdges.at(cFaceID);
						for (int halfID = 0; halfID < 3; ++halfID)
						{
							int oppoHalf = oppoHalfs.at(halfID);
							if (oppoHalf >= 0)
							{
								int oppoFaceID = faceid(oppoHalf);
								if (m_dotValues.at(oppoFaceID) < -faceCosValue && (visitFlags.at(oppoFaceID) == false))
								{
									nextStack.push_back(oppoFaceID);
									facesChunk.push_back(oppoFaceID);
									visitFlags.at(oppoFaceID) = true;
								}
								else
								{
									visitFlags.at(oppoFaceID) = true;
								}
							}
						}
					}

					visitStack.swap(nextStack);
					nextStack.clear();
				}

				supportFaces.push_back(facesChunk);
			}
			else
			{
				visitFlags.at(faceID) = true;
			}
		}
	}

	void Box2DGrid::genSourceOfVecAndSeg(GenSource& source)
	{
		int vertexNum = (int)m_mesh->vertices.size();
		int faceNum = (int)m_mesh->faces.size();

		if (vertexNum == 0 || faceNum == 0)
			return;

		auto checkLowest = [this](TriMesh::Face face, int ptIdx1, int ptIdx2)
		{
			for (int i = 0; i < 3; i++)
			{
				if (face[i] != ptIdx1 && face[i] != ptIdx2)
				{
					float z = m_vertexes.at(face[i]).z;
					if (z >= m_vertexes.at(ptIdx1).z && z >= m_vertexes.at(ptIdx2).z)
						return true;
				}
			}
			return false;
		};

		source.vertexNum = vertexNum;
		source.faceNum = faceNum;

		std::vector<int>& vertexFlags = source.vertexFlags;
		vertexFlags.resize(vertexNum, 0);

		std::vector<int>& supportVertexes = source.supportVertexes;
		for (int vertexID = 0; vertexID < vertexNum; ++vertexID)
		{
			std::vector<int>& vertexHalfs = m_outHalfEdges.at(vertexID);
			int halfSize = (int)vertexHalfs.size();
			vec3& vertex = m_vertexes.at(vertexID);

			bool lowest = true;
			for (int halfIndex = 0; halfIndex < halfSize; ++halfIndex)
			{
				int endVertexID = endvertexid(vertexHalfs.at(halfIndex));
				if (m_vertexes.at(endVertexID).z - vertex.z < 0.01)
				{
					lowest = false;
					break;
				}
			}
			if (lowest)
			{
				supportVertexes.push_back(vertexID);
				vertexFlags.at(vertexID) = 1;
			}			
		}
		
		std::vector<ivec2>& supportEdges = source.supportEdges;
		std::vector<ivec3> edgesFlags(faceNum, ivec3(0, 0, 0));
		float edgeCosValue = cosf(M_PIf * 70.0f / 180.0f);
		float edgeFaceCosValue = cosf(M_PIf * 5.0f / 180.0f);
		float edgeNormalsCosValue = cosf(M_PIf * 30.0f / 180.0f);
		for (int faceID = 0; faceID < faceNum; ++faceID)
		{
			ivec3& oppoHalfs = m_oppositeHalfEdges.at(faceID);
			ivec3& edgeFlag = edgesFlags.at(faceID);
			TriMesh::Face& tFace = m_mesh->faces.at(faceID);
			vec3& faceNormal = m_faceNormals.at(faceID);

			bool faceNotSupport = m_dotValues.at(faceID) > (-edgeFaceCosValue) && m_dotValues.at(faceID) < 0.f;
			for (int edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
			{
				if (edgeFlag[edgeIndex] == 0)
				{
					edgeFlag[edgeIndex] = 1;
					if (!faceNotSupport) continue;
					int vertexID1 = tFace[edgeIndex];
					int vertexID2 = tFace[(edgeIndex + 1) % 3];
					vec3 edge = m_vertexes.at(vertexID1) - m_vertexes.at(vertexID2);
					vec3 nedge = normalized(edge);

					if (abs(trimesh::dot(nedge, vec3(0.0f, 0.0f, 1.0f))) < edgeCosValue)
					{
						int oppoHalf = oppoHalfs.at(edgeIndex);
						bool shouldAdd = false;
						if (oppoHalf >= 0)
						{
							int oppoFaceID;
							int edgeID;
							halfdecode(oppoHalf, oppoFaceID, edgeID);
							edgesFlags.at(oppoFaceID)[edgeID] = 1;

							vec3& oppoFaceNormal = m_faceNormals.at(oppoFaceID);
							bool oppoFaceNotSupport = m_dotValues.at(oppoFaceID) > (-edgeFaceCosValue) && m_dotValues.at(oppoFaceID) < 0.f;
							if (oppoFaceNotSupport && faceNotSupport)
							{
								if ((trimesh::dot(normalized(faceNormal + oppoFaceNormal), vec3(0.0f, 0.0f, -1.0f)) > edgeNormalsCosValue)
									&& checkLowest(tFace, vertexID1, vertexID2))
								{
									shouldAdd = true;		
								}
							}
						}
						else  // hole edge
						{
							shouldAdd = faceNotSupport;
						}

						if (shouldAdd)
						{
							ivec2 edgeID(vertexID1, vertexID2);
							supportEdges.push_back(edgeID);
						}
					}
				}
			}
		}
	}

	void Box2DGrid::autoDLPSupport(std::vector<VerticalSeg>& supports, float ratio, float angle, bool platform)
	{
		supports.clear();

		GenSource source;
		genSource(source);

		genSupports(supports, source);
	}

	void Box2DGrid::genSupports(std::vector<VerticalSeg>& segments, GenSource& source)
	{
		addVertexSupports(segments, source);
		addEdgeSupports(segments, source);
		addFaceSupports(segments, source);
	}

	std::vector<VerticalSeg> Box2DGrid::recheckVertexSupports(std::vector<VerticalSeg> Vertexes)
	{
		std::vector<VerticalSeg> segments;
		for (VerticalSeg vex : Vertexes)
		{
			vec3 vertex = vex.t;
			std::vector<VerticalCollide> collides;
			check(collides, vertex, CheckDir::eDown, -1, false, false);

			std::sort(collides.begin(), collides.end(), [](VerticalCollide& r1, VerticalCollide& r2)->bool {
				return r1.z < r2.z;
				});

			float optimizeZ = 0.0f;
			for (int idx = collides.size() - 1; idx >= 0; idx --)
			{
				VerticalCollide& vCollide = collides[idx];
				if ( vertex[2] - vCollide.z > 0.01 &&  m_dotValues.at(vCollide.faceid) > 0.0f)
				{
					optimizeZ = vCollide.z;
					break;
				}
			}
			VerticalSeg seg;
			seg.t = vertex;
			seg.b = vec3(vertex.x, vertex.y, optimizeZ);
			segments.push_back(seg);
		}
		return segments;
	}

	void Box2DGrid::addVertexSupports(std::vector<VerticalSeg>& segments, GenSource& source)
	{
		std::vector<int>& supportVertexes = source.supportVertexes;

		for (int vertexID : supportVertexes)
		{
			vec3 vertex = m_vertexes.at(vertexID);
			std::vector<VerticalCollide> collides;
			check(collides, vertex, CheckDir::eDown, vertexID, false, true);

			std::sort(collides.begin(), collides.end(), [](VerticalCollide& r1, VerticalCollide& r2)->bool {
				return r1.z < r2.z;
			});

			float optimizeZ = -1.0f;
			if (collides.size() == 0)
			{
				optimizeZ = 0.0f;
			}
			else
			{
				VerticalCollide& vCollide = collides.back();
				if (m_dotValues.at(vCollide.faceid) > 0.0f)
				{
					optimizeZ = vCollide.z;
				}
			}

			if (optimizeZ >= 0.0f)
			{
				VerticalSeg seg;
				seg.t = vertex;
				seg.b = vec3(vertex.x, vertex.y, optimizeZ);
				segments.push_back(seg);
			}
		}
	}

	void Box2DGrid::addEdgeSupports(std::vector<VerticalSeg>& segments, GenSource& source, float supportSize)
	{
		std::vector<ivec2>& supportEdges = source.supportEdges;
		std::vector<int>& vertexFlags = source.vertexFlags;

		float minDelta = supportSize;
		float samePointsDelta = 0.01f;
		for (ivec2 vertexesID : supportEdges)
		{
			int vertexID1 = vertexesID.x;
			int vertexID2 = vertexesID.y;
			vec3& vertex1 = m_vertexes.at(vertexID1);
			vec3& vertex2 = m_vertexes.at(vertexID2);

			box2 box2D;
			box2D += vec2(vertex1.x, vertex1.y);
			box2D += vec2(vertex2.x, vertex2.y);

			vec2 boxSize = box2D.size();
			float maxLen = boxSize.x > boxSize.y ? boxSize.x : boxSize.y;

			std::vector<vec3> samplePoints;

			if (vertexFlags.at(vertexID1) == 0)
			{
				samplePoints.push_back(vertex1);
				vertexFlags.at(vertexID1) = 2;
			}
			if ((maxLen > minDelta) && (vertexFlags.at(vertexID2) == 0))
			{
				samplePoints.push_back(vertex2);
				vertexFlags.at(vertexID2) = 2;
			}

			int sampleEdgeNum = floorf(maxLen / minDelta) - 1;
			if (sampleEdgeNum > 0)
			{
				float dt = 1.0f / (float)(sampleEdgeNum + 1);
				for (int sampleIndex = 0; sampleIndex < sampleEdgeNum; ++sampleIndex)
				{
					float t = (float)(sampleIndex + 1) * dt;
					vec3 p = vertex1 * (1.0f - t) + vertex2 * t;
					samplePoints.push_back(p);
				}
			}

			for (vec3& samplePoint : samplePoints)
			{
				std::vector<VerticalCollide> collides;
				check(collides, samplePoint, CheckDir::eDown, -1, false, false);

				std::sort(collides.begin(), collides.end(), [](VerticalCollide& r1, VerticalCollide& r2)->bool {
					return r1.z < r2.z;
					});

				float optimizeZ = -1.0f;
				if(collides.size() > 0)
				{
					int selectIndex = (int)(collides.size() - 1);
					while (selectIndex >= 0)
					{
						VerticalCollide& vCollide = collides.at(selectIndex);
						if ((samplePoint.z - vCollide.z) >= samePointsDelta)
						{
							break;
						}
						--selectIndex;
					}

					if (selectIndex >= 0)
					{
						VerticalCollide& vCollide = collides.at(selectIndex);
						if (m_dotValues.at(vCollide.faceid) > 0.0f)
						{
							optimizeZ = vCollide.z;
						}
					}
					else
					{
						optimizeZ = 0.0f;
					}
				}

				if (optimizeZ >= 0.0f)
				{
					VerticalSeg seg;
					seg.t = samplePoint;
					seg.b = vec3(samplePoint.x, samplePoint.y, optimizeZ);
					segments.push_back(seg);
				}
			}
		}
	}

	void Box2DGrid::addFaceSupports(std::vector<VerticalSeg>& segments, GenSource& source)
	{
		std::vector<std::vector<int>>& supportFaces = source.supportFaces;
		std::vector<int>& vertexFlags = source.vertexFlags;

		float minDelta = 3.0f;
		float samePointsDelta = 0.01f;
		float insertRadius = 2.0f;

		for (std::vector<int>& faceChunk : supportFaces)
		{
			std::vector<vec3> samplePoints;
			std::vector<vec2> supportsSamplePosition;

			std::sort(faceChunk.begin(), faceChunk.end(), [this](int r1, int r2)->bool {
				return m_dotValues.at(r1) < m_dotValues.at(r2);
				});

			for (int faceID : faceChunk)
			{
				TriMesh::Face& tFace = m_mesh->faces.at(faceID);
				vec3& vertex1 = m_vertexes.at(tFace[0]);
				vec3& vertex2 = m_vertexes.at(tFace[1]);
				vec3& vertex3 = m_vertexes.at(tFace[2]);

				if (vertexFlags.at(tFace[0]) == 0)
				{
					vec2 xypoint = vec2(vertex1.x, vertex1.y);
					if (testInsert(supportsSamplePosition, xypoint, insertRadius))
					{
						samplePoints.push_back(vertex1);
						vertexFlags.at(tFace[0]) = 3;
						supportsSamplePosition.push_back(xypoint);
					}
				}

				vec3 e12 = vertex2 - vertex1;
				vec3 e13 = vertex3 - vertex1;
				float len12 = trimesh::len(e12);
				float len13 = trimesh::len(e13);

				int sampleXNum = floorf(len12 / minDelta) - 1;
				int sampleYNum = floorf(len13 / minDelta) - 1;
				if (sampleXNum > 0 || sampleYNum > 0)
				{
					float dxt = 1.0f / (float)(sampleXNum + 1);
					float dyt = 1.0f / (float)(sampleYNum + 1);
					for (int sampleX = 0; sampleX < sampleXNum; ++sampleX)
					{
						float tx = (float)(sampleX + 1) * dxt;
						for (int sampleY = 0; sampleY < sampleYNum; ++sampleY)
						{
							float ty = (float)(sampleY + 1) * dyt;

							if (tx + ty <= 1.0f)
							{
								vec3 offset = tx * e12 + ty * e13;
								vec3 point = offset + vertex1;
								vec2 xypoint = vec2(point.x, point.y);
								if (testInsert(supportsSamplePosition, xypoint, insertRadius))
								{
									samplePoints.push_back(point);
									vertexFlags.at(tFace[0]) = 3;
									supportsSamplePosition.push_back(xypoint);
								}
							}
						}
					}
				}
			}

			for (vec3& samplePoint : samplePoints)
			{
				std::vector<VerticalCollide> collides;
				check(collides, samplePoint, CheckDir::eDown, -1, false, false);

				std::sort(collides.begin(), collides.end(), [](VerticalCollide& r1, VerticalCollide& r2)->bool {
					return r1.z < r2.z;
					});

				float optimizeZ = -1.0f;
				if (collides.size() > 0)
				{
					int selectIndex = (int)(collides.size() - 1);
					while (selectIndex >= 0)
					{
						VerticalCollide& vCollide = collides.at(selectIndex);
						if ((samplePoint.z - vCollide.z) >= samePointsDelta)
						{
							break;
						}
						--selectIndex;
					}

					if (selectIndex >= 0)
					{
						VerticalCollide& vCollide = collides.at(selectIndex);
						if (m_dotValues.at(vCollide.faceid) > 0.0f)
						{
							optimizeZ = vCollide.z;
						}
					}
					else
					{
						optimizeZ = 0.0f;
					}
				}

				if (optimizeZ >= 0.0f)
				{
					VerticalSeg seg;
					seg.t = samplePoint;
					seg.b = vec3(samplePoint.x, samplePoint.y, optimizeZ);
					segments.push_back(seg);
				}
			}
		}
	}

	bool Box2DGrid::testInsert(std::vector<trimesh::vec2>& samples, trimesh::vec2& xy, float radius)
	{
		bool canInsert = true;
		float radius2 = radius * radius;
		for (vec2& _xy : samples)
		{
			vec2 delta = _xy - xy;
			if (trimesh::len2(delta) < radius2)
			{
				canInsert = false;
				break;
			}
		}

		return canInsert;
	}

	void Box2DGrid::addVertexSources(std::vector<DLPSource>& sources, GenSource& source)
	{
		std::vector<int>& supportVertexes = source.supportVertexes;

		for (int vertexID : supportVertexes)
		{
			vec3 vertex = m_vertexes.at(vertexID);
			DLPSource dlpSource;
			dlpSource.position = vertex;
			dlpSource.normal = trimesh::vec3(0.0f, 0.0f, -1.0f);
			sources.push_back(dlpSource);
		}
	}

	void Box2DGrid::addEdgeSources(std::vector<DLPSource>& sources, GenSource& source)
	{
		std::vector<ivec2>& supportEdges = source.supportEdges;
		std::vector<int>& vertexFlags = source.vertexFlags;

		float minDelta = 3.0f;
		for (ivec2 vertexesID : supportEdges)
		{
			int vertexID1 = vertexesID.x;
			int vertexID2 = vertexesID.y;
			vec3& vertex1 = m_vertexes.at(vertexID1);
			vec3& vertex2 = m_vertexes.at(vertexID2);

			box2 box2D;
			box2D += vec2(vertex1.x, vertex1.y);
			box2D += vec2(vertex2.x, vertex2.y);

			vec2 boxSize = box2D.size();
			float maxLen = boxSize.x > boxSize.y ? boxSize.x : boxSize.y;

			std::vector<vec3> samplePoints;

			if (vertexFlags.at(vertexID1) == 0)
			{
				samplePoints.push_back(vertex1);
				vertexFlags.at(vertexID1) = 2;
			}
			if ((maxLen > minDelta) && (vertexFlags.at(vertexID2) == 0))
			{
				samplePoints.push_back(vertex2);
				vertexFlags.at(vertexID2) = 2;
			}

			int sampleEdgeNum = floorf(maxLen / minDelta) - 1;
			if (sampleEdgeNum > 0)
			{
				float dt = 1.0f / (float)(sampleEdgeNum + 1);
				for (int sampleIndex = 0; sampleIndex < sampleEdgeNum; ++sampleIndex)
				{
					float t = (float)(sampleIndex + 1) * dt;
					vec3 p = vertex1 * (1.0f - t) + vertex2 * t;
					samplePoints.push_back(p);
				}
			}

			for (vec3& samplePoint : samplePoints)
			{
				DLPSource dlpSource;
				dlpSource.position = samplePoint;
				dlpSource.normal = trimesh::vec3(0.0f, 0.0f, -1.0f);
				sources.push_back(dlpSource);
			}
		}
	}

	void Box2DGrid::addFaceSources(std::vector<DLPSource>& sources, GenSource& source)
	{
		std::vector<std::vector<int>>& supportFaces = source.supportFaces;
		std::vector<int>& vertexFlags = source.vertexFlags;

		float minDelta = 3.0f;
		float insertRadius = 2.0f;

		for (std::vector<int>& faceChunk : supportFaces)
		{
			std::vector<vec3> samplePoints;
			std::vector<vec3> sampleNormals;
			std::vector<vec2> supportsSamplePosition;

			std::sort(faceChunk.begin(), faceChunk.end(), [this](int r1, int r2)->bool {
				return m_dotValues.at(r1) < m_dotValues.at(r2);
				});

			for (int faceID : faceChunk)
			{
				TriMesh::Face& tFace = m_mesh->faces.at(faceID);
				vec3& vertex1 = m_vertexes.at(tFace[0]);
				vec3& vertex2 = m_vertexes.at(tFace[1]);
				vec3& vertex3 = m_vertexes.at(tFace[2]);
				vec3 normal = m_faceNormals.at(faceID);

				if (vertexFlags.at(tFace[0]) == 0)
				{
					vec2 xypoint = vec2(vertex1.x, vertex1.y);
					if (testInsert(supportsSamplePosition, xypoint, insertRadius))
					{
						samplePoints.push_back(vertex1);
						sampleNormals.push_back(normal);
						vertexFlags.at(tFace[0]) = 3;
						supportsSamplePosition.push_back(xypoint);
					}
				}

				vec3 e12 = vertex2 - vertex1;
				vec3 e13 = vertex3 - vertex1;
				float len12 = trimesh::len(e12);
				float len13 = trimesh::len(e13);

				int sampleXNum = floorf(len12 / minDelta) - 1;
				int sampleYNum = floorf(len13 / minDelta) - 1;
				if (sampleXNum > 0 || sampleYNum > 0)
				{
					float dxt = 1.0f / (float)(sampleXNum + 1);
					float dyt = 1.0f / (float)(sampleYNum + 1);
					for (int sampleX = 0; sampleX < sampleXNum; ++sampleX)
					{
						float tx = (float)(sampleX + 1) * dxt;
						for (int sampleY = 0; sampleY < sampleYNum; ++sampleY)
						{
							float ty = (float)(sampleY + 1) * dyt;

							if (tx + ty <= 1.0f)
							{
								vec3 offset = tx * e12 + ty * e13;
								vec3 point = offset + vertex1;
								vec2 xypoint = vec2(point.x, point.y);
								if (testInsert(supportsSamplePosition, xypoint, insertRadius))
								{
									samplePoints.push_back(point);
									sampleNormals.push_back(normal);
									vertexFlags.at(tFace[0]) = 3;
									supportsSamplePosition.push_back(xypoint);
								}
							}
						}
					}
				}
			}

			size_t sampleSize = samplePoints.size();
			for (size_t i = 0; i < sampleSize; ++i)
			{
				DLPSource dlpSource;
				dlpSource.position = samplePoints.at(i);
				dlpSource.normal = sampleNormals.at(i);
				sources.push_back(dlpSource);
			}
		}
	}
}