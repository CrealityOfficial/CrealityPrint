//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef TREESUPPORTCIRCLET_H
#define TREESUPPORTCIRCLET_H

#include "utils/Coord_t.h"
#include "utils/polygon.h"
#include <mutex>

#define SUPPORT_TREE_CIRCLE_RESOLUTION 25 // The number of vertices in each circle.

namespace cura54
{
    typedef cura52::coord_t coord_t;
    class TreeSupportBaseCircleT
    {
    protected:
        inline static cura52::Polygon base_circle;
        inline static bool circle_generated = false;

    public:
        inline static constexpr int64_t base_radius = 50;

        static cura52::Polygon getBaseCircle()
        {
            if (circle_generated)
            {
                return base_circle;
            }
            else
            {
                std::mutex critical_sections;
                std::lock_guard<std::mutex> critical_section_progress(critical_sections);
                if (circle_generated)
                {
                    return base_circle;
                }

                constexpr auto support_tree_circle_resolution = 12; // The number of vertices in each circle.
                cura52::Polygon circle;
                // for (const uint64_t i : ranges::views::iota(0, support_tree_circle_resolution))
                for (uint64_t i = 0; i < support_tree_circle_resolution; i++)
                {
                    const cura52::AngleRadians angle = static_cast<double>(i) / support_tree_circle_resolution * TAU;
                    circle.emplace_back(static_cast<coord_t>(std::cos(angle) * base_radius), static_cast<coord_t>(std::sin(angle) * base_radius));
                }
                base_circle = cura52::Polygon(circle);
                circle_generated = true;
                return base_circle;
            }
        }
    };

} // namespace cura


#endif // TREESUPPORTCIRCLE_H
