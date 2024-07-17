#ifndef ORCA_INFILL_SIERPINSKI_FILL_PROVIDER_H
#define ORCA_INFILL_SIERPINSKI_FILL_PROVIDER_H

#include <optional>

#include "SierpinskiFill.h"
//#include "types/EnumSettings.h" //For EFillMethod.
#include "PrintConfig.hpp"

namespace Slic3r::Cross
{
class DensityProvider;

/*!
 * Class for generating infill patterns using the SierpinskiFill class.
 * 
 * This class handles determining the maximum recursion depth, the initial triangle
 * and in general the configuration the SierpinskiFill class requires to be used as fill pattern.
 * 
 * This class also handles the density provider which is used to determine the local density at each location - if there is one.
 */
class SierpinskiFillProvider
{
    static constexpr bool get_constructor = true;
    static constexpr bool use_dithering = true; // !< Whether to employ dithering and error propagation
protected:
    //! Basic parameters from which to start constructing the sierpinski fractal
    struct FractalConfig
    {
        int depth; //!< max recursion depth
        BoundingBox aabb; //!< The bounding box of the initial Triangles in the Sierpinski curve
    };
public:
    FractalConfig fractal_config;
    DensityProvider* density_provider; //!< The object which determines the requested density at each region
    std::optional<SierpinskiFill> fill_pattern_for_all_layers; //!< The fill pattern if one and the same pattern is used on all layers

    SierpinskiFillProvider(const BoundingBox aabb_3d, coord_t min_line_distance, const coord_t line_width);

    //SierpinskiFillProvider(const BoundingBox aabb_3d, coord_t min_line_distance, coord_t line_width, std::string cross_subdisivion_spec_image_file);

    Polygon generate(InfillPattern pattern, coord_t z, coord_t line_width, coord_t pocket_size) const;

    ~SierpinskiFillProvider();
protected:
    /*!
     * Get the parameters with which to generate a sierpinski fractal for this object
     */
    FractalConfig getFractalConfig(const BoundingBox aabb_3d, coord_t min_line_distance);
};
} // namespace Slic3r::Cross


#endif // ORCA_INFILL_SIERPINSKI_FILL_PROVIDER_H
