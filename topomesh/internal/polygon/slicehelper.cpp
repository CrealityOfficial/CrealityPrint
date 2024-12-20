#include "slicehelper.h"
#include "polygonLib.h"
#include "point3.h"
#include "coord_t.h"
#include "linearalg2d.h"

namespace topomesh
{
	SliceHelper::SliceHelper()
		: meshSrc(nullptr)
	{

	}

	SliceHelper::~SliceHelper()
	{

	}

	void SliceHelper::prepare(MeshObject* _mesh)
	{
		mesh = _mesh;
		buildMeshFaceHeightsRange(mesh, faceRanges);
	}

	void SliceHelper::prepare(trimesh::TriMesh* _meshSrc)
	{
		meshSrc = _meshSrc;
		getMeshFace();
		buildMeshFaceHeightsRange(_meshSrc, faceRanges);
	}

	SlicerSegment SliceHelper::project2D(const Point3& p0, const Point3& p1, const Point3& p2, const coord_t z)
	{
		SlicerSegment seg;

		seg.start.X = interpolate(z, p0.z, p1.z, p0.x, p1.x);
		seg.start.Y = interpolate(z, p0.z, p1.z, p0.y, p1.y);
		seg.end.X = interpolate(z, p0.z, p2.z, p0.x, p2.x);
		seg.end.Y = interpolate(z, p0.z, p2.z, p0.y, p2.y);

		return seg;
	}

	void SliceHelper::buildMeshFaceHeightsRange(const MeshObject* mesh, std::vector<Point2>& heightRanges)
	{
		int faceSize = (int)mesh->faces.size();
		if (faceSize > 0) heightRanges.resize(faceSize);
		for (int i = 0; i < faceSize; ++i)
		{
			const MeshFace& face = mesh->faces[i];
			const MeshVertex& v0 = mesh->vertices[face.vertex_index[0]];
			const MeshVertex& v1 = mesh->vertices[face.vertex_index[1]];
			const MeshVertex& v2 = mesh->vertices[face.vertex_index[2]];

			// get all vertices represented as 3D point
			Point3 p0 = v0.p;
			Point3 p1 = v1.p;
			Point3 p2 = v2.p;

			// find the minimum and maximum z point		
			int32_t minZ = p0.z;
			if (p1.z < minZ)
			{
				minZ = p1.z;
			}
			if (p2.z < minZ)
			{
				minZ = p2.z;
			}

			int32_t maxZ = p0.z;
			if (p1.z > maxZ)
			{
				maxZ = p1.z;
			}
			if (p2.z > maxZ)
			{
				maxZ = p2.z;
			}

			heightRanges.at(i) = Point2(minZ, maxZ);
		}
	}

	void SliceHelper::sliceOneLayer(int z,
		std::vector<SlicerSegment>& segments, std::unordered_map<int, int>& face_idx_to_segment_idx)
	{
		segments.reserve(100);

		int faceNum = mesh->faces.size();
		// loop over all mesh faces
		for (int faceIdx = 0; faceIdx < faceNum; ++faceIdx)
		{
			if ((z < faceRanges[faceIdx].x) || (z > faceRanges[faceIdx].y))
				continue;

			// get all vertices per face
			const MeshFace& face = mesh->faces[faceIdx];
			const MeshVertex& v0 = mesh->vertices[face.vertex_index[0]];
			const MeshVertex& v1 = mesh->vertices[face.vertex_index[1]];
			const MeshVertex& v2 = mesh->vertices[face.vertex_index[2]];

			// get all vertices represented as 3D point
			Point3 p0 = v0.p;
			Point3 p1 = v1.p;
			Point3 p2 = v2.p;

			SlicerSegment s;
			s.endVertex = nullptr;
			int end_edge_idx = -1;

			if (p0.z < z && p1.z >= z && p2.z >= z)
			{
				s = project2D(p0, p2, p1, z);
				end_edge_idx = 0;
				if (p1.z == z)
				{
					s.endVertex = &v1;
				}
			}
			else if (p0.z > z && p1.z < z && p2.z < z)
			{
				s = project2D(p0, p1, p2, z);
				end_edge_idx = 2;
			}
			else if (p1.z < z && p0.z >= z && p2.z >= z)
			{
				s = project2D(p1, p0, p2, z);
				end_edge_idx = 1;
				if (p2.z == z)
				{
					s.endVertex = &v2;
				}
			}
			else if (p1.z > z && p0.z < z && p2.z < z)
			{
				s = project2D(p1, p2, p0, z);
				end_edge_idx = 0;
			}
			else if (p2.z < z && p1.z >= z && p0.z >= z)
			{
				s = project2D(p2, p1, p0, z);
				end_edge_idx = 2;
				if (p0.z == z)
				{
					s.endVertex = &v0;
				}
			}
			else if (p2.z > z && p1.z < z && p0.z < z)
			{
				s = project2D(p2, p0, p1, z);
				end_edge_idx = 1;
			}
			else
			{
				//Not all cases create a segment, because a point of a face could create just a dot, and two touching faces
				//  on the slice would create two segments
				continue;
			}

			// store the segments per layer
			face_idx_to_segment_idx.insert(std::make_pair(faceIdx, segments.size()));
			s.faceIndex = faceIdx;
			s.endOtherFaceIdx = face.connected_face_index[end_edge_idx];
			segments.push_back(s);
		}
	}

