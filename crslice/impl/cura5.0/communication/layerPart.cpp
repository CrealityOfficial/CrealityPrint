//Copyright (c) 2022 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#include "layerPart.h"
#include "communication/sliceDataStorage.h"
#include "communication/slicecontext.h"

#include "utils/PolylineStitcher.h"
#include "utils/SettingsWrapper.h"

#include "slice/sliceddata.h"
/*
The layer-part creation step is the first step in creating actual useful data for 3D printing.
It takes the result of the Slice step, which is an unordered list of polygons, and makes groups of polygons,
each of these groups is called a "part", which sometimes are also known as "islands". These parts represent
isolated areas in the 2D layer with possible holes.

Creating "parts" is an important step, as all elements in a single part should be printed before going to another part.
And all every bit inside a single part can be printed without the nozzle leaving the boundary of this part.

It's also the first step that stores the result in the "data storage" so all other steps can access it.
*/

namespace cura52 {

    void createLayerWithParts(const Settings& settings, SliceLayer& storageLayer, SlicedLayer* layer, size_t layer_nr)
    {
        PolylineStitcher<Polygons, Polygon, Point>::stitch(layer->openPolylines, storageLayer.openPolyLines, layer->polygons, settings.get<coord_t>("wall_line_width_0"));

        storageLayer.openPolyLines = simplifyPolyline(storageLayer.openPolyLines, settings);

        const bool union_all_remove_holes = settings.get<bool>("meshfix_union_all_remove_holes");
        if (union_all_remove_holes)
        {
            for (unsigned int i = 0; i < layer->polygons.size(); i++)
            {
                if (layer->polygons[i].orientation())
                    layer->polygons[i].reverse();
            }
        }

        std::vector<PolygonsPart> result;
        const bool union_layers = settings.get<bool>("meshfix_union_all");
        const ESurfaceMode surface_only = settings.get<ESurfaceMode>("magic_mesh_surface_mode");
        if (surface_only == ESurfaceMode::SURFACE && !union_layers)
        { // Don't do anything with overlapping areas; no union nor xor
            result.reserve(layer->polygons.size());
            for (const PolygonRef poly : layer->polygons)
            {
                result.emplace_back();
                result.back().add(poly);
            }
        }
        else
        {
            //// result = layer->polygons.splitIntoParts(union_layers || union_all_remove_holes);
            //result.reserve(layer->polygons.size());
            //for (const PolygonRef poly : layer->polygons)
            //{
            //    result.emplace_back();
            //    result.back().add(poly);
            //}
            result = layer->polygons.splitIntoColorParts(union_layers || union_all_remove_holes);
        }
        const coord_t hole_offset = settings.get<coord_t>("hole_xy_offset");
        //const size_t bottom_layers = settings.get<size_t>("bottom_layers");
        //bool magic_spiralize = settings.get<bool>("magic_spiralize");
        // for (auto& part : result)
        for(int i = 0; i < result.size(); i++) 
        {
            PolygonsPart part = result[i];
            storageLayer.parts.emplace_back();
            storageLayer.parts.back().color = layer->polygons.colors[i];
            //if (magic_spiralize && layer_nr >= bottom_layers)
            if (hole_offset != 0)
            {
                // holes remove
                Polygons outline;
            //    for (const PolygonRef poly : part)
            //    {
            //        if (poly.orientation())
            //        {
            //            outline.add(poly);
            //        }
            //    }
            //    storageLayer.parts.back().outline.add(outline.unionPolygons());
            //}
            //else if (hole_offset != 0)
            //{
            //    // holes are to be expanded or shrunk
            //    Polygons outline;
                Polygons holes;
                for (const PolygonRef poly : part)
                {
                    if (poly.orientation())
                    {
                        outline.add(poly);
                    }
                    else
                    {
                        holes.add(poly.offset(hole_offset));
                    }
                }
                storageLayer.parts.back().outline.add(outline.difference(holes.unionPolygons()));
            }
            else
            {
                storageLayer.parts.back().outline = part;
            }
            storageLayer.parts.back().boundaryBox.calculate(storageLayer.parts.back().outline);
            if (storageLayer.parts.back().outline.empty())
            {
                storageLayer.parts.pop_back();
            }
        }
        //    奇数层和偶数层按颜色一个逆序一个正序
        if (layer_nr % 2 != 0)
        {
            for (int i = 0; i < storageLayer.parts.size(); i++)
            {
                for (int j = i; j < storageLayer.parts.size(); j++)
                {
                    if (storageLayer.parts[i].color > storageLayer.parts[j].color)
                    {
                        std::swap(storageLayer.parts[i], storageLayer.parts[j]);
                    }
                }
            }
        }
        else
        {
            for (int i = 0; i < storageLayer.parts.size(); i++)
            {
                for (int j = i; j < storageLayer.parts.size(); j++)
                {
                    if (storageLayer.parts[i].color < storageLayer.parts[j].color)
                    {
                        std::swap(storageLayer.parts[i], storageLayer.parts[j]);
                    }
                }
            }
        }

        for (int i = 0; i < storageLayer.parts.size(); i++)
        {
            if (std::find(storageLayer.colorused.begin(), storageLayer.colorused.end(), storageLayer.parts[i].color) == storageLayer.colorused.end())
            {
                storageLayer.colorused.push_back(storageLayer.parts[i].color);
            }
        }
    }

