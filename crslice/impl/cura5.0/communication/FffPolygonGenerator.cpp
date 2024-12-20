// Copyright (c) 2022 Ultimaker B.V.
// CuraEngine is released under the terms of the AGPLv3 or higher

#include <algorithm>
#include <atomic>
#include <fstream> // ifstream.good()
#include <map> // multimap (ordered map allowing duplicate keys)
#include <numeric>

// Code smell: Order of the includes is important here, probably due to some forward declarations which might be masking some undefined behaviours
// clang-format off
#include "FffPolygonGenerator.h"
#include "layerPart.h"

#include "support/support.h"
#include "support/TreeSupport.h"
#include "support/TreeSupportT.h"
#include "support/ThomasTreeSupport.h"


#include "infill/infill.h"

#include "magic/raft.h"
#include "magic/skin.h"
#include "magic/SkirtBrim.h"
#include "magic/TopSurface.h"

#include "sliceDataStorage.h"
#include "MeshGroup.h"

#include "skeletal/WallsComputation.h"

#include "infill/DensityProvider.h"
#include "infill/ImageBasedDensityProvider.h"
#include "infill/LightningGenerator.h"
#include "infill/SierpinskiFillProvider.h"
#include "infill/SubDivCube.h"
#include "infill/UniformDensityProvider.h"
#include "progress/ProgressEstimator.h"
#include "progress/ProgressEstimatorLinear.h"
#include "progress/ProgressStageEstimator.h"
#include "magic/AdaptiveLayerHeights.h"
#include "utils/algorithm.h"
#include "utils/math.h"
#include "utils/Simplify.h"
#include "utils/paintdata.h"

#include "slice/slicestep.h"
#include "poly/polystep.h"
#include "poly/getlayerdata.h"
#include "slice/slicer.h"

#include "slice/sliceddata.h"
#include "communication/slicecontext.h"
// clang-format on

