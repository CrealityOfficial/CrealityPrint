#include "renderprimitive.h"
#include "qtuser3d/geometry/bufferhelper.h"
#include "qtuser3d/geometry/geometrycreatehelper.h"

#include "qtuser3d/trimesh2/create.h"

namespace qtuser_3d
{
	static unsigned static_box_triangles_indices[36] = {
		2, 0, 3,
		2, 1, 0,
		0, 1, 5,
		0, 5, 4,
		1, 2, 6,
		1, 6, 5,
		2, 3, 7,
		2, 7, 6,
		3, 0, 4,
		3, 4, 7,
		4, 5, 6,
		4, 6, 7
	};

	static unsigned static_box_part_indices[48] = {
		0, 1,
		0, 2,
		0, 3,

		4, 5,
		4, 6,
		4, 7,

		8, 9,
		8, 10,
		8, 11,

		12, 13,
		12, 14,
		12, 15,

		16, 17,
		16, 18,
		16, 19,

		20, 21,
		20, 22,
		20, 23,

		24, 25,
		24, 26,
		24, 27,

		28, 29,
		28, 30,
		28, 31

	};

	Qt3DRender::QGeometry* createLinesGeometry(trimesh::vec3* lines, int num)
	{
		if (num == 0 || !lines)
			return nullptr;

		Qt3DRender::QAttribute* positionAttribute = qtuser_3d::BufferHelper::CreateVertexAttribute(
			(const char*)lines, num, 3, Qt3DRender::QAttribute::defaultPositionAttributeName());

		return qtuser_3d::GeometryCreateHelper::create(nullptr, positionAttribute);
	}

	Qt3DRender::QGeometry* createIndicesGeometry(trimesh::vec3* positions, int num, int* indices, int tnum)
	{
		if (num == 0 || !positions)
			return nullptr;

		Qt3DRender::QAttribute* positionAttribute = qtuser_3d::BufferHelper::CreateVertexAttribute(
			(const char*)positions, num, 3, Qt3DRender::QAttribute::defaultPositionAttributeName());
		Qt3DRender::QAttribute* indexAttribute = nullptr;
		if(tnum > 0 && indices)
			indexAttribute = qtuser_3d::BufferHelper::CreateIndexAttribute((const char*)indices, tnum);

		return qtuser_3d::GeometryCreateHelper::create(nullptr, positionAttribute, indexAttribute);
	}

	Qt3DRender::QGeometry* createLinesGeometry(const std::vector<trimesh::vec3>& lines)
	{
		int size = (int)lines.size();
		return createLinesGeometry((size > 0 ? (trimesh::vec3*)&lines.at(0) : nullptr), size);
	}

	Qt3DRender::QGeometry* createColorLinesGeometry(const std::vector<trimesh::vec3>& lines, const std::vector<trimesh::vec4>& colors)
	{
		if (lines.size() <= 0 || colors.size() <= 0)
			return nullptr;

		int posNum = lines.size();
		int colorNum = colors.size();

		Qt3DRender::QAttribute* positionAttribute = qtuser_3d::BufferHelper::CreateVertexAttribute(
			(const char*)lines.data(), posNum, 3, Qt3DRender::QAttribute::defaultPositionAttributeName());

		Qt3DRender::QAttribute* colorAttribute = qtuser_3d::BufferHelper::CreateVertexAttribute(
			(const char*)colors.data(), colorNum, 4, Qt3DRender::QAttribute::defaultColorAttributeName());

		return GeometryCreateHelper::create(nullptr, positionAttribute, colorAttribute);
	}