    void createLayerParts(SliceContext* application, SliceMeshStorage& mesh, SlicedData* data)
    {
        const auto total_layers = data->layers.size();
        assert(mesh.layers.size() == total_layers);

        cura52::parallel_for<size_t>(application, 0, total_layers, [data, &mesh](size_t layer_nr)
            {
                SliceLayer& layer_storage = mesh.layers[layer_nr];
                SlicedLayer& slice_layer = data->layers[layer_nr];
                createLayerWithParts(mesh.settings, layer_storage, &slice_layer, layer_nr);
            });

        for (LayerIndex layer_nr = total_layers - 1; layer_nr >= 0; layer_nr--)
        {
            SliceLayer& layer_storage = mesh.layers[layer_nr];
            if (layer_storage.parts.size() > 0 || (mesh.settings.get<ESurfaceMode>("magic_mesh_surface_mode") != ESurfaceMode::NORMAL && layer_storage.openPolyLines.size() > 0))
            {
                mesh.layer_nr_max_filled_layer = layer_nr; // last set by the highest non-empty layer
                break;
            }
        }
    }

    void handleSupportModifierMesh(SliceDataStorage& storage, const Settings& mesh_settings, const SlicedData* data)
    {
        enum ModifierType
        {
            ANTI_OVERHANG,
            SUPPORT_DROP_DOWN,
            SUPPORT_VANILLA
        };

        ModifierType modifier_type = (mesh_settings.get<bool>("anti_overhang_mesh")) ? ANTI_OVERHANG :
            ((mesh_settings.get<bool>("support_mesh_drop_down")) ? SUPPORT_DROP_DOWN : SUPPORT_VANILLA);
        for (unsigned int layer_nr = 0; layer_nr < data->layers.size(); layer_nr++)
        {
            SupportLayer& support_layer = storage.support.supportLayers[layer_nr];
            const SlicedLayer& slicer_layer = data->layers[layer_nr];
            switch (modifier_type)
            {
            case ANTI_OVERHANG:
                support_layer.anti_overhang.add(slicer_layer.polygons);
                break;
            case SUPPORT_DROP_DOWN:
                support_layer.support_mesh_drop_down.add(slicer_layer.polygons);
                break;
            case SUPPORT_VANILLA:
                support_layer.support_mesh.add(slicer_layer.polygons);
                break;
            }
        }
    }

}//namespace cura52
