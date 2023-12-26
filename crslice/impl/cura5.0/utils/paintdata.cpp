#include "paintdata.h"
#include "communication/mesh.h"
#include "communication/sliceDataStorage.h"
#include "communication/slicecontext.h"
#include "communication/layerPart.h"
#include "settings/Settings.h"
#include "slice/sliceddata.h"
#include "slice/slicestep.h"
#include <fstream> 

namespace cura52
{
    void getBinaryData(std::string fileName, std::vector<Mesh>& meshs, Settings& setting)
    {
        std::fstream in(fileName, std::ios::in | std::ios::binary);
        if (in.is_open())
        {
            while (1)
            {
                int pNum = 0;
                in.read((char*)&pNum, sizeof(int));
                if (pNum > 0)
                {
                    meshs.push_back(Mesh(setting));
                    meshs.back().faces.resize(pNum);
                    for (int i = 0; i < pNum; ++i)
                    {
                        int num = 0;
                        in.read((char*)&num, sizeof(int));
                        if (num == 3)
                        {
                            for (int j = 0; j < num; j++)
                            {
                                in.read((char*)&meshs.back().faces[i].vertex_index[j], sizeof(int));
                            }
                        }
                    }
                }
                else
                    break;

                pNum = 0;
                in.read((char*)&pNum, sizeof(int));
                if (pNum > 0)
                {
                    for (int i = 0; i < pNum; ++i)
                    {
                        int num = 0;
                        in.read((char*)&num, sizeof(int));
                        if (num == 3)
                        {
                            std::vector<float> v(3);
                            for (int j = 0; j < num; j++)
                            {
                                in.read((char*)&v[j], sizeof(float));
                            }
                            meshs.back().vertices.push_back(MeshVertex(Point3(MM2INT(v[0]), MM2INT(v[1]), MM2INT(v[02]))));
                        }
                    }
                }
                meshs.back().finish();
            }
        }
        in.close();
    }

    void getPaintSupport(SliceDataStorage& storage, SliceContext* application, const int layer_thickness, const int slice_layer_count, const bool use_variable_layer_heights)
    {
        std::vector<cura52::Mesh> supportMesh;
        getBinaryData(application->supportFile(), supportMesh, application->currentGroup()->settings);
        if (!supportMesh.empty())
        {
            for (Mesh& mesh : supportMesh)
            {
                SlicedData slicedData;
                mesh.settings.add("support_paint_enable", "true");
                mesh.settings.add("keep_open_polygons", "true");
                mesh.settings.add("minimum_polygon_circumference", "0.05");//长度小于此值会被移除
                sliceMesh(application, &mesh, layer_thickness, slice_layer_count, use_variable_layer_heights, nullptr, slicedData);
                handleSupportModifierMesh(storage, mesh.settings, &slicedData);
            }
        }
    }

    void getPaintSupport_anti(SliceDataStorage& storage, SliceContext* application, const int layer_thickness, const int slice_layer_count, const bool use_variable_layer_heights)
    {
        std::vector<cura52::Mesh> antiMesh;
        getBinaryData(application->antiSupportFile(), antiMesh, application->currentGroup()->settings);
        if (!antiMesh.empty())
        {
            for (Mesh& mesh : antiMesh)
            {
                SlicedData slicedData;
                mesh.settings.add("support_paint_enable", "true");
                mesh.settings.add("keep_open_polygons", "true");
                mesh.settings.add("minimum_polygon_circumference", "0.05");//长度小于此值会被移除
                mesh.settings.add("anti_overhang_mesh", "true");
                sliceMesh(application, &mesh, layer_thickness, slice_layer_count, use_variable_layer_heights, nullptr, slicedData);
                handleSupportModifierMesh(storage, mesh.settings, &slicedData);
            }
        }
    }

