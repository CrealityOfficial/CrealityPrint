#ifndef CRCOMMON_PARAMETERMETA_1690769853657_H_2
#define CRCOMMON_PARAMETERMETA_1690769853657_H_2
#include "crslice2/interface.h"

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

namespace crslice2
{
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

		std::vector<std::pair<std::string, std::string> > options;
	};

	typedef std::unordered_map<std::string, std::string>::value_type OptionValue;
	typedef std::unordered_map<std::string, ParameterMeta*> MetasMap;
	typedef MetasMap::iterator MetasMapIter;

	class CRSLICE2_API ParameterMetas
	{
	public:
		ParameterMetas();
		~ParameterMetas();

		ParameterMeta* find(const std::string& key);

		// used for base
		void initializeBase(const std::string& path);
		ParameterMetas* createInherits(const std::string& fileName);
	protected:
		void clear();
		ParameterMetas* copy();
	public:
		MetasMap mm;
	};

	CRSLICE2_API void saveKeysJson(const std::vector<std::string>& keys, const std::string& fileName);

	CRSLICE2_API void parseMetasMap(MetasMap& datas);   //from binary

	enum class MetaGroup {
		emg_machine,
		emg_extruder,
		emg_material,
		emg_profile
	};

	CRSLICE2_API void getMetaKeys(MetaGroup metaGroup, std::vector<std::string>& keys);

	CRSLICE2_API std::string engineVersion();

	CRSLICE2_API void exportParameterMeta();
}

#endif // CRCOMMON_PARAMETERMETA_1690769853657_H
