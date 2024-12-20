//Copyright (c) 2018 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#include "Mold.h"
#include "communication/MeshGroup.h"

#include "communication/ExtruderTrain.h"
#include "types/Ratio.h"
#include "utils/IntPoint.h"

#include "slice/sliceddata.h"

namespace cura52
{

    void Mold::process(MeshGroup* meshGroup, std::vector<SlicedData>& datas)
    {
        unsigned int meshCount = datas.size();
        { // check whether we even need to process molds
            bool has_any_mold = false;
            for (unsigned int mesh_idx = 0; mesh_idx < meshCount; mesh_idx++)
            {
                Mesh& mesh = meshGroup->meshes[mesh_idx];
                if (mesh.settings.get<bool>("mold_enabled"))
                {
                    has_any_mold = true;
                    mesh.expandXY(mesh.settings.get<coord_t>("mold_width"));
                }
            }
            if (!has_any_mold)
            {
                return;
            }
        }

        unsigned int layer_count = 0;
        { // compute layer_count
            for (unsigned int mesh_idx = 0; mesh_idx < meshCount; mesh_idx++)
            {
                SlicedData& data = datas[mesh_idx];
                unsigned int layer_count_here = data.layers.size();
                layer_count = std::max(layer_count, layer_count_here);
            }
        }

        const coord_t layer_height = meshGroup->settings.get<coord_t>("layer_height");
        std::vector<Polygons> mold_outline_above_per_mesh; // the outer outlines of the layer above without the original model(s) being cut out
        mold_outline_above_per_mesh.resize(meshCount);
        for (int layer_nr = layer_count - 1; layer_nr >= 0; layer_nr--)
        {
            Polygons all_original_mold_outlines; // outlines of all models for which to generate a mold (insides of all molds)

            // first generate outlines
            for (unsigned int mesh_idx = 0; mesh_idx < meshCount; mesh_idx++)
            {
                const Mesh& mesh = meshGroup->meshes[mesh_idx];
                SlicedData& data = datas[mesh_idx];
                if (!mesh.settings.get<bool>("mold_enabled") || layer_nr >= static_cast<int>(data.layers.size()))
                {
                    continue;
                }
                coord_t width = mesh.settings.get<coord_t>("mold_width");
                coord_t open_polyline_width = mesh.settings.get<coord_t>("wall_line_width_0");
                if (layer_nr == 0)
                {
                    const ExtruderTrain& train_wall_0 = mesh.settings.get<ExtruderTrain&>("wall_0_extruder_nr");
                    open_polyline_width *= train_wall_0.settings.get<Ratio>("initial_layer_line_width_factor");
                }
                const AngleDegrees angle = mesh.settings.get<AngleDegrees>("mold_angle");
                const coord_t roof_height = mesh.settings.get<coord_t>("mold_roof_height");

                const coord_t inset = tan(angle / 180 * M_PI) * layer_height;
                const size_t roof_layer_count = roof_height / layer_height;


                SlicedLayer& layer = data.layers[layer_nr];
                Polygons model_outlines = layer.polygons.unionPolygons(layer.openPolylines.offsetPolyLine(open_polyline_width / 2));
                layer.openPolylines.clear();
                all_original_mold_outlines.add(model_outlines);

                if (angle >= 90)
                {
                    layer.polygons = model_outlines.offset(width, ClipperLib::jtRound);
                }
                else
                {
                    Polygons& mold_outline_above = mold_outline_above_per_mesh[mesh_idx]; // the outside of the mold on the layer above
                    layer.polygons = mold_outline_above.offset(-inset).unionPolygons(model_outlines.offset(width, ClipperLib::jtRound));
                }

                // add roofs
                if (roof_layer_count > 0 && layer_nr > 0)
                {
                    unsigned int layer_nr_below = std::max(0, static_cast<int>(layer_nr - roof_layer_count));
                    Polygons roofs = data.layers[layer_nr_below].polygons.offset(width, ClipperLib::jtRound); // TODO: don't compute offset twice!
                    layer.polygons = layer.polygons.unionPolygons(roofs);
                }

                mold_outline_above_per_mesh[mesh_idx] = layer.polygons;
            }
            all_original_mold_outlines = all_original_mold_outlines.unionPolygons();

            // cut out molds from all objects after generating mold outlines for all objects so that molds won't overlap into the casting cutout of another mold

            // carve molds out of all other models
            for (unsigned int mesh_idx = 0; mesh_idx < meshCount; mesh_idx++)
            {
                const Mesh& mesh = meshGroup->meshes[mesh_idx];
                if (!mesh.settings.get<bool>("mold_enabled"))
                {
                    continue; // only cut original models out of all molds
                }
                SlicedData& data = datas[mesh_idx];
                SlicedLayer& layer = data.layers[layer_nr];
                layer.polygons = layer.polygons.difference(all_original_mold_outlines);
            }
        }
    }
}//namespace cura52
