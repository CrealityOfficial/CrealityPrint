#ifndef CRSLICE_HEADER_INTERFACE_2
#define CRSLICE_HEADER_INTERFACE_2
#include "crslice2/interface.h"
#include <memory>

#include "trimesh2/TriMesh.h"
#include "trimesh2/XForm.h"
#include "trimesh2/TriMesh_algo.h"

#include <fstream>
#include <unordered_map>
#include "ccglobal/tracer.h"

typedef std::shared_ptr<trimesh::TriMesh> TriMeshPtr;

#include "crslice2/settings.h"
namespace crslice2
{
    typedef std::shared_ptr<crslice2::Settings> SettingsPtr;

	enum class CalibMode : int {
		Calib_None = 0,
		Calib_PA_Line,
		Calib_PA_Pattern,
		Calib_PA_Tower,
		Calib_Flow_Rate,
		Calib_Temp_Tower,
		Calib_Vol_speed_Tower,
		Calib_VFA_Tower,
		Calib_Retraction_tower,
		Calib_Retraction_tower_speed,
		Calib_Limit_Speed,
		Calib_Limit_Acceleration,
		Calib_Speed_Tower,
		Calib_Acceleration_Tower,
		Calib_Arc2Lerance,
		Calib_Accel2Decel,
		Calib_X_Y_Jerk,
		Calib_Fan_Speed
	};

	struct Calib_Params
	{
		Calib_Params() {
			start = 0.0;
			end = 0.0;
			step = 0.0;
			highStep = 0.0;
			print_numbers = false;
			mode = CalibMode::Calib_None;
		}
		double start, end, step;
		double highStep;
		bool print_numbers = false;
		CalibMode mode;
	};

	enum Plate_Mode
	{
		Undef,
		SingleExtruder,
		MultiAsSingle,
		MultiExtruder
	};

	enum Plate_Type
	{
		ColorChange,
		PausePrint,
		ToolChange,
		Template,
		Custom,
		Unknown
	};

	struct Plate_Item
	{
		double      print_z;
		Plate_Type        type;
		int         extruder;
		std::string color;
		std::string extra;
	};

	struct ThumbnailData
	{
		unsigned int width;
		unsigned int height;
		unsigned int pos_s; //for cr_png
		unsigned int pos_e; //for cr_png
		unsigned int pos_h; //for cr_png
		std::vector<unsigned char> pixels;
	};

	struct PlateInfo
	{
		Plate_Mode mode = Undef;
		std::vector<Plate_Item> gcodes;
	};
}

#endif // CRSLICE_HEADER_INTERFACE_2