#include "crslice/gcode/sliceresult.h"
#include "crslice/gcode/parasegcode.h"
#include "gcodeprocesslib/gcode_position.h"
#include "gcodeprocesslib/gcode_parser.h"

#define DEFAULT_GCODE_BUFFER_SIZE 50
#define DEFAULT_G90_G91_INFLUENCES_EXTRUDER false

namespace gcode
{
    SliceResult::SliceResult() 
    {

    }

    SliceResult::~SliceResult()
    {
    }

	gcode_position_args gets_args_(bool g90_g91_influences_extruder, int buffer_size)
	{
		gcode_position_args args;
		// Configure gcode_position_args
		args.g90_influences_extruder = g90_g91_influences_extruder;
		args.position_buffer_size = buffer_size;
		args.autodetect_position = true;
		args.home_x = 0;
		args.home_x_none = true;
		args.home_y = 0;
		args.home_y_none = true;
		args.home_z = 0;
		args.home_z_none = true;
		args.shared_extruder = true;
		args.zero_based_extruder = true;


		args.default_extruder = 0;
		args.xyz_axis_default_mode = "absolute";
		args.e_axis_default_mode = "absolute";
		args.units_default = "millimeters";
		args.location_detection_commands = std::vector<std::string>();
		args.is_bound_ = false;
		args.is_circular_bed = false;
		args.x_min = -9999;
		args.x_max = 9999;
		args.y_min = -9999;
		args.y_max = 9999;
		args.z_min = -9999;
		args.z_max = 9999;
		return args;
	}

    const std::string& SliceResult::prefixCode()
    {
        if (m_data_gcodeprefix.size() > 0)
            return *m_data_gcodeprefix.begin();
        return m_emptyString;
    }

    const std::vector<std::string>& SliceResult::layerCode()
    {
        return m_data_gcodelayer;
    }

    const std::string& SliceResult::layer(int index)
    {
        if (index < 0 || index >= (int)m_data_gcodelayer.size())
            return m_emptyString;

        return m_data_gcodelayer.at(index);
    }

    const std::string& SliceResult::tailCode()
    {
        if (m_data_gcodetail.size() > 0)
            return *(m_data_gcodetail.begin());
        return m_emptyString;
    }

    void SliceResult::clear()
    {
        m_data_gcodelayer.clear();
        m_data_gcodeprefix.clear();
    }

	unsigned long long get_file_size(std::ifstream& fin)
	{
		fin.seekg(0, fin.end);
		auto fos = fin.tellg();
		unsigned long long filesize = fos;
		fin.seekg(0);
		return filesize;
	}

