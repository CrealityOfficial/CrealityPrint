#include "msbase/mesh/chunk.h"
#include "msbase/mesh/get.h"

#include "ccglobal/log.h"

#include <limits.h>
#include <math.h>
#include <set>

namespace msbase
{
    bool generateChunk(
        trimesh::TriMesh* mesh
        , const std::vector<trimesh::ivec3>& neigbs
        , const float max_area_per_chunk
        , std::vector<int>& faceChunkIDs
        , std::vector<std::vector<int>>& chunkFaces)
    {
        if (!mesh || mesh->faces.size() == 0)
        {
            LOGW("msbase::getFaceNormal : error input.");
            return false;
        }

        std::vector<trimesh::vec3> normals;
        std::vector<float> areas;
        calculateFaceNormalOrAreas(mesh,normals,&areas);

        //split chunks
        int faceCount = mesh->faces.size();
        //assert(faceCount == (int)neigbs.size());
        if (faceCount != (int)neigbs.size())
        {
            return false;
        }

        int chunkCount = 50;
        //if (faceCount < chunkCount)
        //    chunkCount = faceCount;

        int max_chunk_count = (int)faceCount / chunkCount;

        int currentChunk = 0;
        chunkFaces.reserve(chunkCount);
        faceChunkIDs.resize(faceCount, -1);

        chunkFaces.push_back(std::vector<int>());
        float area = 0.0f;
        int chunkSize = (int)faceCount / chunkCount / 2;
        for (int i = 0; i < faceCount; ++i)
        {
            if (faceChunkIDs.at(i) >= 0)
                continue;

            std::set<int> seeds;
            seeds.insert(i);
            std::set<int> next_seeds;

            while (!seeds.empty())
            {
                std::vector<int>& chunks = chunkFaces.at(currentChunk);

                for (int s : seeds)
                {
                    const trimesh::ivec3& nei = neigbs.at(s);
                    chunks.push_back(s);

                    area += areas[s];

                    faceChunkIDs.at(s) = currentChunk;

                    for (int j = 0; j < 3; ++j)
                    {
                        if (nei[j] >= 0 && faceChunkIDs.at(nei[j]) < 0)
                            next_seeds.insert(nei[j]);
                    }
                }

                if ((currentChunk < chunkCount - 1) && (/*next_seeds.empty() ||*/ (area > max_area_per_chunk && chunks.size() >= chunkSize)))
                {
                    currentChunk++;
                    chunkFaces.push_back(std::vector<int>());
                    next_seeds.clear();
                    area = 0.0f;
                }

                next_seeds.swap(seeds);
                next_seeds.clear();
            }
        }
        return true;
    }
}