#include "roundedrectanglehelper.h"

namespace qtuser_3d {

	void makeQuad(trimesh::vec3 a, trimesh::vec3 b, std::vector<trimesh::vec3>* vertex)
	{
		vertex->push_back(a);
		vertex->push_back(trimesh::vec3(b.x, a.y, a.z));
		vertex->push_back(b);

		vertex->push_back(a);
		vertex->push_back(b);
		vertex->push_back(trimesh::vec3(a.x, b.y, a.z));
	}

	void makeCircle(trimesh::vec3 o, float radius, std::vector<trimesh::vec3>* vertex)
	{
		int iter = 36;
		float pcs = M_2PIf / float(iter);

		for (size_t i = 0; i < iter; i++)
		{
			float start = i * pcs;
			float end = (i + 1) * pcs;

			vertex->push_back(o);
			vertex->push_back(o + trimesh::vec3(cos(start) * radius, sin(start) * radius, 0.0f));
			vertex->push_back(o + trimesh::vec3(cos(end) * radius, sin(end) * radius, 0.0f));
		}
	}

	void makeTextureCoordinate(const std::vector<trimesh::vec3>* vertex, trimesh::vec3 o, trimesh::vec2 size, std::vector<trimesh::vec2>* uvs)
	{
		if (vertex == nullptr || vertex->size() == 0 || uvs == nullptr)
			return;

		for (size_t i = 0; i < vertex->size(); i++)
		{
			const trimesh::vec3& sub = vertex->at(i);
			trimesh::vec2 uv = trimesh::vec2((sub.x - o.x)/size.x, (sub.y - o.y) / size.y);
			uvs->push_back(uv);
		}
	}

	void makeFan(trimesh::vec3 center, int iter, float R, float startRadian, float endRadian, std::vector<trimesh::vec3>* vertex)
	{
		assert(startRadian < endRadian);
		float pcs = (endRadian - startRadian) / float(iter);
		float radian = startRadian;

		for (size_t i = 0; i < iter; i++)
		{
			trimesh::vec3 p0 = center + trimesh::vec3(R * cos(radian), R * sin(radian), 0.0f);
			radian += pcs;
			trimesh::vec3 p1 = center + trimesh::vec3(R * cos(radian), R * sin(radian), 0.0f);

			vertex->push_back(center);
			vertex->push_back(p0);
			vertex->push_back(p1);
		}
	}
	
	void RoundedRectangle::makeRoundedRectangle(trimesh::vec3 origin, float width, float height, RoundedRectangleCorners corners, float radius, std::vector<trimesh::vec3> *vertex, std::vector<trimesh::vec2>* uvs)
	{
		if (vertex == nullptr)
			return;

		float H = origin.z;
		
		//center
		{
			trimesh::vec3 min = origin + trimesh::vec3(radius, radius, 0.0f);
			trimesh::vec3 max = origin + trimesh::vec3(width, height, 0.0f) - trimesh::vec3(radius, radius, 0.0f);
			makeQuad(min, max, vertex);
		}

		//left
		{
			trimesh::vec3 min = origin + trimesh::vec3(0.0f, radius, 0.0f);
			trimesh::vec3 max = trimesh::vec3(origin.x + radius, origin.y + height - radius, H);
			makeQuad(min, max, vertex);
		}

		//right
		{
			trimesh::vec3 min = trimesh::vec3(origin.x + width - radius, origin.y + radius, H);
			trimesh::vec3 max = trimesh::vec3(origin.x + width, origin.y + height - radius, H);
			makeQuad(min, max, vertex);
		}

		//top
		{
			trimesh::vec3 min = trimesh::vec3(origin.x + radius, origin.y + height - radius, H);
			trimesh::vec3 max = trimesh::vec3(origin.x + width - radius, origin.y + height, H);
			makeQuad(min, max, vertex);
		}

		//bottom
		{
			trimesh::vec3 min = origin + trimesh::vec3(radius, 0.0f, 0.0f);
			trimesh::vec3 max = trimesh::vec3(origin.x + width - radius, origin.y + radius, H);
			makeQuad(min, max, vertex);
		}

		if (corners & Corner::TopLeft)
		{
			trimesh::vec3 center = trimesh::vec3(origin.x + radius, origin.y + height - radius, H);
			int iter = 10;
			float radian = M_PI_2f;
			makeFan(center, iter, radius, radian, radian + M_PI_2f, vertex);
		}
		else {
			trimesh::vec3 min = trimesh::vec3(origin.x, origin.y + height - radius, H);
			trimesh::vec3 max = min + trimesh::vec3(radius, radius, 0.0f);
			makeQuad(min, max, vertex);
		}

		if (corners & Corner::TopRight)
		{
			trimesh::vec3 center = trimesh::vec3(origin.x + width - radius, origin.y + height - radius, H);
			int iter = 10;
			float radian = 0.0f;
			makeFan(center, iter, radius, radian, radian+ M_PI_2f, vertex);
		}
		else {

			trimesh::vec3 min = trimesh::vec3(origin.x + width - radius, origin.y + height - radius, H);
			trimesh::vec3 max = min + trimesh::vec3(radius, radius, 0.0f);
			makeQuad(min, max, vertex);
		}

		if (corners & Corner::BottomLeft)
		{
			trimesh::vec3 center = origin + trimesh::vec3(radius, radius, 0.0f);
			int iter = 10;
			float radian = M_PIf;
			makeFan(center, iter, radius, radian, radian + M_PI_2f, vertex);
		}
		else {
			trimesh::vec3 min = origin;
			trimesh::vec3 max = origin + trimesh::vec3(radius, radius, 0.0f);
			makeQuad(min, max, vertex);
		}

		if (corners & Corner::BottomRight)
		{
			trimesh::vec3 center = trimesh::vec3(origin.x + width - radius, origin.y + radius, H);
			int iter = 10;
			float radian = M_PIf + M_PI_2f;
			makeFan(center, iter, radius, radian, radian + M_PI_2f, vertex);
		}
		else {
			trimesh::vec3 min = trimesh::vec3(origin.x + width - radius, origin.y, H);
			trimesh::vec3 max = min + trimesh::vec3(radius, radius, 0.0f);
			makeQuad(min, max, vertex);
		}
		
		if (uvs == nullptr)
			return;
		
		uvs->push_back(trimesh::vec2(0.0, 0.0));
		uvs->push_back(trimesh::vec2(1.0, 0.0));
		uvs->push_back(trimesh::vec2(1.0, 1.0));
		uvs->push_back(trimesh::vec2(0.0, 0.0));
		uvs->push_back(trimesh::vec2(1.0, 1.0));
		uvs->push_back(trimesh::vec2(0.0, 1.0));

		for (size_t i = uvs->size(); i < vertex->size(); i++)
		{
			uvs->push_back(trimesh::vec2(-1.0, -1.0));
		}
	}
};
