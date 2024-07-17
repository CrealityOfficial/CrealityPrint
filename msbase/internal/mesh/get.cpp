#include "msbase/mesh/get.h"
#include "ccglobal/log.h"

#include <limits.h>
#include <math.h>
#include <set>
#include <map>

namespace msbase
{
	constexpr double _EPSILON = 1e-4;
	const std::map<float, float> _min_depth_per_height = {
		{100.f, 20.f}, {250.f, 40.f}
	};

	trimesh::vec3 getFaceNormal(trimesh::TriMesh* mesh, int faceIndex, bool normalized)
	{
		trimesh::vec3 n = trimesh::vec3(0.0f, 0.0f, 0.0f);
		if (mesh && faceIndex >= 0 && faceIndex < (int)mesh->faces.size())
		{
			const trimesh::TriMesh::Face& f = mesh->faces.at(faceIndex);

			trimesh::vec3 v01 = mesh->vertices.at(f[1]) - mesh->vertices.at(f[0]);
			trimesh::vec3 v02 = mesh->vertices.at(f[2]) - mesh->vertices.at(f[0]);
			n = v01 TRICROSS v02;
			if(normalized)
				trimesh::normalize(n);
		}
		else
		{
			LOGW("msbase::getFaceNormal : error input.");
		}
		return n;
	}

	void calculateFaceNormalOrAreas(trimesh::TriMesh* mesh, 
		std::vector<trimesh::vec3>& normals, std::vector<float>* areas)
	{
		if (!mesh || mesh->faces.size() == 0)
		{
			LOGW("msbase::getFaceNormal : error input.");
			return;
		}

		const int faceNum = mesh->faces.size();
		normals.resize(faceNum);

		for (int i = 0; i < faceNum; ++i) {
			normals.at(i) = getFaceNormal(mesh, i, false);
		}

		//std::transform
		if (areas) {
			areas->resize(faceNum, 0.0f);
			for (int i = 0; i < faceNum; ++i) {
				areas->at(i) = (trimesh::len(normals.at(i)) / 2.0);
			}
		}

		for (auto& n : normals) {
			trimesh::normalize(n);
		}
	}


	double count_triangle_area(const trimesh::point& a, const trimesh::point& b, const trimesh::point& c) {
		double area = -1;

		double side[3];

		side[0] = sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2) + pow(a.z - b.z, 2));
		side[1] = sqrt(pow(a.x - c.x, 2) + pow(a.y - c.y, 2) + pow(a.z - c.z, 2));
		side[2] = sqrt(pow(c.x - b.x, 2) + pow(c.y - b.y, 2) + pow(c.z - b.z, 2));

		if (side[0] + side[1] <= side[2] || side[0] + side[2] <= side[1] || side[1] + side[2] <= side[0]) return area;

		double p = (side[0] + side[1] + side[2]) / 2;
		area = sqrt(p * (p - side[0]) * (p - side[1]) * (p - side[2]));

		return area;
	}

	double getFacetArea(trimesh::TriMesh* mesh, int facetId)
	{
		if (!mesh || mesh->faces.size() == 0)
		{
			return 0.0f;
		}

		double s = 0.0f;
		std::vector<trimesh::point>& vs = mesh->vertices;
		if (mesh->faces.size() <= facetId)
			return 0.0f;

		trimesh::TriMesh::Face& face = mesh->faces[facetId];

		if (vs.size() <= face[0] || vs.size() <= face[1] || vs.size() <= face[2])
			return 0.0f;

		return count_triangle_area(vs[face[0]], vs[face[1]], vs[face[2]]);
	}

	trimesh::vec3 estimate_wipe_tower_size(const double w, const double wipe_volume
		, std::vector<int>& plate_extruders, double layer_height
		, double max_height)
	{
		trimesh::vec3 wipe_tower_size;
		wipe_tower_size.set(0,0,0);
		wipe_tower_size.x = w;

		// empty plate
		if (plate_extruders.empty())
			return wipe_tower_size;

		wipe_tower_size.z = max_height;
		auto timelapse_type = 0;
		bool timelapse_enabled =  false;

		double depth = wipe_volume * (plate_extruders.size() - 1) / (layer_height * w);
		if (timelapse_enabled || depth > _EPSILON) {
			float min_wipe_tower_depth = 0.f;
			auto iter = _min_depth_per_height.begin();
			while (iter != _min_depth_per_height.end()) {
				auto curr_height_to_depth = *iter;

				// This is the case that wipe tower height is lower than the first min_depth_to_height member.
				if (curr_height_to_depth.first >= max_height) {
					min_wipe_tower_depth = curr_height_to_depth.second;
					break;
				}

				iter++;

				// If curr_height_to_depth is the last member, use its min_depth.
				if (iter == _min_depth_per_height.end()) {
					min_wipe_tower_depth = curr_height_to_depth.second;
					break;
				}

				// If wipe tower height is between the current and next member, set the min_depth as linear interpolation between them
				auto next_height_to_depth = *iter;
				if (next_height_to_depth.first > max_height) {
					float height_base = curr_height_to_depth.first;
					float height_diff = next_height_to_depth.first - curr_height_to_depth.first;
					float min_depth_base = curr_height_to_depth.second;
					float depth_diff = next_height_to_depth.second - curr_height_to_depth.second;

					min_wipe_tower_depth = min_depth_base + (max_height - curr_height_to_depth.first) / height_diff * depth_diff;
					break;
				}
			}
			depth = std::max((double)min_wipe_tower_depth, depth);
		}
		wipe_tower_size.y = depth;
		return wipe_tower_size;
	}
}