	void SliceHelper::buildMeshFaceHeightsRange(const trimesh::TriMesh* _meshSrc, std::vector<Point2>& heightRanges)
	{
		int faceSize = (int)_meshSrc->faces.size();
		if (faceSize > 0) heightRanges.resize(faceSize);
		for (int i = 0; i < faceSize; ++i)
		{
			const trimesh::TriMesh::Face& face = _meshSrc->faces[i];

			// get all vertices represented as 3D point
			Point3 p0 = Point3(MM2INT(_meshSrc->vertices[face[0]].x), MM2INT(_meshSrc->vertices[face[0]].y), MM2INT(_meshSrc->vertices[face[0]].z));
			Point3 p1 = Point3(MM2INT(_meshSrc->vertices[face[1]].x), MM2INT(_meshSrc->vertices[face[1]].y), MM2INT(_meshSrc->vertices[face[1]].z));
			Point3 p2 = Point3(MM2INT(_meshSrc->vertices[face[2]].x), MM2INT(_meshSrc->vertices[face[2]].y), MM2INT(_meshSrc->vertices[face[2]].z));

			// find the minimum and maximum z point		
			int32_t minZ = p0.z;
			if (p1.z < minZ)
			{
				minZ = p1.z;
			}
			if (p2.z < minZ)
			{
				minZ = p2.z;
			}

			int32_t maxZ = p0.z;
			if (p1.z > maxZ)
			{
				maxZ = p1.z;
			}
			if (p2.z > maxZ)
			{
				maxZ = p2.z;
			}

			heightRanges.at(i) = Point2(minZ, maxZ);
		}
	}

	void getConnectFaceData(const std::vector<trimesh::TriMesh::Face>& allFaces, std::vector<std::vector<uint32_t>>& vertexConnectFaceData)
	{
		int faceId = 0;
		for (const trimesh::TriMesh::Face& face : allFaces)
		{
			vertexConnectFaceData[face[0]].push_back(faceId);
			vertexConnectFaceData[face[1]].push_back(faceId);
			vertexConnectFaceData[face[2]].push_back(faceId);
			faceId++;
		}
	}

	int getNearFaceId(const std::vector<std::vector<uint32_t>>& vertexConnectFaceData, int curFaceId, int vertexId_1, int vertexId_2)
	{
		const std::vector<uint32_t>& firstVertexFaceId = vertexConnectFaceData[vertexId_1];
		const std::vector<uint32_t>& secondVertexFaceId = vertexConnectFaceData[vertexId_2];
		for (int i = 0; i < firstVertexFaceId.size(); i++)
		{
			int MatchFaceId = firstVertexFaceId[i];
			if (MatchFaceId == curFaceId) continue;
			for (int j = 0; j < secondVertexFaceId.size(); j++)
			{
				if (MatchFaceId == secondVertexFaceId[j])
				{
					return MatchFaceId;
				}
			}
		}
		return -1;
	}

	void SliceHelper::getMeshFace()
	{
		size_t faceSize = meshSrc->faces.size();
		size_t vertexSize = meshSrc->vertices.size();
		vertexConnectFaceData.clear();
		vertexConnectFaceData.resize(vertexSize);
		getConnectFaceData(meshSrc->faces, vertexConnectFaceData);
		faces.clear();
		faces.resize(faceSize);
		for (int i = 0; i < faceSize; i++)
		{
			const trimesh::TriMesh::Face& face = meshSrc->faces[i];
			MeshFace& tmpFace = faces[i];
			tmpFace.vertex_index[0] = face[0];
			tmpFace.vertex_index[1] = face[1];
			tmpFace.vertex_index[2] = face[2];
			tmpFace.connected_face_index[0] = getNearFaceId(vertexConnectFaceData, i, face[0], face[1]);
			tmpFace.connected_face_index[1] = getNearFaceId(vertexConnectFaceData, i, face[1], face[2]);
			tmpFace.connected_face_index[2] = getNearFaceId(vertexConnectFaceData, i, face[2], face[0]);
		}
	}

