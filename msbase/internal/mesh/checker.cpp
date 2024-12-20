#include "msbase/mesh/checker.h"
#include <assert.h>

#include <queue>

namespace msbase
{
	bool checkDegenerateFace(trimesh::TriMesh* mesh, std::vector<bool>& valid, bool remove)
	{
		if (!mesh || mesh->faces.size() <= 0)
			return false;

		size_t size = mesh->faces.size();
        valid.resize(size, true);

		std::vector<int> newFaces;
		newFaces.reserve(size);
		for (size_t i = 0; i < size; ++i)
		{
			const trimesh::TriMesh::Face& face = mesh->faces.at(i);
            if ((face.x == face.y) || (face.x == face.z) || (face.y == face.z))
            {
                valid.at(i) = false;
                continue;
            }

			newFaces.push_back(i);
		}

		bool have = newFaces.size() != mesh->faces.size();
		if (have && remove)
		{
			bool hasColor = mesh->colors.size() > 0;

			std::vector<trimesh::TriMesh::Face> faces;
			std::vector<trimesh::Color> colors;
			for (int i : newFaces)
			{
				faces.push_back(mesh->faces.at(i));
				if(hasColor)
					colors.push_back(mesh->colors.at(i));
			}
			mesh->faces.swap(faces);

			if(hasColor)
				mesh->colors.swap(colors);
		}
		return have;
	}

    void mantainValids(std::vector<std::string>& datas, std::vector<bool>& valid)
    {
        int size = (int)datas.size();
        if (size == valid.size())
        {
            int valid_size = 0;
            int invalid_size = 0;
            for (int i = 0; i < size; ++i)
            {
                if (valid.at(i))
                {
                    if (invalid_size > 0)
                        datas.at(valid_size) = datas.at(i);
                    ++valid_size;
                }
                else
                    ++invalid_size;
            }

            if (valid_size > 0)
                datas.resize(valid_size);
            else
                datas.clear();
        }
    }

    void checkLargetPlanar(trimesh::TriMesh* mesh, const std::vector<trimesh::vec3>& normals, const std::vector<float>& areas, float threshold,
        /*out*/std::vector<int>& faces)
    {
        if (!mesh)
            return;

        assert(mesh->faces.size() == normals.size());
        assert(mesh->faces.size() == areas.size());
        assert(mesh->across_edge.size() == mesh->faces.size());

        const size_t facenums = mesh->faces.size();
        std::vector<int> sequence(facenums);
        for (int i = 0; i < facenums; ++i) {
            sequence[i] = i;
        }

        std::vector<trimesh::TriMesh::Face>  mffaces = mesh->across_edge;

        std::vector<int> masks(facenums, 1);
        std::vector<std::vector<int>> selectFaces;
        selectFaces.reserve(facenums);
        for (const auto& f : sequence) {
            if (masks[f]) {
                const auto& nf = normals[f];
                std::vector<int>currentFaces;
                std::queue<int>currentQueue;
                currentQueue.emplace(f);
                currentFaces.emplace_back(f);
                masks[f] = false;
                while (!currentQueue.empty()) {
                    auto& fr = currentQueue.front();
                    currentQueue.pop();
                    const trimesh::TriMesh::Face& neighbor = mffaces[fr];
                    for (const auto& fa : neighbor) {
                        if (fa < 0) continue;
                        if (masks[fa]) {
                            const auto& na = normals[fa];
                            const auto& nr = normals[fr];
                            if ((nr DOT na) > threshold && (nf DOT na) > threshold) {
                                currentQueue.emplace(fa);
                                currentFaces.emplace_back(fa);
                                masks[fa] = 0;
                            }
                        }
                    }
                }
                selectFaces.emplace_back(std::move(currentFaces));
            }
        }

        //
        double maxArea = 0.0f;
        std::vector<int> resultFaces;
        for (auto& fs : selectFaces) {
            double currentArea = 0.0;
            for (const auto& f : fs) {
                currentArea += areas[f];
            }
            if (currentArea > maxArea) {
                maxArea = currentArea;
                resultFaces.swap(fs);
            }
        }
        
        faces = resultFaces;
    }
}