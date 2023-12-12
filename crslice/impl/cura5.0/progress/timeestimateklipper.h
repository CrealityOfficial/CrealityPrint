#ifndef CURA52_TIMEESTIMATEKLIPPER_1685609041425_H
#define CURA52_TIMEESTIMATEKLIPPER_1685609041425_H
#include "progress/timeEstimate.h"

namespace cura52
{
	//class Ratio;
	//class Settings;
	class TimeEstimateKlipper : public TimeEstimateCalculator
	{
	public:
		TimeEstimateKlipper(); 
		virtual ~TimeEstimateKlipper();

		void plan(Position newPos, Velocity feedRate, PrintFeatureType feature) override;
		std::vector<Duration> calculate() override;

		class Move
		{
		public:
			double delta_v2;
			double max_start_v2;
			double smooth_delta_v2;
			double max_smoothed_v2;
			double max_cruise_v2;
			double accel;
			double start_v;
			double cruise_v;
			double end_v;
			double accel_t;
			double cruise_t;
			double decel_t;
			Velocity theF;
			double junction_deviation;
			double max_velocity;
			double axes_d[NUM_AXIS];
			double move_d;
			bool   is_kinematic_move;
			double axes_r[NUM_AXIS];
			double min_move_t;
			double accel_d;
			double decel_d;
			double cruise_d;
		};

		Acceleration max_accel = 20000.0;
		//std::vector<Move> blocksflush;
		double junction_flush = 2.0;
		bool update_flush_count = false;

		//void setAccel(const Acceleration& acc);

		void reset();

		struct MyStruct
		{
			Block myblock;
			Move mymove;
			double mystart;
			double myend;
			MyStruct(Block a, Move d, double b, double c)
			{
				myblock = a;
				mymove = d;
				mystart = b;
				myend = c;
			}
		};

		std::vector<Move> moves;

	//private:
	//	Velocity max_feedrate[NUM_AXIS] = { 600, 600, 40, 25 }; 
	//	Velocity minimumfeedrate  = 0.01;
	//	Acceleration acceleration = 3000;
	//	Acceleration max_acceleration[NUM_AXIS] = { 9000, 9000, 100, 10000 };   
	//	Velocity max_xy_jerk = 20.0;
	//	Velocity max_z_jerk  = 0.4;
	//	Velocity max_e_jerk  = 5.0;
	//	Duration extra_time  = 0.0;
	//	Position previous_feedrate;
	//	Velocity previous_nominal_feedrate;
	//	Position currentPosition;
	//	std::vector<Block> blocks;

	//private:
	//	void reversePass();
	//	void forwardPass();

	//	// Recalculates the trapezoid speed profiles for all blocks in the plan according to the
	//	// entry_factor for each junction. Must be called by planner_recalculate() after
	//	// updating the blocks.
	//	void recalculateTrapezoids();

	//	// Calculates trapezoid parameters so that the entry- and exit-speed is compensated by the provided factors.
	//	void calculateTrapezoidForBlock(Move* block, const Ratio entry_factor, const Ratio exit_factor);

	//	// The kernel called by accelerationPlanner::calculate() when scanning the plan from last to first entry.
	//	void plannerReversePassKernel(Move* previous, Move* current, Move* next);

	//	// The kernel called by accelerationPlanner::calculate() when scanning the plan from first to last entry.
	//	void plannerForwardPassKernel(Move* previous, Move* current, Move* next);
	
	};
}

#endif // CURA52_TIMEESTIMATEKLIPPER_1685609041425_H