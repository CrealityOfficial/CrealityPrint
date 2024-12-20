#ifndef CRCOMMON_PARAMETERGENERATOR_1690769853658_H
#define CRCOMMON_PARAMETERGENERATOR_1690769853658_H
#include <rapidjson/document.h>
#include "crslice/base/parametermeta.h"
#include <string>
#include <fstream>

namespace crslice
{
	bool openJson(rapidjson::Document& doc, const std::string& fileName);

	void processSub(const rapidjson::Document& doc, MetasMap& metas);
	void processMeta(const rapidjson::Value& value, ParameterMeta& meta);
	void processKeys(const rapidjson::Document& doc, std::vector<std::string>& keys);

	void processInherit(const std::string& fileName, const std::string& directory, ParameterMetas& metas);

	void saveJson(const std::string& fileName, const std::string& content);
	std::string createKeysContent(const std::vector<std::string>& keys);

	template<class T>
	void templateSave(const T& t, std::ofstream& out)
	{
		out.write((const char*)&t, sizeof(T));
	}

	template<class T>
	T templateLoad(std::ifstream& in)
	{
		T t;
		in.read((char*)&t, sizeof(T));
		return t;
	}

	template<class T>
	void templateSave(const std::vector<T>& ts, std::ofstream& out)
	{
		int size = (int)ts.size();
		templateSave<int>(size, out);
		out.write((const char*)ts.data(), size * sizeof(T));
	}

	template<class T>
	void templateLoad(std::vector<T>& ts, std::ifstream& in)
	{
		int size = templateLoad<int>(in);
		if (size > 0)
		{
			ts.resize(size);
			in.read((char*)ts.data(), size * sizeof(T));
		}
	}
}

#endif // CRCOMMON_PARAMETERGENERATOR_1690769853658_H