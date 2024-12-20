#include "crslice2/gcode.h"
#include "libslic3r/GCodeWriter.hpp"
#include "libslic3r/Flow.hpp"

#include <sstream>

namespace crslice2
{
    double cal_e_per_mm(
        double line_width, double layer_height, float nozzle_diameter, float filament_diameter, float print_flow_ratio)
    {
        const Slic3r::Flow   line_flow = Slic3r::Flow(line_width, layer_height, nozzle_diameter);
        const double filament_area = M_PI * std::pow(filament_diameter / 2, 2);

        return line_flow.mm3_per_mm() / filament_area * print_flow_ratio;
    }


	std::string writeVarialLines(double start_width, int n, double delta)
	{
        Slic3r::GCodeWriter writer;
        std::vector<unsigned> extruder_ids;
        extruder_ids.push_back(1);
        writer.set_extruders(extruder_ids);
        writer.toolchange(1);

        Slic3r::FullPrintConfig config;
        config.initial_layer_travel_speed.value = 50.0;
        writer.apply_print_config(config);

        config.use_relative_e_distances.value = true;

        const double filament_diameter = config.filament_diameter.get_at(0);
        const double print_flow_ratio = config.print_flow_ratio;

        double height_layer = 0.2;
        double nozzle_diameter = config.nozzle_diameter.get_at(0);

        std::stringstream gcode;
        trimesh::dvec3 start_pos(40.0, 40.0, 0.2);

        gcode << writer.travel_to_z(start_pos.z);

        for (int i = 0; i < n; ++i)
        {
            double delta_x = 100.0;
            double delta_y = 10.0;
            double line_width = start_width + (double)i * delta;
            trimesh::dvec2 start_xy(start_pos.x, start_pos.y + (double)i * delta_y);
            trimesh::dvec2 end_xy = start_xy + trimesh::dvec2(delta_x, 0.0);

            gcode << writer.travel_to_xy(Slic3r::Vec2d(start_xy.x, start_xy.y));
            const double e_per_mm = cal_e_per_mm(line_width, height_layer, nozzle_diameter, filament_diameter,
                print_flow_ratio);

            gcode << writer.extrude_to_xy(Slic3r::Vec2d(end_xy.x, end_xy.y), e_per_mm * delta_x);
        }

		return gcode.str();
	}
}