namespace cura52
{


bool FffPolygonGenerator::generateAreas(SliceDataStorage& storage, MeshGroup* meshgroup)
{
	CALLTICK("sliceModel 0");
    if (! sliceModel(meshgroup, storage))
    {
        return false;
    }
	CALLTICK("sliceModel 1");

    INTERRUPT_RETURN_FALSE("FffPolygonGenerator::generateAreas");

	CALLTICK("slices2polygons 0");
    slices2polygons(storage);
	CALLTICK("slices2polygons 1");
    return true;
}

size_t FffPolygonGenerator::getDraftShieldLayerCount(const size_t total_layers) const
{
    const Settings& mesh_group_settings = application->currentGroup()->settings;
    if (! mesh_group_settings.get<bool>("draft_shield_enabled"))
    {
        return 0;
    }
    switch (mesh_group_settings.get<DraftShieldHeightLimitation>("draft_shield_height_limitation"))
    {
    case DraftShieldHeightLimitation::FULL:
        return total_layers;
    case DraftShieldHeightLimitation::LIMITED:
        return std::max((coord_t)0, (mesh_group_settings.get<coord_t>("draft_shield_height") - application->get_layer_height_0()) / application->get_layer_height() + 1);
    default:
        LOGW("A draft shield height limitation option was added without implementing the new option in getDraftShieldLayerCount.");
        return total_layers;
    }
}

bool FffPolygonGenerator::sliceModel(MeshGroup* meshgroup, SliceDataStorage& storage) /// slices the model
{
    application->messageProgressStage(Progress::Stage::SLICING);

    storage.model_min = meshgroup->min();
    storage.model_max = meshgroup->max();
    storage.model_size = storage.model_max - storage.model_min;

    LOGI("Slicing model...");

    const Settings& mesh_group_settings = meshgroup->settings;

    // regular layers
    int slice_layer_count = 0; // Use signed int because we need to subtract the initial layer in a calculation temporarily.

    // Initial layer height of 0 is not allowed. Negative layer height is nonsense.
    coord_t initial_layer_thickness = application->get_layer_height_0();
    if (initial_layer_thickness <= 0)
    {
        LOGE("Initial layer height { %f } is disallowed.", initial_layer_thickness);
        return false;
    }

    // Layer height of 0 is not allowed. Negative layer height is nonsense.
    const coord_t layer_thickness = application->get_layer_height();
    if (layer_thickness <= 0)
    {
        LOGE("Layer height { %f } is disallowed.\n", layer_thickness);
        return false;
    }

    // variable layers
    AdaptiveLayerHeights* adaptive_layer_heights = nullptr;
    const bool use_variable_layer_heights = mesh_group_settings.get<bool>("adaptive_layer_height_enabled");

    if (use_variable_layer_heights)
    {
        // Calculate adaptive layer heights
        const coord_t variable_layer_height_max_variation = mesh_group_settings.get<coord_t>("adaptive_layer_height_variation");
        const coord_t variable_layer_height_variation_step = mesh_group_settings.get<coord_t>("adaptive_layer_height_variation_step");
        const coord_t adaptive_threshold = mesh_group_settings.get<coord_t>("adaptive_layer_height_threshold");
        adaptive_layer_heights = new AdaptiveLayerHeights(meshgroup, 
            layer_thickness, variable_layer_height_max_variation, variable_layer_height_variation_step, adaptive_threshold);

        // Get the amount of layers
        slice_layer_count = adaptive_layer_heights->getLayerCount();
    }
    else
    {
        // Find highest layer count according to each mesh's settings.
        for (const Mesh& mesh : meshgroup->meshes)
        {
            if (! mesh.isPrinted())
            {
                continue;
            }
            const coord_t mesh_height = mesh.max().z;
            switch (mesh.settings.get<SlicingTolerance>("slicing_tolerance"))
            {
            case SlicingTolerance::MIDDLE:
                if (storage.model_max.z < initial_layer_thickness)
                {
                    slice_layer_count = std::max(slice_layer_count, (mesh_height > initial_layer_thickness / 2) ? 1 : 0); // One layer if higher than half initial layer height.
                }
                else
                {
                    slice_layer_count = std::max(slice_layer_count, static_cast<int>(round_divide_signed(mesh_height - initial_layer_thickness, layer_thickness) + 1));
                }
                break;
            case SlicingTolerance::EXCLUSIVE:
            {
                int new_slice_layer_count = 0;
                if (mesh_height >= initial_layer_thickness) // If less than the initial layer thickness, leave it at 0.
                {
                    new_slice_layer_count = static_cast<int>(floor_divide_signed(mesh_height - 1 - initial_layer_thickness, layer_thickness) + 1);
                }
                if (new_slice_layer_count > 0) // If there is at least one layer already, then...
                {
                    new_slice_layer_count += 1; // ... need one extra, since we clear the top layer after the repeated intersections with the layer above.
                }
                slice_layer_count = std::max(slice_layer_count, new_slice_layer_count);
                break;
            }
            case SlicingTolerance::INCLUSIVE:
                if (mesh_height < initial_layer_thickness)
                {
                    slice_layer_count = std::max(slice_layer_count, (mesh_height > 0) ? 1 : 0); // If less than the initial layer height, it always has 1 layer unless the height is truly zero.
                }
                else
                {
                    slice_layer_count = std::max(slice_layer_count, static_cast<int>(ceil_divide_signed(mesh_height - initial_layer_thickness, layer_thickness) + 1));
                }
                break;
            default:
                LOGE("Unknown slicing tolerance. Did you forget to add a case here?");
                return false;
            }
        }
    }

    // Model is shallower than layer_height_0, so not even the first layer is sliced. Return an empty model then.
    if (slice_layer_count <= 0)
    {
        return true; // This is NOT an error state!
    }

    int meshCount = (int)meshgroup->meshes.size();
    std::vector<SlicedData> slicedDatas(meshCount);

    SAFE_MESSAGE(1, 0);

    for (int mesh_idx = 0; mesh_idx < meshCount; mesh_idx++)
    {
        INTERRUPT_RETURN_FALSE("FffPolygonGenerator::sliceModel");

        // Check if adaptive layers is populated to prevent accessing a method on NULL
        std::vector<AdaptiveLayer>* adaptive_layer_height_values = {};
        if (adaptive_layer_heights != nullptr)
        {
            adaptive_layer_height_values = adaptive_layer_heights->getLayers();
        }

        Mesh& mesh = meshgroup->meshes[mesh_idx];

        sliceMesh(application, &mesh, layer_thickness, slice_layer_count, use_variable_layer_heights, adaptive_layer_height_values, slicedDatas.at(mesh_idx));
        application->messageProgress(Progress::Stage::SLICING, mesh_idx + 1, meshgroup->meshes.size());
    }

    // Clear the mesh face and vertex data, it is no longer needed after this point, and it saves a lot of memory.
    meshgroup->clear();

#if USE_CACHE
    cacheSlicedData(slicedDatas);
#endif

    SAFE_MESSAGE(2, 0);

    INTERRUPT_RETURN_FALSE("FffPolygonGenerator::sliceModel");
    polyProccess(application, meshgroup, slicedDatas);

    INTERRUPT_RETURN_FALSE("FffPolygonGenerator::sliceModel");
    application->messageProgressStage(Progress::Stage::PARTS);

    storage.print_layer_count = 0;
    for (int meshIdx = 0; meshIdx < meshCount; meshIdx++)
    {
        Mesh& mesh = meshgroup->meshes[meshIdx];
        const SlicedData& slicedData = slicedDatas.at(meshIdx);
        if (! mesh.settings.get<bool>("anti_overhang_mesh") && ! mesh.settings.get<bool>("infill_mesh") && ! mesh.settings.get<bool>("cutting_mesh"))
        {
            storage.print_layer_count = std::max(storage.print_layer_count, slicedData.layers.size());
        }
    }

    INTERRUPT_RETURN_FALSE("FffPolygonGenerator::sliceModel");

    storage.support.supportLayers.resize(storage.print_layer_count);

    //get paint_support
    if (meshgroup->settings.get<bool>("support_enable"))
    {
        getPaintSupport(storage, application, layer_thickness, slice_layer_count, use_variable_layer_heights);
        getPaintSupport_anti(storage, application, layer_thickness, slice_layer_count, use_variable_layer_heights);
    }
        
    //get Zseam line;
    {
        getZseamLine(storage, application, layer_thickness, slice_layer_count, use_variable_layer_heights);
        getZseamLine_anti(storage, application, layer_thickness, slice_layer_count, use_variable_layer_heights);
    }

    storage.meshes.reserve(meshCount); // causes there to be no resize in meshes so that the pointers in sliceMeshStorage._config to retraction_config don't get invalidated.
    for (int meshIdx = 0; meshIdx < meshCount; meshIdx++)
    {
        INTERRUPT_RETURN_FALSE("FffPolygonGenerator::sliceModel");

        SlicedData& slicedData = slicedDatas.at(meshIdx);
        Mesh& mesh = meshgroup->meshes[meshIdx];

        // always make a new SliceMeshStorage, so that they have the same ordering / indexing as meshgroup.meshes
        storage.meshes.emplace_back(&meshgroup->meshes[meshIdx], slicedData.layers.size()); // new mesh in storage had settings from the Mesh
        SliceMeshStorage& meshStorage = storage.meshes.back();
        meshStorage.appliction = storage.application;

        // only create layer parts for normal meshes
        const bool is_support_modifier = mesh.settings.get<bool>("anti_overhang_mesh") || mesh.settings.get<bool>("support_mesh");

        if (! is_support_modifier)
        {
            createLayerParts(application, meshStorage, &slicedData);
        }
        else
        {
            handleSupportModifierMesh(storage, mesh.settings, &slicedData);
        }

        // Do not add and process support _modifier_ meshes further, and ONLY skip support _modifiers_. They have been
        // processed in AreaSupport::handleSupportModifierMesh(), but other helper meshes such as infill meshes are
        // processed in a later stage, except for support mesh itself, so an exception is made for that.
        if (is_support_modifier && ! mesh.settings.get<bool>("support_mesh"))
        {
            storage.meshes.pop_back();
            continue;
        }

        // check one if raft offset is needed
        EPlatformAdhesion adhesion_type = application->get_adhesion_type();
        const bool has_raft = adhesion_type == EPlatformAdhesion::RAFT;
        const bool has_simple_raft = adhesion_type == EPlatformAdhesion::SIMPLERAFT;

        // calculate the height at which each layer is actually printed (printZ)
        for (unsigned int layer_nr = 0; layer_nr < meshStorage.layers.size(); layer_nr++)
        {
            SliceLayer& layer = meshStorage.layers[layer_nr];

            if (use_variable_layer_heights)
            {
                meshStorage.layers[layer_nr].printZ = adaptive_layer_heights->getLayers()->at(layer_nr).z_position;
                meshStorage.layers[layer_nr].thickness = adaptive_layer_heights->getLayers()->at(layer_nr).layer_height;
            }
            else
            {
                meshStorage.layers[layer_nr].printZ = initial_layer_thickness + (layer_nr * layer_thickness);

                if (layer_nr == 0)
                {
                    meshStorage.layers[layer_nr].thickness = initial_layer_thickness;
                }
                else
                {
                    meshStorage.layers[layer_nr].thickness = layer_thickness;
                }
            }

            // add the raft offset to each layer
            if (has_raft)
            {
                const ExtruderTrain& train = mesh_group_settings.get<ExtruderTrain&>("raft_surface_extruder_nr");
                layer.printZ += Raft::getTotalThickness(application) + train.settings.get<coord_t>("raft_airgap") - train.settings.get<coord_t>("layer_0_z_overlap"); // shift all layers (except 0) down

                if (layer_nr == 0)
                {
                    layer.printZ += train.settings.get<coord_t>("layer_0_z_overlap"); // undo shifting down of first layer
                }
            }

            if (has_simple_raft)
            {
                const ExtruderTrain& extruder = mesh_group_settings.get<ExtruderTrain&>("raft_surface_extruder_nr");
                auto t = Raft::getTotalSimpleRaftThickness(application);
                coord_t air = extruder.settings.get<coord_t>("raft_airgap");
                (void)air;

                coord_t ov = extruder.settings.get<coord_t>("layer_0_z_overlap");
                (void)ov;

                layer.printZ += t
                    + extruder.settings.get<coord_t>("raft_airgap")
                    - extruder.settings.get<coord_t>("layer_0_z_overlap"); // shift all layers (except 0) down

                if (layer_nr == 0)
                {
                    layer.printZ += extruder.settings.get<coord_t>("layer_0_z_overlap"); // undo shifting down of first layer
                }
            }
        }

        application->messageProgress(Progress::Stage::PARTS, meshIdx + 1, meshCount);
    }
    return true;
}

void FffPolygonGenerator::slices2polygons(SliceDataStorage& storage)
{
    // compute layer count and remove first empty layers
    // there is no separate progress stage for removeEmptyFisrtLayer (TODO)
    unsigned int slice_layer_count = 0;
    for (SliceMeshStorage& mesh : storage.meshes)
    {
        if (! mesh.settings.get<bool>("infill_mesh") && ! mesh.settings.get<bool>("anti_overhang_mesh"))
        {
            slice_layer_count = std::max<unsigned int>(slice_layer_count, mesh.layers.size());
        }
    }

    // handle meshes
    std::vector<double> mesh_timings;
    for (unsigned int mesh_idx = 0; mesh_idx < storage.meshes.size(); mesh_idx++)
    {
        mesh_timings.push_back(1.0); // TODO: have a more accurate estimate of the relative time it takes per mesh, based on the height and number of polygons
    }
    ProgressStageEstimator inset_skin_progress_estimate(mesh_timings);

    application->messageProgressStage(Progress::Stage::INSET_SKIN);
    std::vector<size_t> mesh_order;
    { // compute mesh order
        std::multimap<int, size_t> order_to_mesh_indices;
        for (size_t mesh_idx = 0; mesh_idx < storage.meshes.size(); mesh_idx++)
        {
            order_to_mesh_indices.emplace(storage.meshes[mesh_idx].settings.get<int>("infill_mesh_order"), mesh_idx);
        }
        for (std::pair<const int, size_t>& order_and_mesh_idx : order_to_mesh_indices)
        {
            mesh_order.push_back(order_and_mesh_idx.second);
        }
    }

    for (size_t mesh_order_idx = 0; mesh_order_idx < mesh_order.size(); ++mesh_order_idx)
    {
        processBasicWallsSkinInfill(storage, mesh_order_idx, mesh_order, inset_skin_progress_estimate);
        application->messageProgress(Progress::Stage::INSET_SKIN, mesh_order_idx + 1, storage.meshes.size());
    }

    const Settings& mesh_group_settings = application->currentGroup()->settings;
    if (isEmptyLayer(storage, 0) && ! isEmptyLayer(storage, 1))
    {
        // the first layer is empty, the second is not empty, so remove the empty first layer as support isn't going to be generated under it.
        // Do this irrespective of the value of remove_empty_first_layers as that setting is hidden when support is enabled and so cannot be relied upon

        removeEmptyFirstLayers(storage, storage.print_layer_count); // changes storage.print_layer_count!
    }

    LOGI("Layer count: { %d }", storage.print_layer_count);

    // layerparts2HTML(storage, "output/output.html");
	CALLTICK("support 0");
    application->messageProgressStage(Progress::Stage::SUPPORT);

	if (mesh_group_settings.get<bool>("support_enable"))
	{
        SAFE_MESSAGE(6, 0);
	}
    if (!mesh_group_settings.get<bool>("support_enable")) 
	{
		SAFE_MESSAGE(5, 0);
		if (AreaSupport::isSupportNecessary(storage))
		{
			SAFE_MESSAGE(11, 0);
		}
    }

    AreaSupport::generateOverhangAreas(storage);

    INTERRUPT_RETURN("FffPolygonGenerator::slices2polygons");

    AreaSupport::generateSupportAreas(storage);

    INTERRUPT_RETURN("FffPolygonGenerator::slices2polygons");
    mergePaintSupport(storage);
    if (application->sceneSettings().get<ESupportStructure>("support_structure") == ESupportStructure::THOMASTREE
        || application->sceneSettings().get<ESupportStructure>("support_structure") == ESupportStructure::THOMASTREE_MANUAL)
    {
        //ThomasTreeSupport thomas_tree_support_generator(storage);
        //thomas_tree_support_generator.generateSupportAreas(storage);
        cura54::TreeSupportT tree_support_generator(storage);
        tree_support_generator.generateSupportAreas(storage);
    }
    //else if(application->sceneSettings().get<ESupportStructure>("support_structure") == ESupportStructure::TREE)
    //{
    //    TreeSupport tree_support_generator(storage);
    //    tree_support_generator.generateSupportAreas(storage);
    //}

    AreaSupport::generateSharpTailSupport(storage);

	INTERRUPT_RETURN("FffPolygonGenerator::slices2polygons");
	CALLTICK("support 1");
    // we need to remove empty layers after we have processed the insets
    // processInsets might throw away parts if they have no wall at all (cause it doesn't fit)
    // brim depends on the first layer not being empty
    // only remove empty layers if we haven't generate support, because then support was added underneath the model.
    //   for some materials it's better to print on support than on the build plate.
    if (mesh_group_settings.get<bool>("remove_empty_first_layers"))
    {
        removeEmptyFirstLayers(storage, storage.print_layer_count); // changes storage.print_layer_count!
    }
    if (storage.print_layer_count == 0)
    {
        LOGW("Stopping process because there are no non-empty layers.");
        return;
    }

    computePrintHeightStatistics(storage);

    // handle helpers
    storage.primeTower.generateGroundpoly();
    storage.primeTower.generatePaths(storage);
    storage.primeTower.subtractFromSupport(storage);

    LOGD("Processing ooze shield");
    processOozeShield(storage);

    LOGD("Processing draft shield");
    processDraftShield(storage);

    INTERRUPT_RETURN("FffPolygonGenerator::slices2polygons");

    // This catches a special case in which the models are in the air, and then
    // the adhesion mustn't be calculated.
    if (! isEmptyLayer(storage, 0) || storage.primeTower.enabled)
    {
        LOGD("Processing platform adhesion");
        processPlatformAdhesion(storage);
    }

    LOGD("Meshes post-processing");
    // meshes post processing
    for (SliceMeshStorage& mesh : storage.meshes)
    {
        INTERRUPT_RETURN("FffPolygonGenerator::slices2polygons");

        processDerivedWallsSkinInfill(mesh);
    }

    LOGD("Processing gradual support");
    // generate gradual support
    AreaSupport::generateSupportInfillFeatures(storage);


    if(application->debugger())
        getLayerSupportData(storage, application);

#if USE_CACHE
        cacheAll(storage);
#endif 
}

void FffPolygonGenerator::processBasicWallsSkinInfill(SliceDataStorage& storage, const size_t mesh_order_idx, const std::vector<size_t>& mesh_order, ProgressStageEstimator& inset_skin_progress_estimate)
{
    size_t mesh_idx = mesh_order[mesh_order_idx];
    SliceMeshStorage& mesh = storage.meshes[mesh_idx];
    size_t mesh_layer_count = mesh.layers.size();

    SAFE_MESSAGE(3, 0);

    if (mesh.settings.get<bool>("infill_mesh"))
    {
        processInfillMesh(storage, mesh_order_idx, mesh_order);
    }

    INTERRUPT_RETURN("FffPolygonGenerator::processBasicWallsSkinInfill");

    // TODO: make progress more accurate!!
    // note: estimated time for     insets : skins = 22.953 : 48.858
    std::vector<double> walls_vs_skin_timing({ 22.953, 48.858 });
    ProgressStageEstimator* mesh_inset_skin_progress_estimator = new ProgressStageEstimator(walls_vs_skin_timing);

    inset_skin_progress_estimate.nextStage(mesh_inset_skin_progress_estimator); // the stage of this function call

    ProgressEstimatorLinear* inset_estimator = new ProgressEstimatorLinear(mesh_layer_count);
    mesh_inset_skin_progress_estimator->nextStage(inset_estimator);

    struct
    {
        ProgressStageEstimator& progress_estimator;
        SliceContext* application;

        std::mutex mutex{};
        std::atomic<size_t> processed_layer_count = 0;

        void operator++(int)
        {
            std::unique_lock<std::mutex> lock(mutex, std::try_to_lock);
            if (lock)
            { // progress estimation is done only in one thread so that no two threads message progress at the same time
                size_t processed_layer_count_ = processed_layer_count.fetch_add(1, std::memory_order_relaxed);
                double progress = progress_estimator.progress(processed_layer_count_);
                application->messageProgress(Progress::Stage::INSET_SKIN, progress * 100, 100);
            }
            else
            {
                processed_layer_count.fetch_add(1, std::memory_order_release);
            }
        }
        void reset()
        {
            processed_layer_count.store(0, std::memory_order_relaxed);
        }
    } guarded_progress = { inset_skin_progress_estimate, application };


	// walls
	CALLTICK("processWalls 0");
	cura52::parallel_for<size_t>(application, 0,
		mesh_layer_count,
		[&](size_t layer_number)
		{
			INTERRUPT_RETURN("FffPolygonGenerator::processBasicWallsSkinInfill");
			processWalls(mesh, layer_number);
			guarded_progress++;
		});
	CALLTICK("processWalls 1");

    ProgressEstimatorLinear* skin_estimator = new ProgressEstimatorLinear(mesh_layer_count);
    mesh_inset_skin_progress_estimator->nextStage(skin_estimator);

    bool process_infill = mesh.settings.get<coord_t>("infill_line_distance") > 0;
    if (! process_infill)
    { // do process infill anyway if it's modified by modifier meshes
        for (size_t other_mesh_order_idx = mesh_order_idx + 1; other_mesh_order_idx < mesh_order.size(); ++other_mesh_order_idx)
        {
            const size_t other_mesh_idx = mesh_order[other_mesh_order_idx];
            SliceMeshStorage& other_mesh = storage.meshes[other_mesh_idx];
            if (other_mesh.settings.get<bool>("infill_mesh"))
            {
                AABB3D aabb = application->currentGroup()->meshes[mesh_idx].getAABB();
                AABB3D other_aabb = application->currentGroup()->meshes[other_mesh_idx].getAABB();
                if (aabb.hit(other_aabb))
                {
                    process_infill = true;
                }
            }
        }
    }

    SAFE_MESSAGE(4, 0);

    // skin & infill
	CALLTICK("skin & infill 0");
    const Settings& mesh_group_settings = application->currentGroup()->settings;
    bool magic_spiralize = mesh_group_settings.get<bool>("magic_spiralize");
    size_t mesh_max_initial_bottom_layer_count = 0;
    if (magic_spiralize)
    {
        mesh_max_initial_bottom_layer_count = std::max(mesh_max_initial_bottom_layer_count, mesh.settings.get<size_t>("initial_bottom_layers"));
    }

    guarded_progress.reset();
#ifdef _DEBUG
	for (size_t layer_number=0;layer_number< mesh_layer_count;layer_number++)
	{
		INTERRUPT_RETURN("FffPolygonGenerator::processBasicWallsSkinInfill");

		if (!magic_spiralize || layer_number < mesh_max_initial_bottom_layer_count) // Only generate up/downskin and infill for the first X layers when spiralize is choosen.
		{
			processSkinsAndInfill(mesh, layer_number, process_infill);
		}
		guarded_progress++;
	}
#else 
	cura52::parallel_for<size_t>(application, 0,
		mesh_layer_count,
		[&](size_t layer_number)
		{
			INTERRUPT_RETURN("FffPolygonGenerator::processBasicWallsSkinInfill");

			if (!magic_spiralize || layer_number < mesh_max_initial_bottom_layer_count) // Only generate up/downskin and infill for the first X layers when spiralize is choosen.
			{
				processSkinsAndInfill(mesh, layer_number, process_infill);
			}
			guarded_progress++;
		});
#endif


	CALLTICK("skin & infill 1");
}

void FffPolygonGenerator::processInfillMesh(SliceDataStorage& storage, const size_t mesh_order_idx, const std::vector<size_t>& mesh_order)
{
    size_t mesh_idx = mesh_order[mesh_order_idx];
    SliceMeshStorage& mesh = storage.meshes[mesh_idx];
    coord_t surface_line_width = mesh.settings.get<coord_t>("wall_line_width_0");

    mesh.layer_nr_max_filled_layer = -1;
    for (LayerIndex layer_idx = 0; layer_idx < static_cast<LayerIndex>(mesh.layers.size()); layer_idx++)
    {
        SliceLayer& layer = mesh.layers[layer_idx];

        if (mesh.settings.get<ESurfaceMode>("magic_mesh_surface_mode") == ESurfaceMode::SURFACE)
        {
            // break up polygons into polylines
            // they have to be polylines, because they might break up further when doing the cutting
            for (SliceLayerPart& part : layer.parts)
            {
                for (PolygonRef poly : part.outline)
                {
                    layer.openPolyLines.add(poly);
                    layer.openPolyLines.back().add(layer.openPolyLines.back()[0]); // add the segment which closes the polygon
                }
            }
            layer.parts.clear();
        }

        std::vector<PolygonsPart> new_parts;
        Polygons new_polylines;

        for (const size_t other_mesh_idx : mesh_order)
        { // limit the infill mesh's outline to within the infill of all meshes with lower order
            if (other_mesh_idx == mesh_idx)
            {
                break; // all previous meshes have been processed
            }
            SliceMeshStorage& other_mesh = storage.meshes[other_mesh_idx];
            if (layer_idx >= static_cast<LayerIndex>(other_mesh.layers.size()))
            { // there can be no interaction between the infill mesh and this other non-infill mesh
                continue;
            }

            SliceLayer& other_layer = other_mesh.layers[layer_idx];

            for (SliceLayerPart& other_part : other_layer.parts)
            {
                if (mesh.settings.get<ESurfaceMode>("magic_mesh_surface_mode") != ESurfaceMode::SURFACE)
                {
                    for (SliceLayerPart& part : layer.parts)
                    { // limit the outline of each part of this infill mesh to the infill of parts of the other mesh with lower infill mesh order
                        if (! part.boundaryBox.hit(other_part.boundaryBox))
                        { // early out
                            continue;
                        }
                        Polygons new_outline = part.outline.intersection(other_part.getOwnInfillArea());
                        if (new_outline.size() == 1)
                        { // we don't have to call splitIntoParts, because a single polygon can only be a single part
                            PolygonsPart outline_part_here;
                            outline_part_here.add(new_outline[0]);
                            new_parts.push_back(outline_part_here);
                        }
                        else if (new_outline.size() > 1)
                        { // we don't know whether it's a multitude of parts because of newly introduced holes, or because the polygon has been split up
                            std::vector<PolygonsPart> new_parts_here = new_outline.splitIntoParts();
                            for (PolygonsPart& new_part_here : new_parts_here)
                            {
                                new_parts.push_back(new_part_here);
                            }
                        }
                        // change the infill area of the non-infill mesh which is to be filled with e.g. lines
                        other_part.infill_area_own = other_part.getOwnInfillArea().difference(part.outline);
                        // note: don't change the part.infill_area, because we change the structure of that area, while the basic area in which infill is printed remains the same
                        //       the infill area remains the same for combing
                    }
                }
                if (mesh.settings.get<ESurfaceMode>("magic_mesh_surface_mode") != ESurfaceMode::NORMAL)
                {
                    const Polygons& own_infill_area = other_part.getOwnInfillArea();
                    Polygons cut_lines = own_infill_area.intersectionPolyLines(layer.openPolyLines);
                    new_polylines.add(cut_lines);
                    // NOTE: closed polygons will be represented as polylines, which will be closed automatically in the PathOrderOptimizer
                    if (! own_infill_area.empty())
                    {
                        other_part.infill_area_own = own_infill_area.difference(layer.openPolyLines.offsetPolyLine(surface_line_width / 2));
                    }
                }
            }
        }

        layer.parts.clear();
        for (PolygonsPart& part : new_parts)
        {
            layer.parts.emplace_back();
            layer.parts.back().outline = part;
            layer.parts.back().boundaryBox.calculate(part);
        }

        if (mesh.settings.get<ESurfaceMode>("magic_mesh_surface_mode") != ESurfaceMode::NORMAL)
        {
            layer.openPolyLines = new_polylines;
        }

        if (layer.parts.size() > 0 || (mesh.settings.get<ESurfaceMode>("magic_mesh_surface_mode") != ESurfaceMode::NORMAL && layer.openPolyLines.size() > 0))
        {
            mesh.layer_nr_max_filled_layer = layer_idx; // last set by the highest non-empty layer
        }
    }
}

void FffPolygonGenerator::processDerivedWallsSkinInfill(SliceMeshStorage& mesh)
{
    if (mesh.settings.get<bool>("infill_support_enabled"))
    { // create gradual infill areas
        SkinInfillAreaComputation::generateInfillSupport(mesh);
    }

    // create gradual infill areas
    SkinInfillAreaComputation::generateGradualInfill(mesh);

    // SubDivCube Pre-compute Octree
    if (mesh.settings.get<coord_t>("infill_line_distance") > 0 && mesh.settings.get<EFillMethod>("infill_pattern") == EFillMethod::CUBICSUBDIV)
    {
        const Point3 mesh_middle = mesh.bounding_box.getMiddle();
        const Point infill_origin(mesh_middle.x + mesh.settings.get<coord_t>("infill_offset_x"), mesh_middle.y + mesh.settings.get<coord_t>("infill_offset_y"));
        SubDivCube::precomputeOctree(mesh, infill_origin);
    }

    // Pre-compute Cross Fractal
    if (mesh.settings.get<coord_t>("infill_line_distance") > 0 && (mesh.settings.get<EFillMethod>("infill_pattern") == EFillMethod::CROSS || mesh.settings.get<EFillMethod>("infill_pattern") == EFillMethod::CROSS_3D))
    {
        const std::string cross_subdivision_spec_image_file = mesh.settings.get<std::string>("cross_infill_density_image");
        std::ifstream cross_fs(cross_subdivision_spec_image_file.c_str());
        if (! cross_subdivision_spec_image_file.empty() && cross_fs.good())
        {
            mesh.cross_fill_provider = new SierpinskiFillProvider(mesh.bounding_box, mesh.settings.get<coord_t>("infill_line_distance"), mesh.settings.get<coord_t>("infill_line_width"), cross_subdivision_spec_image_file);
        }
        else
        {
            if (! cross_subdivision_spec_image_file.empty() && cross_subdivision_spec_image_file != " ")
            {
                LOGE("Cannot find density image: {}.", cross_subdivision_spec_image_file.c_str());
            }
            mesh.cross_fill_provider = new SierpinskiFillProvider(mesh.bounding_box, mesh.settings.get<coord_t>("infill_line_distance"), mesh.settings.get<coord_t>("infill_line_width"));
        }
    }

    // Pre-compute lightning fill (aka minfill, aka ribbed support vaults)
    if (mesh.settings.get<coord_t>("infill_line_distance") > 0 && mesh.settings.get<EFillMethod>("infill_pattern") == EFillMethod::LIGHTNING)
    {
        // TODO: Make all of these into new type pointers (but the cross fill things need to happen too then, otherwise it'd just look weird).
        mesh.lightning_generator = new LightningGenerator(mesh);
    }

    // combine infill
    SkinInfillAreaComputation::combineInfillLayers(mesh);

    // fuzzy skin
    if (mesh.settings.get<bool>("magic_fuzzy_skin_enabled"))
    {
        processFuzzyWalls(mesh);
    }
}

/*
 * This function is executed in a parallel region based on layer_nr.
 * When modifying make sure any changes does not introduce data races.
 *
 * processInsets only reads and writes data for the current layer
 */
void FffPolygonGenerator::processWalls(SliceMeshStorage& mesh, size_t layer_nr)
{
    SliceLayer* layer = &mesh.layers[layer_nr];
    SliceLayer* layer_upper = nullptr;
    if (layer_nr + 1 < mesh.layers.size())
        layer_upper = &mesh.layers[layer_nr + 1];
    WallsComputation walls_computation(mesh.settings, layer_nr, mesh.appliction);
    walls_computation.generateWalls(layer, layer_upper);
}

bool FffPolygonGenerator::isEmptyLayer(SliceDataStorage& storage, const unsigned int layer_idx)
{
    if (storage.support.generated && layer_idx < storage.support.supportLayers.size())
    {
        SupportLayer& support_layer = storage.support.supportLayers[layer_idx];
        if (! support_layer.support_infill_parts.empty() || ! support_layer.support_bottom.empty() || ! support_layer.support_roof.empty())
        {
            return false;
        }
    }
    for (SliceMeshStorage& mesh : storage.meshes)
    {
        if (layer_idx >= mesh.layers.size())
        {
            continue;
        }
        SliceLayer& layer = mesh.layers[layer_idx];
        if (mesh.settings.get<ESurfaceMode>("magic_mesh_surface_mode") != ESurfaceMode::NORMAL && layer.openPolyLines.size() > 0)
        {
            return false;
        }
        for (const SliceLayerPart& part : layer.parts)
        {
            if (part.print_outline.size() > 0)
            {
                return false;
            }
        }
    }
    return true;
}

void FffPolygonGenerator::removeEmptyFirstLayers(SliceDataStorage& storage, size_t& total_layers)
{
    size_t n_empty_first_layers = 0;
    for (size_t layer_idx = 0; layer_idx < total_layers; layer_idx++)
    {
        if (isEmptyLayer(storage, layer_idx))
        {
            n_empty_first_layers++;
        }
        else
        {
            break;
        }
    }

    if (n_empty_first_layers > 0)
    {
        LOGI("Removing {} layers because they are empty", n_empty_first_layers);
        const coord_t layer_height = application->get_layer_height();

        //belt ����ֵ
        //QString str = QString::number(-minZ.y, 'f', 2);
        //float num = layer_height * n_empty_first_layers / 1000.0f;
        //std::string str = std::to_string(num);
        //application->scene->current_mesh_group->settings.add("machine_belt_offset_Y", str);

        for (SliceMeshStorage& mesh : storage.meshes)
        {
            std::vector<SliceLayer>& layers = mesh.layers;
            if (layers.size() > n_empty_first_layers)
            {
                // transfer initial layer thickness to new initial layer
                layers[n_empty_first_layers].thickness = layers[0].thickness;
            }
            layers.erase(layers.begin(), layers.begin() + n_empty_first_layers);
            for (SliceLayer& layer : layers)
            {
                layer.printZ -= n_empty_first_layers * layer_height;
            }
            mesh.layer_nr_max_filled_layer -= n_empty_first_layers;
        }
        total_layers -= n_empty_first_layers;
        storage.support.layer_nr_max_filled_layer -= n_empty_first_layers;
        std::vector<SupportLayer>& support_layers = storage.support.supportLayers;
        support_layers.erase(support_layers.begin(), support_layers.begin() + n_empty_first_layers);
    }
}

/*
 * This function is executed in a parallel region based on layer_nr.
 * When modifying make sure any changes does not introduce data races.
 *
 * generateSkins read (depend on) data from mesh.layers[*].parts[*].insets and write mesh.layers[n].parts[*].skin_parts
 * generateInfill read mesh.layers[n].parts[*].{insets,skin_parts,boundingBox} and write mesh.layers[n].parts[*].infill_area
 *
 * processSkinsAndInfill read (depend on) mesh.layers[*].parts[*].{insets,boundingBox}.
 *                       write mesh.layers[n].parts[*].{skin_parts,infill_area}.
 */
void FffPolygonGenerator::processSkinsAndInfill(SliceMeshStorage& mesh, const LayerIndex layer_nr, bool process_infill)
{
    if (mesh.settings.get<ESurfaceMode>("magic_mesh_surface_mode") == ESurfaceMode::SURFACE)
    {
        return;
    }

    SkinInfillAreaComputation skin_infill_area_computation(layer_nr, mesh, process_infill);
    skin_infill_area_computation.generateSkinsAndInfill();

    INTERRUPT_RETURN("FffPolygonGenerator::processSkinsAndInfill");

    if (mesh.settings.get<bool>("ironing_enabled") && (! mesh.settings.get<bool>("ironing_only_highest_layer") || mesh.layer_nr_max_filled_layer == layer_nr))
    {
        // Generate the top surface to iron over.
        mesh.layers[layer_nr].top_surface.setAreasFromMeshAndLayerNumber(mesh, layer_nr);
    }
}

void FffPolygonGenerator::computePrintHeightStatistics(SliceDataStorage& storage)
{
    const size_t extruder_count = (size_t)application->extruderCount();

    std::vector<int>& max_print_height_per_extruder = storage.max_print_height_per_extruder;
    assert(max_print_height_per_extruder.size() == 0 && "storage.max_print_height_per_extruder shouldn't have been initialized yet!");
    const int raft_layers = Raft::getTotalExtraLayers(application);
    max_print_height_per_extruder.resize(extruder_count, -(raft_layers + 1)); // Initialize all as -1 (or lower in case of raft).
    { // compute max_object_height_per_extruder
        // Height of the meshes themselves.
        for (SliceMeshStorage& mesh : storage.meshes)
        {
            if (mesh.settings.get<bool>("anti_overhang_mesh") || mesh.settings.get<bool>("support_mesh"))
            {
                continue; // Special type of mesh that doesn't get printed.
            }
            for (size_t extruder_nr = 0; extruder_nr < extruder_count; extruder_nr++)
            {
                for (LayerIndex layer_nr = static_cast<LayerIndex>(mesh.layers.size()) - 1; layer_nr > max_print_height_per_extruder[extruder_nr]; layer_nr--)
                {
                    if (mesh.getExtruderIsUsed(extruder_nr, layer_nr))
                    {
                        assert(max_print_height_per_extruder[extruder_nr] <= layer_nr);
                        max_print_height_per_extruder[extruder_nr] = layer_nr;
                    }
                }
            }
        }

        // Height of where the support reaches.
        const Settings& mesh_group_settings = application->currentGroup()->settings;
        const size_t support_infill_extruder_nr = mesh_group_settings.get<ExtruderTrain&>("support_infill_extruder_nr").extruder_nr; // TODO: Support extruder should be configurable per object.
        max_print_height_per_extruder[support_infill_extruder_nr] = std::max(max_print_height_per_extruder[support_infill_extruder_nr], storage.support.layer_nr_max_filled_layer);
        const size_t support_roof_extruder_nr = mesh_group_settings.get<ExtruderTrain&>("support_roof_extruder_nr").extruder_nr; // TODO: Support roof extruder should be configurable per object.
        max_print_height_per_extruder[support_roof_extruder_nr] = std::max(max_print_height_per_extruder[support_roof_extruder_nr], storage.support.layer_nr_max_filled_layer);
        const size_t support_bottom_extruder_nr = mesh_group_settings.get<ExtruderTrain&>("support_bottom_extruder_nr").extruder_nr; // TODO: Support bottom extruder should be configurable per object.
        max_print_height_per_extruder[support_bottom_extruder_nr] = std::max(max_print_height_per_extruder[support_bottom_extruder_nr], storage.support.layer_nr_max_filled_layer);

        // Height of where the platform adhesion reaches.
        const EPlatformAdhesion adhesion_type = application->get_adhesion_type();
        switch (adhesion_type)
        {
        case EPlatformAdhesion::SKIRT:
        case EPlatformAdhesion::BRIM:
        {
            const size_t skirt_brim_extruder_nr = mesh_group_settings.get<ExtruderTrain&>("skirt_brim_extruder_nr").extruder_nr;
            max_print_height_per_extruder[skirt_brim_extruder_nr] = std::max(0, max_print_height_per_extruder[skirt_brim_extruder_nr]); // Includes layer 0.
            break;
        }
        case EPlatformAdhesion::RAFT:
        case EPlatformAdhesion::SIMPLERAFT:
        {
            const size_t base_extruder_nr = mesh_group_settings.get<ExtruderTrain&>("raft_base_extruder_nr").extruder_nr;
            max_print_height_per_extruder[base_extruder_nr] = std::max(-raft_layers, max_print_height_per_extruder[base_extruder_nr]); // Includes the lowest raft layer.
            const size_t interface_extruder_nr = mesh_group_settings.get<ExtruderTrain&>("raft_interface_extruder_nr").extruder_nr;
            max_print_height_per_extruder[interface_extruder_nr] = std::max(-raft_layers + 1, max_print_height_per_extruder[interface_extruder_nr]); // Includes the second-lowest raft layer.
            const size_t surface_extruder_nr = mesh_group_settings.get<ExtruderTrain&>("raft_surface_extruder_nr").extruder_nr;
            max_print_height_per_extruder[surface_extruder_nr] = std::max(-1, max_print_height_per_extruder[surface_extruder_nr]); // Includes up to the first layer below the model (so -1).
            break;
        }
        default:
            break; // No adhesion, so no maximum necessary.
        }
    }

    storage.max_print_height_order = order(max_print_height_per_extruder);
    if (extruder_count >= 2)
    {
        int second_highest_extruder = storage.max_print_height_order[extruder_count - 2];
        storage.max_print_height_second_to_last_extruder = max_print_height_per_extruder[second_highest_extruder];
    }
    else
    {
        storage.max_print_height_second_to_last_extruder = -(raft_layers + 1);
    }
}


void FffPolygonGenerator::processOozeShield(SliceDataStorage& storage)
{
    const Settings& mesh_group_settings = application->currentGroup()->settings;
    if (! mesh_group_settings.get<bool>("ooze_shield_enabled"))
    {
        return;
    }

    INTERRUPT_RETURN("FffPolygonGenerator::processOozeShield");

    const coord_t ooze_shield_dist = mesh_group_settings.get<coord_t>("ooze_shield_dist");

    for (int layer_nr = 0; layer_nr <= storage.max_print_height_second_to_last_extruder; layer_nr++)
    {
        constexpr bool around_support = true;
        constexpr bool around_prime_tower = false;
        storage.oozeShield.push_back(storage.getLayerOutlines(layer_nr, around_support, around_prime_tower).offset(ooze_shield_dist, ClipperLib::jtRound).getOutsidePolygons());
    }

    const AngleDegrees angle = mesh_group_settings.get<AngleDegrees>("ooze_shield_angle");
    if (angle <= 89)
    {
        const coord_t allowed_angle_offset = tan(mesh_group_settings.get<AngleRadians>("ooze_shield_angle")) * application->get_layer_height(); // Allow for a 60deg angle in the oozeShield.
        for (LayerIndex layer_nr = 1; layer_nr <= storage.max_print_height_second_to_last_extruder; layer_nr++)
        {
            storage.oozeShield[layer_nr] = storage.oozeShield[layer_nr].unionPolygons(storage.oozeShield[layer_nr - 1].offset(-allowed_angle_offset));
        }
        for (LayerIndex layer_nr = storage.max_print_height_second_to_last_extruder; layer_nr > 0; layer_nr--)
        {
            storage.oozeShield[layer_nr - 1] = storage.oozeShield[layer_nr - 1].unionPolygons(storage.oozeShield[layer_nr].offset(-allowed_angle_offset));
        }
    }

    const float largest_printed_area = 1.0; // TODO: make var a parameter, and perhaps even a setting?
    for (LayerIndex layer_nr = 0; layer_nr <= storage.max_print_height_second_to_last_extruder; layer_nr++)
    {
        storage.oozeShield[layer_nr].removeSmallAreas(largest_printed_area);
    }
}

void FffPolygonGenerator::processDraftShield(SliceDataStorage& storage)
{
    const size_t draft_shield_layers = getDraftShieldLayerCount(storage.print_layer_count);
    if (draft_shield_layers <= 0)
    {
        return;
    }
    const Settings& mesh_group_settings = application->currentGroup()->settings;
    const coord_t layer_height = application->get_layer_height();

    const unsigned int layer_skip = 500 / layer_height + 1;

    Polygons& draft_shield = storage.draft_protection_shield;
    for (unsigned int layer_nr = 0; layer_nr < storage.print_layer_count && layer_nr < draft_shield_layers; layer_nr += layer_skip)
    {
        constexpr bool around_support = true;
        constexpr bool around_prime_tower = false;
        draft_shield = draft_shield.unionPolygons(storage.getLayerOutlines(layer_nr, around_support, around_prime_tower));
    }

    const coord_t draft_shield_dist = mesh_group_settings.get<coord_t>("draft_shield_dist");
    storage.draft_protection_shield = draft_shield.approxConvexHull(draft_shield_dist);

    // Extra offset has rounded joints, so simplify again.
    coord_t maximum_resolution = 0; // Draft shield is printed with every extruder, so resolve with the max() or min() of them to meet the requirements of all extruders.
    coord_t maximum_deviation = std::numeric_limits<coord_t>::max();
    for (const ExtruderTrain& extruder : application->extruders())
    {
        maximum_resolution = std::max(maximum_resolution, extruder.settings.get<coord_t>("meshfix_maximum_resolution"));
        maximum_deviation = std::min(maximum_deviation, extruder.settings.get<coord_t>("meshfix_maximum_deviation"));
    }
    storage.draft_protection_shield = Simplify(maximum_resolution, maximum_deviation, 0).polygon(storage.draft_protection_shield);
}

void FffPolygonGenerator::processPlatformAdhesion(SliceDataStorage& storage)
{
    const Settings& mesh_group_settings = application->currentGroup()->settings;
    ExtruderTrain& train = mesh_group_settings.get<ExtruderTrain&>("skirt_brim_extruder_nr");

    Polygons first_layer_outline;
    coord_t primary_line_count;

    EPlatformAdhesion adhesion_type = application->get_adhesion_type();

    if (adhesion_type == EPlatformAdhesion::SKIRT)
    {
        primary_line_count = train.settings.get<size_t>("skirt_line_count");
        SkirtBrim::getFirstLayerOutline(storage, primary_line_count, true, first_layer_outline);
        SkirtBrim::generate(storage, first_layer_outline, train.settings.get<coord_t>("skirt_gap"), primary_line_count);
    }

	if (adhesion_type == EPlatformAdhesion::LACE)
	{
		primary_line_count = train.settings.get<size_t>("skirt_line_count");
		SkirtBrim::getFirstLayerOutline(storage, primary_line_count, true, first_layer_outline);

		ClipperLib::Paths pathsTemp;
        ClipperLib::Paths outlinePaths;
        for (ClipperLib::Paths::iterator it = first_layer_outline.begin();it != first_layer_outline.end();it ++)
        {
            outlinePaths.push_back(*it);
        }

		pathsTemp = SkirtBrim::skirt2Lace(outlinePaths);
		first_layer_outline.clear();
        for (ClipperLib::Path& apath:pathsTemp)
        {
            Polygon aPolygon(apath);
            first_layer_outline.add(aPolygon);
        }
		SkirtBrim::generate(storage, first_layer_outline, train.settings.get<coord_t>("skirt_gap"), primary_line_count);
	}

    // Generate any brim for the prime tower, should happen _after_ any skirt, but _before_ any other brim (since FffGCodeWriter assumes that the outermost contour is last).
    if ((adhesion_type != EPlatformAdhesion::RAFT && adhesion_type != EPlatformAdhesion::SIMPLERAFT) && storage.primeTower.enabled && mesh_group_settings.get<bool>("prime_tower_brim_enable"))
    {
        constexpr bool dont_allow_helpers = false;
        SkirtBrim::generate(storage, storage.primeTower.outer_poly, 0, train.settings.get<size_t>("brim_line_count"), dont_allow_helpers);
		//storage.skirt_brim[1] = storage.skirt_brim[0];
		//storage.skirt_brim[0].clear();
    }

	std::vector<size_t> vct_primary_line_count;
    switch (adhesion_type)
    {
    case EPlatformAdhesion::SKIRT:
        // Already done, because of prime-tower-brim & ordering, see above.
        break;
    case EPlatformAdhesion::BRIM:
        primary_line_count = train.settings.get<size_t>("brim_line_count");
        SkirtBrim::getFirstLayerOutline(storage, primary_line_count, false, first_layer_outline);
        SkirtBrim::generate(storage, first_layer_outline, 0, primary_line_count);
        break;
	case EPlatformAdhesion::AUTOBRIM:
        SkirtBrim::generateAutoBrim(storage, first_layer_outline,vct_primary_line_count);
		SkirtBrim::generateEX(storage, first_layer_outline, 0, vct_primary_line_count);
		break;
    case EPlatformAdhesion::RAFT:
    case EPlatformAdhesion::SIMPLERAFT:
        Raft::generate(storage);
        break;
    case EPlatformAdhesion::NONE:
        if (mesh_group_settings.get<bool>("support_brim_enable"))
        {
            SkirtBrim::generate(storage, Polygons(), 0, 0);
        }
        break;
    }

    Polygon machine_rect;
    machine_rect.add(Point(storage.machine_size.min.x, storage.machine_size.min.y));
    machine_rect.add(Point(storage.machine_size.min.x, storage.machine_size.max.y));
    machine_rect.add(Point(storage.machine_size.max.x, storage.machine_size.max.y));
    machine_rect.add(Point(storage.machine_size.max.x, storage.machine_size.min.y));
    Polygons machine_rects;
    machine_rects.add(machine_rect);
    // Also apply maximum_[deviation|resolution] to skirt/brim.

    coord_t max_resolution = train.settings.get<coord_t>("meshfix_maximum_resolution");
    coord_t max_deviation = train.settings.get<coord_t>("meshfix_maximum_deviation");
    coord_t max_area_deviation = train.settings.get<coord_t>("meshfix_maximum_extrusion_area_deviation");

    Simplify simplifier(max_resolution, max_deviation, max_area_deviation);

    for (Polygons& polygons : storage.skirt_brim)
    {
        polygons = simplifier.polygon(polygons.intersection(machine_rects));
		std::reverse(polygons.begin(), polygons.end());
		for (auto& aPolygon : polygons)
		{
			if (ClipperLib::Orientation(aPolygon))
			{
				ClipperLib::ReversePath(aPolygon);
			}
		}
    }
}


void FffPolygonGenerator::processFuzzyWalls(SliceMeshStorage& mesh)
{
    if (mesh.settings.get<size_t>("wall_line_count") == 0)
    {
        return;
    }

    const coord_t line_width = mesh.settings.get<coord_t>("line_width");
    const bool apply_outside_only = mesh.settings.get<bool>("magic_fuzzy_skin_outside_only");
    const coord_t fuzziness = mesh.settings.get<coord_t>("magic_fuzzy_skin_thickness");
    const coord_t avg_dist_between_points = mesh.settings.get<coord_t>("magic_fuzzy_skin_point_dist");
    const coord_t min_dist_between_points = avg_dist_between_points * 3 / 4; // hardcoded: the point distance may vary between 3/4 and 5/4 the supplied value
    const coord_t range_random_point_dist = avg_dist_between_points / 2;
    unsigned int start_layer_nr = (application->get_adhesion_type() == EPlatformAdhesion::BRIM) ? 1 : 0; // don't make fuzzy skin on first layer if there's a brim

    auto hole_area = Polygons();
    std::function<bool(const bool&, const ExtrusionJunction&)> accumulate_is_in_hole = [](const bool& prev_result, const ExtrusionJunction& junction) { return false; };

    for (unsigned int layer_nr = start_layer_nr; layer_nr < mesh.layers.size(); layer_nr++)
    {
        INTERRUPT_RETURN("FffPolygonGenerator::processFuzzyWalls");

        SliceLayer& layer = mesh.layers[layer_nr];
        for (SliceLayerPart& part : layer.parts)
        {
            std::vector<VariableWidthLines> result_paths;
            for (auto& toolpath : part.wall_toolpaths)
            {
                if (toolpath.front().inset_idx != 0)
                {
                    result_paths.push_back(toolpath);
                    continue;
                }

                auto& result_lines = result_paths.emplace_back();

                if (apply_outside_only)
                {
                    hole_area = part.print_outline.getOutsidePolygons().offset(-line_width);
                    accumulate_is_in_hole = [&hole_area](const bool& prev_result, const ExtrusionJunction& junction) { return prev_result || hole_area.inside(junction.p); };
                }
                for (auto& line : toolpath)
                {
                    if (apply_outside_only && std::accumulate(line.begin(), line.end(), false, accumulate_is_in_hole))
                    {
                        result_lines.push_back(line);
                        continue;
                    }

                    auto& result = result_lines.emplace_back();
                    result.inset_idx = line.inset_idx;
                    result.is_odd = line.is_odd;
                    result.is_closed = line.is_closed;

                    // generate points in between p0 and p1
                    int64_t dist_left_over = (min_dist_between_points / 4) + rand() % (min_dist_between_points / 4); // the distance to be traversed on the line before making the first new point
                    auto* p0 = &line.front();
                    for (auto& p1 : line)
                    {
                        if (p0->p == p1.p) // avoid seams
                        {
                            result.emplace_back(p1.p, p1.w, p1.perimeter_index);
                            continue;
                        }

                        // 'a' is the (next) new point between p0 and p1
                        const Point p0p1 = p1.p - p0->p;
                        const int64_t p0p1_size = vSize(p0p1);
                        int64_t p0pa_dist = dist_left_over;
                        if (p0pa_dist >= p0p1_size)
                        {
                            const Point p = p1.p - (p0p1 / 2);
                            const double width = (p1.w * vSize(p1.p - p) + p0->w * vSize(p0->p - p)) / p0p1_size;
                            result.emplace_back(p, width, p1.perimeter_index);
                        }
                        for (; p0pa_dist < p0p1_size; p0pa_dist += min_dist_between_points + rand() % range_random_point_dist)
                        {
                            const int r = rand() % (fuzziness * 2) - fuzziness;
                            const Point perp_to_p0p1 = turn90CCW(p0p1);
                            const Point fuzz = normal(perp_to_p0p1, r);
                            const Point pa = p0->p + normal(p0p1, p0pa_dist);
                            const double width = (p1.w * vSize(p1.p - pa) + p0->w * vSize(p0->p - pa)) / p0p1_size;
                            result.emplace_back(pa + fuzz, width, p1.perimeter_index);
                        }
                        // p0pa_dist > p0p1_size now because we broke out of the for-loop
                        dist_left_over = p0pa_dist - p0p1_size;

                        p0 = &p1;
                    }
                    while (result.size() < 3)
                    {
                        size_t point_idx = line.size() - 2;
                        result.emplace_back(line[point_idx].p, line[point_idx].w, line[point_idx].perimeter_index);
                        if (point_idx == 0)
                        {
                            break;
                        }
                        point_idx--;
                    }
                    if (result.size() < 3)
                    {
                        result.clear();
                        for (auto& p : line)
                        {
                            result.emplace_back(p.p, p.w, p.perimeter_index);
                        }
                    }
                    if (line.back().p == line.front().p) // avoid seams
                    {
                        result.back().p = result.front().p;
                    }
                }
            }
            part.wall_toolpaths = result_paths;
        }
    }
}


} // namespace cura52
