#include "serial.h"

#include "trimesh2/Vec.h"
#include <fstream>
namespace cura52
{
	Polygons readPolygons(const std::string& fileName)
	{
        std::vector<std::vector<trimesh::vec2>> ploys;

        std::fstream in(fileName, std::ios::in | std::ios::binary);
        if (in.is_open())
        {
            int pNum = 0;
            in.read((char*)&pNum, sizeof(int));
            if (pNum > 0)
            {
                ploys.resize(pNum);
                for (int i = 0; i < pNum; ++i)
                {
                    int num = 0;
                    in.read((char*)&num, sizeof(int));
                    ploys[i].resize(num);
                    //in.read((char*)&ploys.at(i), sizeof(trimesh::vec2) * num);
                    for (int j = 0; j < num; j++)
                    {
                        in.read((char*)&ploys[i][j].x, sizeof(float));
                        in.read((char*)&ploys[i][j].y, sizeof(float));
                    }
                }
            }
        }
        Polygons result;
        for (const std::vector<trimesh::vec2>& p : ploys)
        {
            if (p.size() > 0)
            {
                for (size_t i = 0; i < p.size() - 1; i++)
                {
                    trimesh::vec2 v0 = p.at(i);
                    trimesh::vec2 v1 = p.at(i + 1);
                    result.addLine(Point(v0.x, v0.y), Point(v1.x, v1.y));
                }
            }
        }
		return result;
	}
}