	bool SliceResult::load(const std::string& fileName, ccglobal::Tracer* tracer)
    {
		m_fileName = fileName;
		m_data_gcodelayer.clear();
		m_data_gcodeprefix.clear();

		std::ifstream  fileInfo(fileName);
		if (!fileInfo.is_open())
		{
			return false;
		}

		unsigned long long fileSize = get_file_size(fileInfo);
		std::string value = "";
		bool headend = false;
		unsigned long long int readBytes = 0;
		std::string temp;
		
		while (std::getline(fileInfo, temp))
		{			
			value += (temp + "\n");
			readBytes += temp.size();
			if (tracer && (readBytes % 20000) == 0)
			{
				float p = (float)((double)readBytes / (double)fileSize);
				tracer->progress(p);

				if (tracer->interrupt())
				{
					tracer->failed("cxLoadGCode load interrupt ");
					m_data_gcodelayer.clear();
					break;
				}
			}

			if (temp.find(";LAYER_COUNT")!= std::string::npos)
			{
				if (!headend)
				{
					m_data_gcodeprefix.push_back(value);
				}
				else
				{
					//针对逐个打印 增加抬升代码
					std::string str = m_data_gcodelayer.back();
					str += value;
					m_data_gcodelayer.back() = str;
				}

				//layerString.append(temp);
				headend = true;
				value.clear();
				continue;
			}
			if (!headend)
			{
				continue;
			}
			if (temp.find(";TIME_ELAPSED:") != std::string::npos)
			{
				m_data_gcodelayer.push_back(value);
				value.clear();
				continue;
			}
		}
		m_data_gcodetail.push_back(value);
		fileInfo.close();
		return true;
    }
	bool SliceResult::load_pressureEquity(const std::string& fileName, ccglobal::Tracer* tracer)
	{
		m_fileName = fileName;
		m_data_gcodelayer.clear();
		m_data_gcodeprefix.clear();

		std::ifstream  fileInfo(fileName);
		if (!fileInfo.is_open())
		{
			return false;
		}

		unsigned long long fileSize = get_file_size(fileInfo);
		std::string value = "";
		bool headend = false;
		unsigned long long int readBytes = 0;
		std::string temp;
		gcode_parser parser;
		int gcodes_processed = 0;
		int num_arc_commands_ = 0;
		int lines_processed_ = 0;
		bool is_shell = true;
		gcode_position* p_source_position_ = new gcode_position(gets_args_(DEFAULT_G90_G91_INFLUENCES_EXTRUDER, DEFAULT_GCODE_BUFFER_SIZE));
		int lines_with_no_commands = 0;
		double velocity = 0;
		float rate_previous = 0;
		float rate_previous_tmp = 0;
		float feed_rate_previous = 0;
		float feed_rate = 0;
		float distance_move = 0;
		float rate = 0;
		float e_gcode = 0;
		trimesh::vec3 previous_position;
		trimesh::vec3 current_position;

		while (std::getline(fileInfo, temp))
		{
			parsed_command cmd;
			lines_processed_++;
			bool isLayer = false;
			cmd.clear();
			parser.try_parse_gcode(temp.c_str(), cmd);
			bool has_gcode = false;
			if (cmd.gcode.length() > 0)
			{
				has_gcode = true;
				gcodes_processed++;
			}
			else
			{
				lines_with_no_commands++;
			}
			p_source_position_->update(cmd, lines_processed_, gcodes_processed, -1);
			gcode_comment_processor* gcode_comment_processor = p_source_position_->get_gcode_comment_processor();
			extruder* ex = nullptr;
			if (cmd.command == "G1" && 1)
			{
				// increment the number of arc commands encountered
				num_arc_commands_++;
				// Get the current and previous positions


				previous_position = trimesh::vec3(p_source_position_->get_previous_position_ptr()->x, p_source_position_->get_previous_position_ptr()->y,
					p_source_position_->get_previous_position_ptr()->z);
				current_position = trimesh::vec3(p_source_position_->get_current_position_ptr()->x, p_source_position_->get_current_position_ptr()->y,
					p_source_position_->get_current_position_ptr()->z);
				trimesh::vec3 diff = previous_position - current_position;
				position p = p_source_position_->get_current_position();
				ex = p.p_extruders;
				rate = ex->e_relative;
				//rate_previous_tmp = ex->e;
				e_gcode = ex->extrusion_length;
				distance_move = diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2];
			}
			std::vector<std::string> find_E;
			Stringsplit(temp, 'E', find_E);
			std::string temp_E = "";

			if (find_E.size() > 1)
			{
				if (!find_E.empty())
				{
					for (int i = 0; i < find_E.at(1).size(); i++)
					{
						temp_E.push_back(find_E.at(1).at(i));
					}

				}
			}
			if (std::atof(temp_E.c_str()) > 0)
				rate_previous_tmp = std::atof(temp_E.c_str());
			std::vector<std::string> find_Velocity;
			Stringsplit(temp, 'F', find_Velocity);
			std::string temp_Velocity = "";

			if (find_Velocity.size() > 1 && (find_Velocity.at(0) == "G0" || find_Velocity.at(0) == "G1"))
			{
				std::vector<std::string> find_Velocity_F;
				Stringsplit(find_Velocity.at(1), ' ', find_Velocity_F);
				if (!find_Velocity_F.empty())
				{
					for (int i = 0; i < find_Velocity_F.at(0).size(); i++)
					{
						temp_Velocity.push_back(find_Velocity_F.at(0).at(i));
					}

				}
			}
			float V_now = velocity;
			float f_temp_Velocity = atof(temp_Velocity.c_str());
			if (f_temp_Velocity > 0. && f_temp_Velocity<10000.0)
				V_now = f_temp_Velocity;
			feed_rate = 2.40556 * V_now * sqrt((rate * rate) / distance_move);   //distance short   rate large
			if (velocity > 0 && V_now / velocity > 5 && e_gcode < 0.2 && e_gcode > 0.08 && distance_move > 8)
			//if (velocity > 0 && V_now / velocity > 5 && distance_move > 3)
			{
				// volumetric extrusion rate = A_filament * F_xyz * L_e / L_xyz [mm^3/min
				if (feed_rate > 800 && (feed_rate / feed_rate_previous) > 5.0)
				{
					float interX = previous_position.x + abs(current_position.x - previous_position.x) / 4.;
					//y1 + (x - x1) * (y2 - y1) / (x2 - x1);
					float K_slope = 1;
					if (current_position.x != previous_position.x)
						K_slope = (current_position.y - previous_position.y) / (current_position.x - previous_position.x);
					float interY = previous_position.y + (interX - previous_position.x) * K_slope;
					std::string inerp_V_E = "G1 F" + std::to_string(int(V_now));

					inerp_V_E += " X";
					inerp_V_E += std::to_string(interX);
					inerp_V_E += " Y";
					inerp_V_E += std::to_string(interY);
					//inerp_V_E += " E1.3400\n"; //special S12000
					inerp_V_E += " E";
					float rata_increment = 1;
					//if (K_slope!=0 && K_slope!= FLT_MAX && K_slope != FLT_MIN)
					//	rata_increment = K_slope * (rate_previous_tmp - rate_previous);
					//else
					rata_increment = rate_previous + (rate_previous_tmp - rate_previous) / 4.0;
					if (rata_increment - rate_previous > 5)
						int sss = 100;
					inerp_V_E += std::to_string(rata_increment);

					//if (lines_processed_ > 43412 && lines_processed_ < 43835)
					//	std::cout << rata_increment << " ";
					//inerp_V_E += std::to_string(rate_previous + 0.02);

					inerp_V_E += "\n";
					//value += inerp_V_E;
					//continue;
					//speed_long_jump = true;
				}

			}

			std::vector<std::string> oneLS;
			Stringsplit(temp, ':', oneLS);
			if (oneLS.size() == 2 && oneLS.at(0) == ";TYPE")
			{
				if (oneLS.at(1) == "WALL-INNER" || oneLS.at(1) == "WALL-OUTER")
				{
					value += (";_EXTRUSION_ROLE:1\n");
				}
				if (oneLS.at(1) == "SKIN")
				{
					value += (";_EXTRUSION_ROLE:2\n");
				}
				if (oneLS.at(1) == "FILL")
				{
					value += (";_EXTRUSION_ROLE:3\n");
				}
				if (oneLS.at(1) == "SKIRT")
				{
					value += (";_EXTRUSION_ROLE:4\n");
				}
				if (oneLS.at(1) == "SUPPORT-INTERFACE" || oneLS.at(1) == "SUPPORT")
				{
					value += (";_EXTRUSION_ROLE:5\n");
				}
				if (oneLS.at(1) == "SUPPORT-INTERFACE" || oneLS.at(1) == "SUPPORT")
				{
					value += (";_EXTRUSION_ROLE:5\n");
				}
			}

			if (temp == ";;retract;;" || temp == ";;retract move;;")
			{
				value += (";_EXTRUSION_ROLE:0\n");
			}

			value += (temp + "\n");
			readBytes += temp.size();
			if (tracer && (readBytes % 20000) == 0)
			{
				float p = (float)((double)readBytes / (double)fileSize);
				tracer->progress(p);

				if (tracer->interrupt())
				{
					tracer->failed("cxLoadGCode load interrupt ");
					m_data_gcodelayer.clear();
					break;
				}
			}

			if (temp.find(";LAYER_COUNT") != std::string::npos)
			{
				if (!headend)
				{
					m_data_gcodeprefix.push_back(value);
				}
				else
				{
					//针对逐个打印 增加抬升代码
					std::string str = m_data_gcodelayer.back();
					str += value;
					m_data_gcodelayer.back() = str;
				}

				//layerString.append(temp);
				headend = true;
				value.clear();
				continue;
			}
			if (!headend)
			{
				continue;
			}
			if (temp.find(";TIME_ELAPSED:") != std::string::npos)
			{
				m_data_gcodelayer.push_back(value);
				value.clear();
				continue;
			}
			if (!temp_Velocity.empty())
				velocity = V_now;
			rate_previous = rate_previous_tmp;
			feed_rate_previous = feed_rate;
		}
		m_data_gcodetail.push_back(value);
		fileInfo.close();
		return true;
	}
	void SliceResult::setFileName(const std::string& fileName)
	{
		m_fileName = fileName;
	}

	bool SliceResult::set_data_gcodelayer(int layer, const std::string& gcodelayer)//set a layer data
	{
		int count = layer - m_data_gcodelayer.size();
		if (count >= 0)
		{
			for (int i = 0; i < count+1; i++)
			{
				m_data_gcodelayer.push_back("");
			}
		}

		m_data_gcodelayer[layer] = gcodelayer;
		return true;
	}

	bool SliceResult::set_gcodeprefix(const std::string& gcodeprefix)//set gcodeprefix
	{
		if (m_data_gcodeprefix.empty())
		{
			m_data_gcodeprefix.push_back(gcodeprefix);
		}
		else
			m_data_gcodeprefix[0] = gcodeprefix;
		return true;
	}


	std::string SliceResult::fileName()
    {
        return m_fileName;
    }

	std::string SliceResult::sliceName()
    {
        return m_sliceName;
    }

    void SliceResult::setSliceName(const std::string& _sliceName)
    {
        m_sliceName = _sliceName;
    }
}
