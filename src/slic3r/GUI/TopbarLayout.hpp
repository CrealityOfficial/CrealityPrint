#pragma once

namespace Slic3r {
namespace GUI {

enum class TopbarLayout { Normal, Compact, Overflow };

struct TopbarWidths {
    int normal = 0;
    int compact = 0;
    int overflow = 0;

    TopbarLayout layout_for(int available_width) const
    {
        if (available_width >= normal)
            return TopbarLayout::Normal;
        return available_width >= compact ? TopbarLayout::Compact : TopbarLayout::Overflow;
    }

    int minimum_for_screen(int client_width) const
    {
        return compact <= client_width ? compact : overflow;
    }
};

} // namespace GUI
} // namespace Slic3r
