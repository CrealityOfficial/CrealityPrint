#ifndef CRCOMMON_JSONLOADER_1669972280254_H
#define CRCOMMON_JSONLOADER_1669972280254_H
#include "crslice2/interface.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

namespace crslice2
{
	typedef std::unordered_map<std::string, std::string> KValues;
    /*
 * \brief Load a JSON file and store the settings inside it.
 * \param json_filename The location of the JSON file to load settings from.
 * \param settings The settings storage to store the settings in.
 * \return Error code. If it's 0, the file was successfully loaded. If it's
 * 1, the file could not be opened. If it's 2, there was a syntax error in
 * the file.
 */
    int loadJSON(const std::string& jsonFileName, KValues& KVs, std::vector<KValues>& extruderKVs);

    std::unordered_set<std::string> jsonSearchDirectories();

    /*
     * \brief Find a definition file in the search directories.
     * \param definition_id The ID of the definition to look for.
     * \param search_directories The directories to search in.
     * \return The first definition file that matches the definition ID.
     */
    std::string findDefinitionFile(const std::string& definition_id, const std::unordered_set<std::string>& search_directories);
}

#endif // CRCOMMON_JSONLOADER_1669972280254_H