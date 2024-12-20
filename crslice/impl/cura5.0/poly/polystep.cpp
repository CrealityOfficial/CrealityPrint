#include "poly/polystep.h"
#include "slice/sliceddata.h"
#include "communication/slicecontext.h"

#include "ConicalOverhang.h"
#include "Mold.h"
#include "multiVolumes.h"

#include "trimesh2/Vec.h"
#include <fstream>

namespace cura52 {
    void processPolygons(SliceContext* application, MeshGroup* meshgroup, std::vector<SlicedData>& slicedDatas)
    {
        std::string ploygonFile = application->polygonFile();

        std::vector<std::vector<trimesh::vec2>> ploys;
        auto readPloygon = [](const std::string& ploygonFile, std::vector<std::vector<trimesh::vec2>>& ploys)
        {
            std::fstream in(ploygonFile, std::ios::in | std::ios::binary);
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
        };

        readPloygon(ploygonFile, ploys);
        if (ploys.empty())
        {
            return;
        }
        Polygons ploygon;
        for (auto& ploy : ploys)
        {
            ClipperLib::Path path;
            for (auto& point : ploy)
            {
                path.push_back(ClipperLib::IntPoint(MM2INT(point.x), MM2INT(point.y)));
            }
            if (!path.empty())
            {
                ploygon.add(path);
            }
        }

        auto getPloygons = [](Polygons& ploygon, const coord_t& gap)
        {
            ClipperLib::ClipperOffset clipper;
            clipper.AddPaths(ploygon.paths, ClipperLib::JoinType::jtSquare, ClipperLib::etOpenSquare);
            clipper.Execute(ploygon.paths, gap);
        };

        int meshCount = (int)meshgroup->meshes.size();
        assert(meshCount == (int)slicedDatas.size());

        for (int i = 0; i < meshCount; ++i)
        {
            const Mesh& mesh = meshgroup->meshes.at(i);
            SlicedData& data = slicedDatas.at(i);
            //double c_gap = 500;
            const coord_t c_gap = mesh.settings.get<coord_t>("mesh_split_gap");
            getPloygons(ploygon, c_gap);

            for (SlicedLayer& layer : data.layers)
            {
                layer.polygons = layer.polygons.difference(ploygon);
            }
        }


    }

	void polyProccess(SliceContext* application, MeshGroup* meshgroup, std::vector<SlicedData>& slicedDatas)
	{
        int meshCount = (int)slicedDatas.size();

        //ÇÐÆ¬ÂÖÀªÔ¤´¦Àí
        processPolygons(application, meshgroup, slicedDatas);
        INTERRUPT_RETURN("polyProccess::processPolygons");

        Mold::process(meshgroup, slicedDatas);

        INTERRUPT_RETURN("polyProccess::Mold");

        for (int mesh_idx = 0; mesh_idx < meshCount; mesh_idx++)
        {
            Mesh& mesh = meshgroup->meshes[mesh_idx];
            if (mesh.settings.get<bool>("conical_overhang_enabled") && !mesh.settings.get<bool>("anti_overhang_mesh"))
            {
                ConicalOverhang::apply(slicedDatas[mesh_idx], mesh);
            }
        }

        INTERRUPT_RETURN("polyProccess::ConicalOverhang");

        MultiVolumes::carveCuttingMeshes(meshgroup, slicedDatas);

        INTERRUPT_RETURN("polyProccess::MultiVolumes");

        if (meshgroup->settings.get<bool>("carve_multiple_volumes"))
        {
            carveMultipleVolumes(meshgroup, slicedDatas);
        }

        INTERRUPT_RETURN("polyProccess::carveMultipleVolumes");

        generateMultipleVolumesOverlap(meshgroup, slicedDatas);

	}
}