    void getZseamLine(SliceDataStorage& storage, SliceContext* application, const int layer_thickness, const int slice_layer_count, const bool use_variable_layer_heights)
    {
        std::vector<cura52::Mesh> ZseamMesh;
        getBinaryData(application->seamFile(), ZseamMesh, application->currentGroup()->settings);
        if (!ZseamMesh.empty())
        {
            storage.zSeamPoints.resize(slice_layer_count);
            for (Mesh& mesh : ZseamMesh)
            {
                SlicedData slicedData;
                std::vector<SlicerLayer> Zseamlineslayers;

                mesh.settings.add("zseam_paint_enable", "true");
                sliceMesh(application, &mesh, layer_thickness, slice_layer_count, use_variable_layer_heights, nullptr, Zseamlineslayers);
                for (unsigned int layer_nr = 0; layer_nr < Zseamlineslayers.size(); layer_nr++)
                {
                    for (size_t i = 0; i < Zseamlineslayers[layer_nr].segments.size(); i++)
                    {
                        storage.zSeamPoints[layer_nr].ZseamLayers.push_back(ZseamDrawPoint(Zseamlineslayers[layer_nr].segments[i].start));
                    }
                }
            }
        }
    }
    void getZseamLine_anti(SliceDataStorage& storage, SliceContext* application, const int layer_thickness, const int slice_layer_count, const bool use_variable_layer_heights)
    {
        std::vector<cura52::Mesh> antiMesh;
        getBinaryData(application->antiSeamFile(), antiMesh, application->currentGroup()->settings);
        if (!antiMesh.empty())
        {
            storage.interceptSeamPoints.resize(slice_layer_count);
            for (Mesh& mesh : antiMesh)
            {
                SlicedData slicedData;
                std::vector<SlicerLayer> Zseamlineslayers;

                mesh.settings.add("zseam_paint_enable", "true");
                sliceMesh(application, &mesh, layer_thickness, slice_layer_count, use_variable_layer_heights, nullptr, Zseamlineslayers);
                for (unsigned int layer_nr = 0; layer_nr < Zseamlineslayers.size(); layer_nr++)
                {
                    for (size_t i = 0; i < Zseamlineslayers[layer_nr].segments.size(); i++)
                    {
                        storage.interceptSeamPoints[layer_nr].ZseamLayers.push_back(ZseamDrawPoint(Zseamlineslayers[layer_nr].segments[i].start));
                    }
                }
            }
        }
    }

    void mergePaintSupport(SliceDataStorage& storage)
    {
        int layer = storage.support.supportLayers.size();
        for (auto& mesh : storage.meshes)
        {
            if (layer >= mesh.overhang_areas.size())
            {
                for (int i = 0; i < mesh.overhang_areas.size(); i++)
                {
                    if (!storage.support.supportLayers[i].support_mesh_drop_down.empty())
                    {
                        Polygons polys = mesh.layers[i % mesh.layers.size()].getOutlines().intersection(storage.support.supportLayers[i].support_mesh_drop_down);
                        mesh.overhang_areas[i] = mesh.overhang_areas[i].unionPolygons(polys);
                        mesh.full_overhang_areas[i] = mesh.overhang_areas[i].unionPolygons(polys);
                    }

                    if (!mesh.overhang_areas[i].empty() && !storage.support.supportLayers[i].anti_overhang.empty())
                    {
                        Polygons polys = mesh.layers[i % mesh.layers.size()].getOutlines().intersection(storage.support.supportLayers[i].anti_overhang);
                        mesh.overhang_areas[i] = mesh.overhang_areas[i].unionPolygons(polys);
                        mesh.overhang_areas[i] = mesh.overhang_areas[i].difference(polys);
                        //
                    }
                }
            }
        }
    }

    void mergeOpenPloygons(const Mesh* mesh, std::vector<SlicerLayer>& layers)
    {
        coord_t c_gap = 200;
        if (mesh->settings.get<bool>("support_paint_enable"))
            c_gap = mesh->settings.get<coord_t>("support_line_width")*2;

        auto getPloygons = [&c_gap](Polygons& ploygon)
        {
            ClipperLib::ClipperOffset clipper;
            clipper.AddPaths(ploygon.paths, ClipperLib::JoinType::jtSquare, ClipperLib::etOpenSquare);
            clipper.Execute(ploygon.paths, c_gap);
        };
        for (SlicerLayer& alayer : layers)
        {
            if (!alayer.openPolylines.empty())
            {
                getPloygons(alayer.openPolylines);
                alayer.polygons.add(alayer.openPolylines);
                alayer.openPolylines.clear();
            }
        }
    }
}


