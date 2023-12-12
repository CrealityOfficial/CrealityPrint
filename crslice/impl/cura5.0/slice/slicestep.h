#ifndef CRSLICE_MESHSLICE_STEP_1668840402293_H
#define CRSLICE_MESHSLICE_STEP_1668840402293_H
#include "utils/Coord_t.h"
#include "magic/AdaptiveLayerHeights.h"

namespace cura52 {
	class SliceContext;
	class Mesh;
	class SlicedData;
	class SlicerLayer;
	void sliceMesh(SliceContext* _application, Mesh* i_mesh, const coord_t thickness, const size_t slice_layer_count,
		bool use_variable_layer_heights, std::vector<AdaptiveLayer>* adaptive_layers, /*out*/ SlicedData& data);

	void sliceMesh(SliceContext* _application, Mesh* i_mesh, const coord_t thickness, const size_t slice_layer_count,
		bool use_variable_layer_heights, std::vector<AdaptiveLayer>* adaptive_layers, std::vector<SlicerLayer>& layers);
}
#endif // CRSLICE_MESHSLICE_STEP_1668840402293_H