	Qt3DRender::QGeometry* createCubeLines(const trimesh::box3& box, const CubeParameter& parameter)
	{
		if (parameter.parts)
		{
			auto genDatasFromCorner = [](std::vector<trimesh::vec3>& positions, int pos,
				const trimesh::vec3& boxsz, const trimesh::vec3& dir)
			{
				positions[pos + 1 + 0] = positions[pos] + trimesh::vec3(boxsz.x * dir.x, 0.0f, 0.0f);
				positions[pos + 1 + 1] = positions[pos] + trimesh::vec3(0.0f, boxsz.y * dir.y, 0.0f);
				positions[pos + 1 + 2] = positions[pos] + trimesh::vec3(0.0f, 0.0f, boxsz.z * dir.z);
			};

			std::vector<trimesh::vec3> positions;
			positions.resize(32);

			trimesh::vec3 bmin = box.min;
			trimesh::vec3 bmax = box.max;

			trimesh::vec3 boxsz = (bmax - bmin) * (parameter.ratio / 2.0f);

			positions[0] = bmin;
			genDatasFromCorner(positions, 0, boxsz, trimesh::vec3(1.0f, 1.0f, 1.0f));

			positions[4] = trimesh::vec3(bmax.x, bmin.y, bmin.z);
			genDatasFromCorner(positions, 4, boxsz, trimesh::vec3(-1.0f, 1.0f, 1.0f));

			positions[8] = trimesh::vec3(bmax.x, bmax.y, bmin.z);
			genDatasFromCorner(positions, 8, boxsz, trimesh::vec3(-1.0f, -1.0f, 1.0f));

			positions[12] = trimesh::vec3(bmin.x, bmax.y, bmin.z);
			genDatasFromCorner(positions, 12, boxsz, trimesh::vec3(1.0f, -1.0f, 1.0f));

			positions[16] = trimesh::vec3(bmin.x, bmin.y, bmax.z);
			genDatasFromCorner(positions, 16, boxsz, trimesh::vec3(1.0f, 1.0f, -1.0f));

			positions[20] = trimesh::vec3(bmax.x, bmin.y, bmax.z);
			genDatasFromCorner(positions, 20, boxsz, trimesh::vec3(-1.0f, 1.0f, -1.0f));

			positions[24] = bmax;
			genDatasFromCorner(positions, 24, boxsz, trimesh::vec3(-1.0f, -1.0f, -1.0f));

			positions[28] = trimesh::vec3(bmin.x, bmax.y, bmax.z);
			genDatasFromCorner(positions, 28, boxsz, trimesh::vec3(1.0f, -1.0f, -1.0f));

			return createIndicesGeometry(positions.data(), 32, (int*)static_box_part_indices, 48);;
		}

		std::vector<trimesh::vec3> lines;
		boxLines(box, lines);

		return createLinesGeometry(lines);
	}

	Qt3DRender::QGeometry* createCubeTriangles(const trimesh::box3& box)
	{
		std::vector<trimesh::vec3> positions;
		positions.resize(8);

		trimesh::vec3 bmin = box.min;
		trimesh::vec3 bmax = box.max;

		positions[0] = bmin;
		positions[1] = trimesh::vec3(bmax.x, bmin.y, bmin.z);
		positions[2] = trimesh::vec3(bmax.x, bmax.y, bmin.z);
		positions[3] = trimesh::vec3(bmin.x, bmax.y, bmin.z);
		positions[4] = trimesh::vec3(bmin.x, bmin.y, bmax.z);
		positions[5] = trimesh::vec3(bmax.x, bmin.y, bmax.z);
		positions[6] = bmax;
		positions[7] = trimesh::vec3(bmin.x, bmax.y, bmax.z);

		return createIndicesGeometry(positions.data(), 24, (int*)static_box_triangles_indices, 36);
	}

	Qt3DRender::QGeometry* createGridLines(const GridParameter& parameter)
	{
		std::vector<trimesh::vec3> lines;
		int n = parameter.xNum + parameter.yNum + 2;
		if (n < 2)
			return nullptr;

		int nn = 2 * n;
		lines.reserve(nn);

		float xmax = parameter.delta * (float)parameter.xNum;
		float ymax = parameter.delta * (float)parameter.yNum;
		for (int i = 0; i <= parameter.xNum; ++i)
		{
			float x = parameter.delta * (float)i;
			lines.push_back(trimesh::vec3(x, 0.0f, 0.0f));
			lines.push_back(trimesh::vec3(x, ymax, 0.0f));
		}

		for (int i = 0; i <= parameter.yNum; ++i)
		{
			float y = parameter.delta * (float)i;
			lines.push_back(trimesh::vec3(0.0f, y, 0.0f));
			lines.push_back(trimesh::vec3(xmax, y, 0.0f));
		}

		offsetPoints(lines, parameter.offset);
		return createLinesGeometry(lines);
	}

