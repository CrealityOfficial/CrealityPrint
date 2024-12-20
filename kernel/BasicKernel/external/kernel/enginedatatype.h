#ifndef CREATIVE_KERNEL_ENGINEDATATYPE_
#define CREATIVE_KERNEL_ENGINEDATATYPE_

#include <string>
#include <unordered_map>

namespace creative_kernel
{
	enum class EngineType : unsigned char
	{
		ET_CURA,
		ET_ORCA
	};

	struct ParameterMeta
	{
		std::string name;
		std::string label;
		std::string description;
		std::string type;
		std::string default_value;
		std::string value;
		std::string enabled;

		std::string unit;
		std::string minimum_value;
		std::string maximum_value;
		std::string minimum_value_warning;
		std::string maximum_value_warning;

		std::string settable_globally;
		std::string settable_per_extruder;
		std::string settable_per_mesh;
		std::string settable_per_meshgroup;

		std::unordered_map<std::string, std::string> options;
	};

	enum class MetaGroup {
		emg_machine,
		emg_extruder,
		emg_material,
		emg_profile
	};

	using OptionValue = std::unordered_map<std::string, std::string>::value_type;
	using MetasMap = std::unordered_map<std::string, ParameterMeta*>;
}
#endif
