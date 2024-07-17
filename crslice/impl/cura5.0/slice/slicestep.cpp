#include "slice/slicestep.h"
#include "slice/sliceddata.h"
#include "slicer.h"

namespace cura52 {
	void sliceMesh(SliceContext* application, Mesh* mesh, const coord_t thickness, const size_t slice_layer_count,
		bool use_variable_layer_heights, std::vector<AdaptiveLayer>* adaptive_layers, /*out*/ SlicedData& data)
	{
		Slicer impl(application, mesh, thickness, slice_layer_count, use_variable_layer_heights, adaptive_layers);

		int size = (int)impl.layers.size();
		if (size > 0)
		{
			data.layers.resize(size);
			for (int i = 0; i < size; ++i)
			{
				SlicerLayer& layer = impl.layers.at(i);
				SlicedLayer& l = data.layers.at(i);

				l.z = layer.z;
				l.openPolylines.paths.swap(layer.openPolylines.paths);
				l.polygons.paths.swap(layer.polygons.paths);
				l.polygons.colors.swap(layer.polygons.colors);
				l.polygons.polys.swap(layer.polygons.polys);
				data.mesh = mesh;
			}
		}
	}

	void sliceMesh(SliceContext* application, Mesh* mesh, const coord_t thickness, const size_t slice_layer_count,
		bool use_variable_layer_heights, std::vector<AdaptiveLayer>* adaptive_layers, std::vector<SlicerLayer>& layers)
	{
		Slicer impl(application, mesh, thickness, slice_layer_count, use_variable_layer_heights, adaptive_layers);
		layers = impl.layers;
	}
}
