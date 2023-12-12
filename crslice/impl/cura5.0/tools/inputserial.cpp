#include "inputserial.h"
#include "skeletal/SkeletalTrapezoidation.h"
#include "skeletal/BeadingStrategyFactory.h"

using namespace crslice;
namespace cura52
{
    void savePolygons(const Polygons& polygons, std::ofstream& out)
    {
        int size = (int)polygons.paths.size();
        templateSave<int>(size, out);
        for (int i = 0; i < size; ++i)
            templateSave(polygons.paths.at(i), out);
    }

    void loadPolygons(Polygons& polygons, std::ifstream& in)
    {
        int size = templateLoad<int>(in);
        if (size > 0)
        {
            polygons.paths.resize(size);
            for (int i = 0; i < size; ++i)
                templateLoad(polygons.paths.at(i), in);
        }
    }

    void saveAngleRadians(const AngleRadians& radians, std::ofstream& out)
    {
        double v = radians;
        templateSave<double>(v, out);
    }

    void loadAngleRadians(AngleRadians& radians, std::ifstream& in)
    {
        double v = templateLoad<double>(in);
        radians = v;
    }

    void saveRatio(const Ratio& ratio, std::ofstream& out)
    {
        double v = ratio;
        templateSave<double>(v, out);
    }

    void loadRatio(Ratio& ratio, std::ifstream& in)
    {
        double v = templateLoad<double>(in);
        ratio = v;
    }

	SkeletalTrapezoidationTester::SkeletalTrapezoidationTester()
	{

	}

	SkeletalTrapezoidationTester::~SkeletalTrapezoidationTester()
	{

	}

	void SkeletalTrapezoidationTester::load(const std::string& fileName)
	{
        std::ifstream in;
        in.open(fileName, std::ios_base::binary);
        if (in.is_open())
        {
            allowed_distance = templateLoad<coord_t>(in);
            loadAngleRadians(transitioning_angle, in);
            discretization_step_size = templateLoad<coord_t>(in);

            offset_insert = templateLoad<coord_t>(in);
            scaled_spacing_wall_0 = templateLoad<double>(in);
            scaled_spacing_wall_X = templateLoad<double>(in);

            wall_transition_length = templateLoad<coord_t>(in);
            min_even_wall_line_width = templateLoad<double>(in);
            wall_line_width_0 = templateLoad<double>(in);
            loadRatio(wall_split_middle_threshold, in);

            min_odd_wall_line_width = templateLoad<double>(in);
            wall_line_width_x = templateLoad<double>(in);
            loadRatio(wall_add_middle_threshold, in);

            wall_distribution_count = templateLoad<int>(in);
            max_bead_count = templateLoad<size_t>(in);

            transition_filter_dist = templateLoad<coord_t>(in);
            allowed_filter_deviation = templateLoad<coord_t>(in);

            print_thin_walls = templateLoad<bool>(in);
            min_feature_size = templateLoad<coord_t>(in);
            min_bead_width = templateLoad<coord_t>(in);
            wall_0_inset = templateLoad<coord_t>(in);

            loadPolygons(prepared_outline, in);
        }
	}

	void SkeletalTrapezoidationTester::save(const std::string& fileName)
	{
        std::ofstream out;
        out.open(fileName, std::ios_base::binary);
        if (out.is_open())
        {
            templateSave(allowed_distance, out);
            saveAngleRadians(transitioning_angle, out);
            templateSave(discretization_step_size, out);
            templateSave(offset_insert, out);
            templateSave(scaled_spacing_wall_0, out);
            templateSave(scaled_spacing_wall_X, out);
            templateSave(wall_transition_length, out);
            templateSave(min_even_wall_line_width, out);
            templateSave(wall_line_width_0, out);
            saveRatio(wall_split_middle_threshold, out);
            templateSave(min_odd_wall_line_width, out);
            templateSave(wall_line_width_x, out);
            saveRatio(wall_add_middle_threshold, out);
            templateSave(wall_distribution_count, out);
            templateSave(max_bead_count, out);
            templateSave(transition_filter_dist, out);
            templateSave(allowed_filter_deviation, out);
            templateSave(print_thin_walls, out);
            templateSave(min_feature_size, out);
            templateSave(min_bead_width, out);
            templateSave(wall_0_inset, out);

            savePolygons(prepared_outline, out);
        }
        out.close();
	}

	void SkeletalTrapezoidationTester::test()
	{
        const auto beading_strat = BeadingStrategyFactory::makeStrategy
        (
            scaled_spacing_wall_0,
            scaled_spacing_wall_X,
            wall_transition_length,
            transitioning_angle,
            print_thin_walls,
            min_bead_width,
            min_feature_size,
            wall_split_middle_threshold,
            wall_add_middle_threshold,
            max_bead_count,
            wall_0_inset,
            wall_distribution_count
        );

        SkeletalTrapezoidation wall_maker
        (
            prepared_outline,
            *beading_strat,
            beading_strat->getTransitioningAngle(),
            discretization_step_size,
            transition_filter_dist,
            allowed_filter_deviation,
            wall_transition_length
        );

        std::vector<VariableWidthLines> toolpaths;
        wall_maker.generateToolpaths(toolpaths);
	}
}