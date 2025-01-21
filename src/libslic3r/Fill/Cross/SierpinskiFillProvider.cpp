// Copyright (c) 2022 Ultimaker B.V.
// CuraEngine is released under the terms of the AGPLv3 or higher

//#include "ccglobal/log.h"

#include "ImageBasedDensityProvider.h"
#include "SierpinskiFillProvider.h"
//#include "UniformDensityProvider.h"
//#include "utils/AABB3D.h"
//#include "utils/math.h"
//#include "utils/polygon.h"

namespace Slic3r::Cross
{


constexpr bool SierpinskiFillProvider::get_constructor;
constexpr bool SierpinskiFillProvider::use_dithering;

SierpinskiFillProvider::SierpinskiFillProvider(const BoundingBox aabb_3d, coord_t min_line_distance, const coord_t line_width)
    : fractal_config(getFractalConfig(aabb_3d, min_line_distance))
    , density_provider(new UniformDensityProvider((float)line_width / min_line_distance))
    , fill_pattern_for_all_layers(std::in_place, *density_provider, fractal_config.aabb, fractal_config.depth, line_width, use_dithering)
{
}

//SierpinskiFillProvider::SierpinskiFillProvider(const BoundingBox aabb_3d, coord_t min_line_distance, coord_t line_width, std::string cross_subdisivion_spec_image_file)
//    : fractal_config(getFractalConfig(aabb_3d, min_line_distance))
//    , density_provider(new ImageBasedDensityProvider(cross_subdisivion_spec_image_file, aabb_3d.flatten()))
//    , fill_pattern_for_all_layers(std::in_place, *density_provider, fractal_config.aabb, fractal_config.depth, line_width, use_dithering)
//{
//}

Polygon SierpinskiFillProvider::generate(InfillPattern pattern, coord_t z, coord_t line_width, coord_t pocket_size) const
{
    try {
        if (fill_pattern_for_all_layers)
        {
            if (pattern != ipCross)
            {
                return fill_pattern_for_all_layers->generateCross(z, line_width / 2, pocket_size);
            }
            else
            {
                return fill_pattern_for_all_layers->generateCross();
            }
        }
        else
        {
            Polygon ret;
            BOOST_LOG_TRIVIAL(error) << boost::format("Different density sierpinski fill for different layers is not implemented yet!");
            std::exit(-1);
            return ret;
        }
    }
    catch (const std::exception& ex)
    {
        return Polygon();
    }
}

SierpinskiFillProvider::~SierpinskiFillProvider()
{
    if (density_provider)
    {
        delete density_provider;
    }
}

SierpinskiFillProvider::FractalConfig SierpinskiFillProvider::getFractalConfig(const BoundingBox aabb_3d, coord_t min_line_distance)
{
    static constexpr float sqrt2 = 1.41421356237f;
    BoundingBox model_aabb = aabb_3d;
    Point model_aabb_size = model_aabb.max - model_aabb.min;
    coord_t max_side_length = std::max(model_aabb_size.x(), model_aabb_size.y());
    Point model_middle = model_aabb.center();

    int depth = 0;
    coord_t aabb_size = min_line_distance;
    while (aabb_size < max_side_length)
    {
        aabb_size *= 2;
        depth += 2;
    }
    const float half_sqrt2 = .5 * sqrt2;
    if (depth > 0 && aabb_size * half_sqrt2 >= max_side_length)
    {
        aabb_size *= half_sqrt2;
        depth--;
    }

    Point radius(aabb_size / 2, aabb_size / 2);
    BoundingBox aabb(model_middle - radius, model_middle + radius);

    return FractalConfig{ depth, aabb };
}


} // namespace cura52