	//std::pair < uint32_t, int>, first表示连接点id, 
	//second表示半边方向，second = 0表示无半边连接，second = 1表示当前点为半边终点，second = 2表示当前点为半边起始点
	std::vector<std::vector<std::pair < uint32_t, int>>> SliceHelper::generateVertexConnectVertexData()
	{
		auto checkExit = [](const std::vector<std::pair<uint32_t, int>>& connet_vertex, int vex_idx)
		{
			for (std::pair<uint32_t, int> data : connet_vertex)
			{
				if (data.first == vex_idx)
					return true;
			}
			return false;
		};

		int vertex_size = vertexConnectFaceData.size();
		std::vector<std::vector<std::pair<uint32_t, int>>> vertexConnectVertexData(vertex_size);
		for (int i = 0; i < vertex_size; i++)
		{
			std::vector<std::pair<uint32_t, int>>& connet_vertex = vertexConnectVertexData[i];
			const std::vector<uint32_t>& connet_faces = vertexConnectFaceData[i];
			for (int j = 0; j < connet_faces.size(); j++)
			{
				const MeshFace& face = faces[connet_faces[j]];
				for (int k = 0; k < 3; k++)
				{
					int vex_idx = face.vertex_index[k];
					if (vex_idx == i || checkExit(connet_vertex, vex_idx))
						continue;
					connet_vertex.push_back(std::make_pair(vex_idx, 0));
				}
			}
		}
		return vertexConnectVertexData;
	}

