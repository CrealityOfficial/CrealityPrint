#include "../../src/slic3r/GUI/TopbarLayout.hpp"
#include <cstdlib>
#include <iostream>

using namespace Slic3r::GUI;

static void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

int main()
{
    // Different translated text lengths and DPI-scaled measurements.
    for (const TopbarWidths logical : {TopbarWidths{1100, 960, 875}, TopbarWidths{1250, 1110, 1025}}) {
        for (int scale_percent : {100, 125, 150, 200}) {
            auto scale = [scale_percent](int value) { return value * scale_percent / 100; };
            const TopbarWidths widths{scale(logical.normal), scale(logical.compact), scale(logical.overflow)};
            require(widths.layout_for(widths.normal) == TopbarLayout::Normal, "Exact normal fit");
            require(widths.layout_for(widths.normal - 1) == TopbarLayout::Compact, "Shrink into compact");
            require(widths.layout_for(widths.compact) == TopbarLayout::Compact, "Exact compact fit");
            require(widths.layout_for(widths.compact - 1) == TopbarLayout::Overflow, "Shrink into overflow");
            require(widths.minimum_for_screen(widths.normal) == widths.compact, "Wide screen keeps compact minimum");
            require(widths.minimum_for_screen(widths.compact) == widths.compact, "Exact screen fit");
            require(widths.minimum_for_screen(widths.compact - 1) == widths.overflow, "Small screen uses overflow minimum");
            require(widths.layout_for(widths.overflow) == TopbarLayout::Overflow, "Window minimum keeps overflow controls");

            // Every supported width must choose a layout whose controls fit.
            for (int available = widths.overflow; available <= widths.normal + 20; ++available) {
                const auto layout = widths.layout_for(available);
                const int required = layout == TopbarLayout::Normal ? widths.normal :
                    layout == TopbarLayout::Compact ? widths.compact : widths.overflow;
                require(required <= available, "Chosen controls must fit the available width");
            }
        }
    }
    const TopbarWidths long_labels{1200, 1200, 1100};
    require(long_labels.layout_for(1199) == TopbarLayout::Overflow, "Long labels can have no compact savings");
    require(long_labels.layout_for(1200) == TopbarLayout::Normal, "Restore all controls for long labels");
    std::cout << "Topbar layout checks passed\n";
}
