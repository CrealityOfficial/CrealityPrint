#include "../../src/slic3r/GUI/GCodeRenderer/PreviewLayerRange.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

using Slic3r::GUI::clamp_preview_layer_range;

namespace {
void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}
}

int main()
{
    try {
        // The dump contains ten doubles, but the old slider still selects 101.
        const std::vector<double> layers{0.2, 0.4, 0.6, 0.8, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0};
        const auto shorter = clamp_preview_layer_range({0, 101}, layers.size());
        require(shorter && (*shorter)[0] == 0 && (*shorter)[1] == 9, "shorter result must clamp the old upper layer");
        require(layers.at(shorter->back()) == 2.0, "height warning must still inspect the last valid layer");
        require(layers.at(shorter->back()) > 1.5, "clamping must not hide a real height overflow");

        const auto old_selection = clamp_preview_layer_range({50, 101}, layers.size());
        require(old_selection && *old_selection == std::array<unsigned int, 2>{9, 9},
                "a selection entirely above the new result must remain ordered and in bounds");

        const auto selected = clamp_preview_layer_range({2, 5}, layers.size());
        require(selected && *selected == std::array<unsigned int, 2>{2, 5}, "valid selection must be preserved");
        require(layers.at(selected->back()) == 1.2, "valid upper layer must not be replaced with the final layer");

        require(!clamp_preview_layer_range({0, 101}, 0), "empty result has no readable layer");
        require(!clamp_preview_layer_range({0, 0}, 0), "zero is not a valid index into an empty result");
        const auto single = clamp_preview_layer_range({50, 101}, 1);
        require(single && *single == std::array<unsigned int, 2>{0, 0}, "single-layer result must select zero");

        const auto reversed = clamp_preview_layer_range({8, 3}, layers.size());
        require(reversed && *reversed == std::array<unsigned int, 2>{3, 3}, "reversed range must preserve a valid upper layer");
        const auto maximum = std::numeric_limits<unsigned int>::max();
        const auto wrapped = clamp_preview_layer_range({maximum, maximum}, layers.size());
        require(wrapped && *wrapped == std::array<unsigned int, 2>{9, 9}, "wrapped slider indices must not escape the result");

        std::cout << "Preview layer range regression tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
