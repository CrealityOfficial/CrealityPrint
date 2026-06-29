#ifndef slic3r_MultiMaterialSegmentation_hpp_
#define slic3r_MultiMaterialSegmentation_hpp_

#include <utility>
#include <vector>

namespace Slic3r {

class ModelVolume;
class FacetsAnnotation;
class PrintObject;
class ExPolygon;
using ExPolygons = std::vector<ExPolygon>;


struct ColoredLine
{
    Line line;
    int  color;
    int  poly_idx       = -1;
    int  local_line_idx = -1;
};

using ColoredLines = std::vector<ColoredLine>;

enum class IncludeTopAndBottomLayers {
    Yes,
    No
};

struct ModelVolumeFacetsInfo {
    const FacetsAnnotation &facets_annotation;
    // Indicate if model volume is painted.
    const bool              is_painted;
    // Indicate if the default extruder (TriangleStateType::NONE) should be replaced with the volume extruder.
    const bool              replace_default_extruder;
};

// Returns segmentation based on painting in segmentation gizmos.
std::vector<std::vector<ExPolygons>> segmentation_by_painting(const PrintObject                                               &print_object,
                                                              const std::function<ModelVolumeFacetsInfo(const ModelVolume &)> &extract_facets_info,
                                                              size_t                                                           num_facets_states,
                                                              float                                                            segmentation_max_width,
                                                              float                                                            segmentation_interlocking_depth,
                                                              bool                                                             segmentation_interlocking_beam,
                                                              IncludeTopAndBottomLayers                                        include_top_and_bottom_layers,
                                                              const std::function<void()>                                     &throw_on_cancel_callback,
                                                              const std::vector<ExPolygons>                                   *override_input_expolygons = nullptr);

// Returns MMU segmentation based on painting in MMU segmentation gizmo
std::vector<std::vector<ExPolygons>> multi_material_segmentation_by_painting(const PrintObject &print_object, const std::function<void()> &throw_on_cancel_callback);

// Returns fuzzy skin segmentation based on painting in fuzzy skin segmentation gizmo.
// clean_input_expolygons (optional): per-layer outline captured BEFORE color segmentation split
// the layer into painted/non-painted regions; used as the Voronoi input to avoid degeneration
// along color seams. Pass nullptr to read the (possibly color-split) layer region slices.
std::vector<std::vector<ExPolygons>> fuzzy_skin_segmentation_by_painting(const PrintObject &print_object, const std::function<void()> &throw_on_cancel_callback, const std::vector<ExPolygons> *clean_input_expolygons = nullptr);

} // namespace Slic3r

namespace boost::polygon {
template<> struct geometry_concept<Slic3r::ColoredLine>
{
    typedef segment_concept type;
};

template<> struct segment_traits<Slic3r::ColoredLine>
{
    typedef coord_t       coordinate_type;
    typedef Slic3r::Point point_type;

    static inline point_type get(const Slic3r::ColoredLine &line, const direction_1d &dir)
    {
        return dir.to_int() ? line.line.b : line.line.a;
    }
};
} // namespace boost::polygon

#endif // slic3r_MultiMaterialSegmentation_hpp_
