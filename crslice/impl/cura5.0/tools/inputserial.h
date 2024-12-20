#ifndef CURA52_INPUTSERIAL_1679560834710_H
#define CURA52_INPUTSERIAL_1679560834710_H
#include "jsonhelper.h"
#include "types/Angle.h"
#include "utils/polygon.h"

#if _DEBUG
#define CURA_SERIAL_DATA 0   // set 0, when shipping
#else
#define CURA_SERIAL_DATA 0
#endif

namespace cura52
{
	void savePolygons(const Polygons& polygons, std::ofstream& out);
	void loadPolygons(Polygons& polygons, std::ifstream& in);

	void saveAngleRadians(const AngleRadians& radians, std::ofstream& out);
	void loadAngleRadians(AngleRadians& radians, std::ifstream& in);
	void saveRatio(const Ratio& ratio, std::ofstream& out);
	void loadRatio(Ratio& ratio, std::ifstream& in);

	class SkeletalTrapezoidationTester
	{
	public:
		SkeletalTrapezoidationTester();
		~SkeletalTrapezoidationTester();

		void load(const std::string& fileName);
		void save(const std::string& fileName);
		void test();

	public:
		coord_t allowed_distance = 0;
		AngleRadians transitioning_angle = 0;
		coord_t discretization_step_size = MM2INT(0.8);

		coord_t offset_insert = 0;
		double scaled_spacing_wall_0 = 0.0;
		double scaled_spacing_wall_X = 0.0;

		coord_t wall_transition_length = 0;
		double min_even_wall_line_width = 0.0;
		double wall_line_width_0 = 0.0;
		Ratio wall_split_middle_threshold = 0.0;

		double min_odd_wall_line_width = 0.0;
		double wall_line_width_x = 0.0;
		Ratio wall_add_middle_threshold = 0.0;

		int wall_distribution_count = 0;
		size_t max_bead_count = 0;

		coord_t transition_filter_dist = 0;
		coord_t allowed_filter_deviation = 0;

		bool print_thin_walls = false;
		coord_t min_feature_size = 0;
		coord_t min_bead_width = 0;
		coord_t wall_0_inset = 0;

		Polygons prepared_outline;
	};
}

#endif // CURA52_INPUTSERIAL_1679560834710_H