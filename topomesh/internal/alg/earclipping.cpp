#include "earclipping.h"


#ifndef EPS
#define EPS 1e-8f
#endif 

namespace topomesh {
	EarClipping::EarClipping(std::vector<std::pair<trimesh::point, int>>& data)
	{
		while (!data.empty()) 
		{
			int before = data.size();
			for (int i = 0; i < data.size(); i++)
			{
				int size = data.size();
				if (data.size() == 3)
				{
					_result.push_back(trimesh::ivec3(data[0].second, data[1].second, data[2].second));
					data.clear();
					break;
				}
				//trimesh::point b1 = trimesh::normalized(data[(i + 1) % size].first - data[i].first);
				//trimesh::point b2 = trimesh::normalized(data[(i + size - 1) % size].first - data[i].first);
				float b = trimesh::point((data[(i + 1) % size].first - data[i].first) % (data[(i + size - 1) % size].first - data[i].first)).z;
				float a = trimesh::normalized(trimesh::point(data[(i + 1)%size].first - data[i].first)) ^ trimesh::normalized(trimesh::point(data[(i + size - 1)%size].first - data[i].first));
				//if(a<=-0.9999f)
				if (a <= -0.99999f || b < 0.f)
					continue;
				std::vector<trimesh::vec2> triangle = { trimesh::vec2(data[i].first.x,data[i].first.y),trimesh::vec2(data[(i + 1) % size].first.x, data[(i + 1) % size].first.y),
				trimesh::vec2(data[(i + size - 1) % size].first.x, data[(i + size - 1) % size].first.y) };

				bool pass = false;
				for (int j = 0; j < size; j++)
				{
					int RightrayCorssPoint = 0; int LeftrayCrossPoint = 0;
					if (j != i && j != ((i + 1) % size) && j != ((i + size - 1) % size)
						&&data[j].second!=data[i].second
						&& data[j].second != data[(i+1)%size].second
						&& data[j].second != data[(i+size-1)%size].second)
					{
						for (int k = 0; k < triangle.size(); k++)
						{
							double ty_abs = std::abs(triangle[k].y - triangle[(k + 1) % 3].y);
							double yy_abs = std::abs(triangle[k].y - data[j].first.y);
							if (ty_abs < 10 * EPS && yy_abs < 10 * EPS &&
								data[j].first.x>std::min(triangle[k].x, triangle[(k + 1) % 3].x) &&
								data[j].first.x < std::max(triangle[k].x, triangle[(k + 1) % 3].x)) {
								pass = true; break;
							}
							if (std::abs(triangle[k].y - triangle[(k + 1) % 3].y) < EPS) continue;
							if (data[j].first.y < std::min(triangle[k].y, triangle[(k + 1) % 3].y))continue;
							if (data[j].first.y > std::max(triangle[k].y, triangle[(k + 1) % 3].y)) continue;
							double x = (data[j].first.y - triangle[k].y) * (triangle[(k + 1) % 3].x - triangle[k].x) / (triangle[(k + 1) % 3].y - triangle[k].y) + triangle[k].x;
							double x_abs = std::abs(x - data[j].first.x);
							if (x_abs < 10 * EPS) { pass = true; break; }
							if (x - data[j].first.x <= 0)
							{
								RightrayCorssPoint++;
							}
							else if (x - data[j].first.x >= 0)
							{
								LeftrayCrossPoint++;
							}
						}
					}
					if (RightrayCorssPoint > 0 && LeftrayCrossPoint > 0)
					{
						pass = true; break;
					}
				}
				if (pass)
					continue;
				else {
					_result.push_back(trimesh::ivec3(data[i].second, data[(i + 1) % size].second, data[(i + size - 1) % size].second));
					data.erase(data.begin() + i);
					i--;
				}
			}	
			if (before == data.size())
			{
				/*float k = data[0].first.y / data[0].first.x;
				std::vector<int> triangle(data.size(), 0);
				triangle[0] = 1;
				for (int i = 1; i < data.size(); i++)
				{
					if (k - (data[i].first.y / data[i].first.x) < 1e-7)
						triangle[i] = 1;
				}
				for (int i = 1; i < triangle.size() - 1; i++)
				{
					if (triangle[i] == 1 && triangle[i + 1] == 1)						
						_result.push_back(trimesh::ivec3(data[0].second, data[i].second, data[(i+1)%data.size()].second));
				}*/
				break;
			}
		}
	};
}