	void SliceHelper::generateConcave(std::vector<trimesh::vec3> & concave, const trimesh::quaternion* rotation, const trimesh::vec3 scale)
	{
		auto addPoint = [](ClipperLib::Path& item, int max_dis, bool openPoly = false)
		{
			int ptSize = item.size();
			if (ptSize == 0) return;
			ClipperLib::Path itemDensed;
			itemDensed.push_back(item[0]);
			int add = openPoly ? 0 : 1;
			for (int i = 1; i < ptSize + add; i++)
			{
				int curIdx = i < ptSize ? i : i - ptSize;
				ClipperLib::IntPoint ptAdd;
				ptAdd.X = item[curIdx].X - item[i - 1].X;
				ptAdd.Y = item[curIdx].Y - item[i - 1].Y;
				int len = sqrt(ptAdd.X * ptAdd.X + ptAdd.Y * ptAdd.Y);
				if (len > max_dis)
				{
					int addNum = len / max_dis;
					for (int j = 1; j < addNum; j++)
					{
						float theta = j * max_dis / (float)len;

						ClipperLib::IntPoint pt_theta = ClipperLib::IntPoint(theta * ptAdd.X, theta * ptAdd.Y);
						ClipperLib::IntPoint pt_new;
						pt_new.X = item[i - 1].X + pt_theta.X;
						pt_new.Y = item[i - 1].Y + pt_theta.Y;
						itemDensed.push_back(pt_new);
					}
				}
				itemDensed.push_back(item[curIdx]);
			}
			item.swap(itemDensed);
		};
		auto getMidPoint = [](const ClipperLib::Path& path)
		{
			ClipperLib::IntPoint mid_point;
			ClipperLib::Clipper clipper;
			clipper.AddPath(path, ClipperLib::PolyType::ptSubject, true);
			ClipperLib::IntRect rect = clipper.GetBounds();
			mid_point = ClipperLib::IntPoint((rect.left + rect.right) / 2, (rect.bottom + rect.top) / 2);
			return mid_point;
		};

		auto checkNoConnetion = [](const std::vector<std::pair<uint32_t, int>>& connet_data)
		{
			for (const std::pair<uint32_t, int>& connet : connet_data)
			{
				if (connet.second == 0)
					return false;
			}
			return true;
		};

		auto getAngleLeft = [](const trimesh::point & a, const trimesh::point& b, const trimesh::point& c)
		{
			const trimesh::point ba = a - b;
			const trimesh::point bc = c - b;
			const coord_t dott = dot(ba, bc); // dot product
			const coord_t det = ba.x * bc.y - ba.y * bc.x; // determinant
			if (det == 0)
			{
				if (
					(ba.x != 0 && (ba.x > 0) == (bc.x > 0))
					|| (ba.x == 0 && (ba.y > 0) == (bc.y > 0))
					)
				{
					return 0.; // pointy bit
				}
				else
				{
					return M_PI; // straight bit
				}
			}
			const double angle = -atan2(det, dott); // from -pi to pi
			if (angle >= 0)
			{
				return angle;
			}
			else
			{
				return M_PI * 2 + angle;
			}
		};

		auto find_connet = [&](std::vector<std::vector<std::pair<uint32_t, int>>>& vertexConnectVertexData, int cur_idx, int pre_point_idx)
		{
			int connet_idx = -1, first_connet_idx = -1, min_angle_idx = -1, find_i = -1;
			double min_angle = 2 * M_PI;
			std::vector<std::pair<uint32_t, int>>& vertex_connet_data = vertexConnectVertexData[cur_idx];
			for (int i = 0; i < vertex_connet_data.size(); i++)
			{
				if (vertex_connet_data[i].second > 0)
				{
					if (first_connet_idx == -1)
					{
						first_connet_idx = vertex_connet_data[i].first;
					}
					if (vertex_connet_data[i].second == 2 )
					{
						if (pre_point_idx > -1)
						{
							double angle = getAngleLeft(meshSrc->vertices[pre_point_idx], meshSrc->vertices[pre_point_idx], meshSrc->vertices[vertex_connet_data[i].first]);
							if (angle < min_angle)
							{
								min_angle_idx = vertex_connet_data[i].first;
								find_i = i;
							}
						}
						else
						{
							connet_idx = vertex_connet_data[i].first;
							vertex_connet_data[i].second = 0;
							std::vector<std::pair<uint32_t, int>>& vertex_connet_data_n = vertexConnectVertexData[connet_idx];
							for (int j = 0; j < vertex_connet_data_n.size(); j++)
							{
								if (vertex_connet_data_n[j].first == cur_idx)
								{
									vertex_connet_data_n[j].second = 0;
									break;
								}
							}
							break;
						}
					}
				}
			}
			if (min_angle_idx > -1)
			{
				connet_idx = min_angle_idx;
				vertex_connet_data[find_i].second = 0;
				std::vector<std::pair<uint32_t, int>>& vertex_connet_data_n = vertexConnectVertexData[connet_idx];
				for (int j = 0; j < vertex_connet_data_n.size(); j++)
				{
					if (vertex_connet_data_n[j].first == cur_idx)
					{
						vertex_connet_data_n[j].second = 0;
						break;
					}
				}
			}
			return connet_idx;
		};

		double SCALE = 1000.0;
		int face_size = faces.size();
		std::vector<bool> face_normal(face_size, false);
		for (int i = 0; i < face_size; i++)
		{
			const MeshFace& face = faces[i];
			trimesh::vec3& v0 = meshSrc->vertices.at(face.vertex_index[0]);
			trimesh::vec3& v1 = meshSrc->vertices.at(face.vertex_index[1]);
			trimesh::vec3& v2 = meshSrc->vertices.at(face.vertex_index[2]);

			trimesh::vec3 v01 = v1 - v0;
			trimesh::vec3 v02 = v2 - v0;
			trimesh::vec3 n = v01 TRICROSS v02;
			trimesh::normalize(n);
			face_normal[i] = n.z < 0;
		}
		std::vector<std::vector<std::pair<uint32_t, int>>> vertexConnectVertexData = generateVertexConnectVertexData();

		int vertex_size = vertexConnectVertexData.size();
		for (int i = 0; i < face_size; i++)
		{
			if (!face_normal[i]) continue;
			const MeshFace& face = faces[i];
			for (int j = 0; j < 3; j++)
			{
				if (face.connected_face_index[j] == -1 || !face_normal[face.connected_face_index[j]])
				{
					int current_idx = face.vertex_index[j];
					int next_idx = face.vertex_index[(j + 1) % 3];
					std::vector<std::pair<uint32_t, int>>& vertexData = vertexConnectVertexData[current_idx];
					for (int k = 0; k < vertexData.size(); k++)
					{
						if (vertexData[k].first == next_idx)
						{
							vertexData[k].second = 2;
							break;
						}
					}
					std::vector<std::pair<uint32_t, int>>& vertexData_n = vertexConnectVertexData[next_idx];
					for (int k = 0; k < vertexData_n.size(); k++)
					{
						if (vertexData_n[k].first == current_idx)
						{
							vertexData_n[k].second = 1;
							break;
						}							
					}
				}
			}
		}
		
		std::vector< std::vector<int>> polys_record;
		for (int i = 0; i < vertex_size; i++)
		{
			if (checkNoConnetion(vertexConnectVertexData[i]))
			{
				continue;
			}
			std::vector<int> poly_idx_record;
			poly_idx_record.reserve(vertex_size);;	
			int current_idx = i;
			int pre_point_idx = -1;
			while (1)
			{
				if (poly_idx_record.empty())
				{
					poly_idx_record.push_back(i);
				}
				else
				{
					pre_point_idx = poly_idx_record.back();
				}
				int next_idx = find_connet(vertexConnectVertexData, current_idx, pre_point_idx);
				if (next_idx == -1 || next_idx == i)
				{
					break;
				}
				poly_idx_record.push_back(next_idx);
				current_idx = next_idx;
			}
			if(poly_idx_record.size() > 2)
				polys_record.push_back(poly_idx_record);
		}

		ClipperLib::Paths paths;
		for (std::vector<int> poly_idx_record : polys_record)
		{
			ClipperLib::Path poly;
			for (int idx : poly_idx_record)
			{
				trimesh::vec3 pt_rot = (*rotation) * meshSrc->vertices[idx];
				poly.push_back(ClipperLib::IntPoint(pt_rot.x * SCALE, pt_rot.y * SCALE));
			}
			if (!ClipperLib::Orientation(poly))
				ClipperLib::ReversePath(poly);
			paths.push_back(poly);
		}

		ClipperLib::Clipper clipper;
		clipper.AddPaths(paths, ClipperLib::ptSubject, true);
		clipper.Execute(ClipperLib::ctUnion, paths, ClipperLib::pftPositive, ClipperLib::pftPositive);

		double max_area = 0;
		int max_idx = -1;
		int paths_size = paths.size();
		ClipperLib::Path path_max_area;
		std::vector<double> area_data(paths_size, 0);
		for (int i = 0; i < paths_size; i++)//find the MaxPolygon
		{
			const ClipperLib::Path& path = paths[i];
			double area = ClipperLib::Area(path);
			area_data[i] = area;
			if (area > max_area)
			{
				max_idx = i;
				max_area = area;
			}
		}
		if (max_idx > -1)
		{
			path_max_area.swap(paths[max_idx]);
		}		

		ClipperLib::Paths result_paths;
		result_paths.push_back(path_max_area);
		for (int i = 0; i < paths_size; i++)
		{
			const ClipperLib::Path& path = paths[i];
			if (area_data[i] < 0.05 * max_area) //remove small polygon
			{
				continue;
			}
			bool inMaxPolygon = true;
			for (const ClipperLib::IntPoint pt: path)
			{
				if (!ClipperLib::PointInPolygon(pt, path_max_area))
				{
					inMaxPolygon = false;
					break;
				}
			}
			if (!inMaxPolygon)//remove the polygon in MaxPolygon
			{
				result_paths.push_back(path);
			}
		}

		ClipperLib::Path polys_point;
		if (result_paths.size() > 1)
		{
			for (int i = 0; i < result_paths.size(); i++)
			{
				ClipperLib::Path& path = result_paths[i];
				addPoint(path, SCALE);
				if (!polys_point.empty())
				{
					ClipperLib::Path line;
					line.push_back(getMidPoint(result_paths[i - 1]));
					line.push_back(getMidPoint(path));
					addPoint(line, SCALE, true);
					polys_point.insert(polys_point.end(), line.begin(), line.end());
				}
				polys_point.insert(polys_point.end(), path.begin(), path.end());
			}
			polys_point = PolygonPro::polygonConcaveHull(polys_point);
		}
		else if(!result_paths.empty())
		{
			polys_point = result_paths[0];
		}
		
		ClipperLib::CleanPolygon(polys_point);
		polys_point = PolygonPro::polygonSimplyfy(polys_point, 0.1 * SCALE);		
		
		concave.clear();
		for (const ClipperLib::IntPoint& v : polys_point)
		{
			concave.push_back(trimesh::vec3(v.X / SCALE * scale.x, v.Y / SCALE * scale.y, 0.0f));
		}
	}
}