	Qt3DRender::QGeometry* createMidGridLines(const trimesh::box3& box, float gap, float offset)
	{
		trimesh::vec3 size = box.size();
		if (size.x == 0.0f || size.y == 0.0f)
			return nullptr;

		float minX = box.min.x;
		float maxX = box.max.x;
		float minY = box.min.y;
		float maxY = box.max.y;

		if (maxX - minX <= 0 || maxY - minY <= 0)
		{
			return nullptr;
		}

		std::vector<trimesh::vec3> positions;
		std::vector<trimesh::vec2> flags;

		float zcoord = 0.0f;
		float midX = (minX + maxX) / 2.0f;
		float midY = (minY + maxY) / 2.0f;

		int vertexCount = 0;

		float k = 0.0f;
		for (float i = midX; i <= maxX; i += gap)
		{
			positions.push_back(trimesh::vec3(i, minY, zcoord));
			positions.push_back(trimesh::vec3(i, maxY, zcoord));

			flags.push_back(trimesh::vec2(k, 1.0f));
			flags.push_back(trimesh::vec2(k, 1.0f));

			k += gap;
			vertexCount += 2;
		}
		k = gap;
		for (float i = midX - gap; i >= minX; i -= gap)
		{
			positions.push_back(trimesh::vec3(i, minY, zcoord));
			positions.push_back(trimesh::vec3(i, maxY, zcoord));

			flags.push_back(trimesh::vec2(k, 1.0f));
			flags.push_back(trimesh::vec2(k, 1.0f));

			k += gap;
			vertexCount += 2;
		}

		k = 0;
		for (float i = midY; i <= maxY; i += gap)
		{
			positions.push_back(trimesh::vec3(minX, i, zcoord));
			positions.push_back(trimesh::vec3(maxX, i, zcoord));

			flags.push_back(trimesh::vec2(1.0f, k));
			flags.push_back(trimesh::vec2(1.0f, k));

			k += gap;
			vertexCount += 2;
		}
		k = gap;
		for (float i = midY - gap; i >= minY; i -= gap)
		{
			positions.push_back(trimesh::vec3(minX, i, zcoord));
			positions.push_back(trimesh::vec3(maxX, i, zcoord));

			flags.push_back(trimesh::vec2(1.0f, k));
			flags.push_back(trimesh::vec2(1.0f, k));

			k += gap;
			vertexCount += 2;
		}

		Qt3DRender::QAttribute* positionAttribute = qtuser_3d::BufferHelper::CreateVertexAttribute((const char*)&positions.at(0), qtuser_3d::AttribueSlot::Position, vertexCount);
		Qt3DRender::QAttribute* flagAttribute = qtuser_3d::BufferHelper::CreateVertexAttribute("vertexFlag", (const char*)&flags.at(0), 2, vertexCount);
		return qtuser_3d::GeometryCreateHelper::create(nullptr, positionAttribute, flagAttribute);
	}

	Qt3DRender::QGeometry* createIdentityTriangle()
	{
		float static_triangle_position[9] = {
			1.0f, 0.0f, 0.0f,
			-1.0f, 0.0f, 0.0f,
			0.0f, -1.0f, 0.0f,
		};

		return createIndicesGeometry((trimesh::vec3*)static_triangle_position, 3, nullptr, 0);
	}

	Qt3DRender::QGeometry* createSimpleTriangle(const trimesh::vec3& v1, const trimesh::vec3& v2, const trimesh::vec3& v3)
	{
		std::vector<trimesh::vec3> positions;
		positions.push_back(v1);
		positions.push_back(v2);
		positions.push_back(v3);
		return createIndicesGeometry((trimesh::vec3*)positions.data(), 3, nullptr, 0);
	}

	Qt3DRender::QGeometry* createSimpleQuad()
	{
		//	float quadVertices[] = {   // Vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
		//// Positions   // TexCoords
		//-1.0f,  1.0f,  0.0f, 1.0f,
		//-1.0f, -1.0f,  0.0f, 0.0f,
		// 1.0f, -1.0f,  1.0f, 0.0f,

		//-1.0f,  1.0f,  0.0f, 1.0f,
		// 1.0f, -1.0f,  1.0f, 0.0f,
		// 1.0f,  1.0f,  1.0f, 1.0f
		//	};

		float quadVertices[] = {
			// Positions  
			-1.0f,  1.0f, 0.0f,
			-1.0f, -1.0f, 0.0f,
			1.0f, -1.0f,  0.0f,

			-1.0f,  1.0f,  0.0f,
			1.0f,  -1.0f,  0.0f,
			1.0f,  1.0f,   0.0f,
		};

		//float quadVertices[] = {
		//	// Positions  
		//	-0.5f,  0.5f, 0.0f,
		//	-0.5f, -0.5f, 0.0f,
		//	0.5f, -0.5f,  0.0f,

		//	-0.5f,  0.5f,  0.0f,
		//	0.5f,  -0.5f,  0.0f,
		//	0.5f,  0.5f,   0.0f,
		//};

		//float quadTexcoords[] = {
		//	// TexCoords  
		//	0.0f,  1.0f,
		//	0.0f,  0.0f,
		//	1.0f,  0.0f,

		//	0.0f,  1.0f,
		//	1.0f,  0.0f,
		//	1.0f,  1.0f,
		//};

		float quadTexcoords[] = {
			// TexCoords  
			0.0f,  995.0f,
			0.0f,  0.0f,
			1920.0f,  0.0f,

			0.0f,  995.0f,
			1920.0f,  0.0f,
			1920.0f,  995.0f,
		};

		QByteArray posArray;
		int lenPos = sizeof(quadVertices);
		posArray.resize(lenPos);
		memcpy(posArray.data(), &quadVertices, lenPos);
		Qt3DRender::QBuffer* positionBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
		positionBuffer->setData(posArray);

		Qt3DRender::QAttribute* positionAttribute = new Qt3DRender::QAttribute(positionBuffer, Qt3DRender::QAttribute::defaultPositionAttributeName(), Qt3DRender::QAttribute::Float, 3, 6);

		Qt3DRender::QAttribute* texcoordAttribute = nullptr;
		Qt3DRender::QBuffer* texcoordBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
		QByteArray texcoordArray;
		int lenTex = sizeof(quadTexcoords);
		texcoordArray.resize(lenTex);
		memcpy(texcoordArray.data(), &quadTexcoords, lenTex);
		texcoordBuffer->setData(texcoordArray);
		texcoordAttribute = new Qt3DRender::QAttribute(texcoordBuffer, Qt3DRender::QAttribute::defaultTextureCoordinateAttributeName(), Qt3DRender::QAttribute::Float, 2, 6);

		return qtuser_3d::GeometryCreateHelper::create(nullptr, positionAttribute, texcoordAttribute);
	}

