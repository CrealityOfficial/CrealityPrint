#include "meshbuilder.h"
#include "meshobject.h"
#include "hashmeshbuilder.h"

namespace topomesh
{
	MeshObject* meshFromTriangleSoup(const std::vector<Point3>& soup)
	{
		HashMeshBuilder builder;

		size_t size = soup.size() / 3;
		for (size_t i = 0; i < size; ++i)
		{
			int index = 3 * i;
			builder.addFace(soup.at(index), soup.at(index + 1), soup.at(index + 2));
		}

		MeshObject* mesh = builder.build();
		if (mesh) mesh->calculateBox();
		return mesh;
	}

    MeshObject* meshFromTriangleSoup(float* vertex, int vertexNum)
    {
        HashMeshBuilder builder;

        size_t size = vertexNum / 3;
        for (size_t i = 0; i < size; ++i)
        {
            float* v = vertex + 9 * i;
            Point3 v1 = Point3(DLP_MM2_S(1.0 * *v), DLP_MM2_S(1.0 * *(v + 1)), DLP_MM2_S(1.0 * *(v + 2)));
            Point3 v2 = Point3(DLP_MM2_S(1.0 * *(v + 3)), DLP_MM2_S(1.0 * *(v + 4)), DLP_MM2_S(1.0 * *(v + 5)));
            Point3 v3 = Point3(DLP_MM2_S(1.0 * *(v + 6)), DLP_MM2_S(1.0 * *(v + 7)), DLP_MM2_S(1.0 * *(v + 8)));
            builder.addFace(v1, v2, v3);
        }

        MeshObject* mesh = builder.build();
        if (mesh) mesh->calculateBox();
        return mesh;
    }

    MeshObject* meshFromTrimesh(trimesh::TriMesh* mesh)
    {
        if (!mesh || mesh->vertices.size() == 0 || mesh->faces.size() == 0)
            return nullptr;

        return meshFromTrimesh((float*)&mesh->vertices.at(0), (int)mesh->faces.size(), (int*)&mesh->faces.at(0));
    }

    MeshObject* meshFromTrimesh(float* vertex, int faceNum, int* faceIndex)
    {
        HashMeshBuilder builder;

        for (size_t i = 0; i < faceNum; ++i)
        {
            int* face = faceIndex + 3 * i;
            float* v1 = vertex + 3 * *(face);
            float* v2 = vertex + 3 * *(face + 1);
            float* v3 = vertex + 3 * *(face + 2);
            Point3 p1 = Point3(DLP_MM2_S(1.0 * *v1), DLP_MM2_S(1.0 * *(v1 + 1)), DLP_MM2_S(1.0 * *(v1 + 2)));
            Point3 p2 = Point3(DLP_MM2_S(1.0 * *v2), DLP_MM2_S(1.0 * *(v2 + 1)), DLP_MM2_S(1.0 * *(v2 + 2)));
            Point3 p3 = Point3(DLP_MM2_S(1.0 * *v3), DLP_MM2_S(1.0 * *(v3 + 1)), DLP_MM2_S(1.0 * *(v3 + 2)));
            builder.addFace(p1, p2, p3);
        }

        MeshObject* mesh = builder.build();
        if (mesh) mesh->calculateBox();
        return mesh;
    }

	Mesh* meshFromTrimesh2(int vertexNum, float* vertex, int faceNum, int* faceIndex)
	{
		HashMeshBuilder builder;

		for (size_t i = 0; i < faceNum; ++i)
		{
			int* face = faceIndex + 3 * i;
			int index1 = *(face);
			int index2 = *(face + 1);
			int index3 = *(face + 2);
			if (index1 >= 0 && index1 < vertexNum && index2 >= 0 && index2 < vertexNum
				&& index3 >= 0 && index3 < vertexNum)
			{
				float* v1 = vertex + 3 * index1;
				float* v2 = vertex + 3 * index2;
				float* v3 = vertex + 3 * index3;
				Point3 p1 = Point3(DLP_MM2_S(1.0 * *v1), DLP_MM2_S(1.0 * *(v1 + 1)), DLP_MM2_S(1.0 * *(v1 + 2)));
				Point3 p2 = Point3(DLP_MM2_S(1.0 * *v2), DLP_MM2_S(1.0 * *(v2 + 1)), DLP_MM2_S(1.0 * *(v2 + 2)));
				Point3 p3 = Point3(DLP_MM2_S(1.0 * *v3), DLP_MM2_S(1.0 * *(v3 + 1)), DLP_MM2_S(1.0 * *(v3 + 2)));
				builder.addFace(p1, p2, p3);
			}
		}

		Mesh* mesh = builder.buildMesh();
		if (mesh)
			mesh->calculateBox();
		return mesh;
	}

	MeshObject* loadSTLBinaryMesh(const char* fileName)
	{
		std::vector<Point3> soups;
		loadSTLBinarySoup(fileName, soups);

		return meshFromTriangleSoup(soups);
	}

    void loadSTLBinarySoup(const char* fileName, std::vector<Point3>& soups)
    {
        soups.clear();

        std::vector<FPoint3> fSoups;
        loadSTLBinarySoup(fileName, fSoups);
        convertf2i(fSoups, soups);
    }

    void loadSTLBinarySoup(const char* fileName, std::vector<FPoint3>& soups)
    {
        FILE* file = fopen(fileName, "rb");

		if (!file) return;

        fseek(file, 0L, SEEK_END);
        long long fileSize = ftell(file);
        rewind(file); //Seek back to start.
        size_t faceCount = (fileSize - 80 - sizeof(uint32_t)) / 50;

        char buffer[80];
        //Skip the header
        if (fread(buffer, 80, 1, file) == 1)
        {
            uint32_t reportedFaceCount;
            //Read the face count. We'll use it as a sort of redundancy code to check for file corruption.
            if (fread(&reportedFaceCount, sizeof(uint32_t), 1, file) == 1 && reportedFaceCount == faceCount && faceCount > 0)
            {
                soups.reserve(3 * faceCount);
                for (size_t i = 0; i < faceCount; ++i)
                {
                    if (fread(buffer, 50, 1, file) == 1)
                    {
                        float* v = ((float*)buffer) + 3;

                        soups.emplace_back(FPoint3(v[0], v[1], v[2]));
                        soups.emplace_back(FPoint3(v[3], v[4], v[5]));
                        soups.emplace_back(FPoint3(v[6], v[7], v[8]));
                    }
                    else
                    {
                        soups.clear();
                        break;
                    }
                }
            }
        }

        fclose(file);
    }

    void convertf2i(const std::vector<FPoint3>& fpoints, std::vector<Point3>& points)
    {
        size_t size = fpoints.size();
        if (size > 0)
        {
            points.resize(size);
            for (size_t i = 0; i < size; ++i)
            {
                const FPoint3& p = fpoints.at(i);

                //if (i == 566 * 3)
                //	printf("");

                points.at(i) = Point3(DLP_MM2_S(1.0 * p.x), DLP_MM2_S(1.0 * p.y), DLP_MM2_S(1.0 * p.z));
            }
        }
    }
}