	Qt3DRender::QGeometry* createTriangles(const std::vector<trimesh::vec3>& tris)
	{
		return createIndicesGeometry((trimesh::vec3*)tris.data(), (int)tris.size(), nullptr, 0);
	}

	Qt3DRender::QGeometry* createPoints(const std::vector<trimesh::vec3>& points)
	{
		return createIndicesGeometry((trimesh::vec3*)points.data(), (int)points.size(), nullptr, 0);
	}

	Qt3DRender::QGeometry* createTrianglesWithNormals(const std::vector<trimesh::vec3>& tris, const std::vector<trimesh::vec3>& normals)
	{
		if (tris.size() <= 0 || normals.size() <= 0)
			return nullptr;

		trimesh::vec3* positions = (trimesh::vec3*)tris.data();
		int posNum = tris.size();

		trimesh::vec3* norms = (trimesh::vec3*)normals.data();
		int normalNum = normals.size();

		if (!positions || !norms)
			return nullptr;

		Qt3DRender::QAttribute* positionAttribute = qtuser_3d::BufferHelper::CreateVertexAttribute(
			(const char*)positions, posNum, 3, Qt3DRender::QAttribute::defaultPositionAttributeName());

		Qt3DRender::QAttribute* normalAttribute = qtuser_3d::BufferHelper::CreateVertexAttribute(
			(const char*)norms, normalNum, 3, Qt3DRender::QAttribute::defaultNormalAttributeName());

		return GeometryCreateHelper::create(nullptr, positionAttribute, normalAttribute);
	}

	Qt3DRender::QGeometry* createTrianglesWithColors(const std::vector<trimesh::vec3>& tris, const std::vector<trimesh::vec3>& colors)
	{
		if (tris.size() <= 0 || colors.size() <= 0)
			return nullptr;

		int posNum = tris.size();
		int colorNum = colors.size();

		Qt3DRender::QAttribute* positionAttribute = qtuser_3d::BufferHelper::CreateVertexAttribute(
			(const char*)tris.data(), posNum, 3, Qt3DRender::QAttribute::defaultPositionAttributeName());

		Qt3DRender::QAttribute* normalAttribute = qtuser_3d::BufferHelper::CreateVertexAttribute(
			(const char*)colors.data(), colorNum, 3, Qt3DRender::QAttribute::defaultColorAttributeName());

		return GeometryCreateHelper::create(nullptr, positionAttribute, normalAttribute);
	}
	
	Qt3DRender::QGeometry* createTrianglesWithFlags(const std::vector<trimesh::vec3>& tris, const std::vector<float>& flags)
	{
		if (tris.size() <= 0 || flags.size() <= 0)
			return nullptr;

		int posNum = tris.size();
		int flagNum = flags.size();

		Qt3DRender::QAttribute* positionAttribute = qtuser_3d::BufferHelper::CreateVertexAttribute(
			(const char*)tris.data(), posNum, 3, Qt3DRender::QAttribute::defaultPositionAttributeName());

		Qt3DRender::QAttribute* flagAttribute = qtuser_3d::BufferHelper::CreateVertexAttribute(
			(const char*)flags.data(), flagNum, 1, "vertexFlag");

		return GeometryCreateHelper::create(nullptr, positionAttribute, flagAttribute);
	}
}