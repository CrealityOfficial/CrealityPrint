#ifndef CCGLOBAL_SERIAL_H
#define CCGLOBAL_SERIAL_H
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <functional>

namespace ccglobal
{
	typedef std::function<bool(std::fstream& fs, int version)> serial_func;
	inline bool cxndSave(const std::string& fileName, int version, serial_func func)
	{
		std::fstream out(fileName, std::ios::out | std::ios::binary);
		if (!out.is_open())
		{
			out.close();
			return false;
		}


		int ver = version;
		out.write((const char*)&ver, sizeof(int));
		bool result = false;
		if (func)
			result = func(out, ver);

		out.close();
		return result;
	}

	inline bool cxndLoad(const std::string& fileName, serial_func func)
	{
		std::fstream in(fileName, std::ios::in | std::ios::binary);
		if (!in.is_open())
		{
			in.close();
			return false;
		}

		int ver = -1;
		in.read((char*)&ver, sizeof(int));
		bool result = false;
		if (func)
			result = func(in, ver);

		in.close();
		return result;
	}

	template<class T>
	void cxndLoadT(std::fstream& in, T& t)
	{
		in.read((char*)&t, sizeof(T));
	}

	template<class T>
	void cxndSaveT(std::fstream& out, const T& t)
	{
		out.write((const char*)&t, sizeof(T));
	}

	template<class T>
	void cxndLoadVectorT(std::fstream& in, std::vector<T>& vecs)
	{
		int num = 0;
		cxndLoadT(in, num);
		if (num > 0)
		{
			vecs.resize(num);
			in.read((char*)&vecs.at(0), num * sizeof(T));
		}
	}

	template<class T>
	void cxndSaveVectorT(std::fstream& out, const std::vector<T>& vecs)
	{
		int num = (int)vecs.size();
		cxndSaveT(out, num);
		if (num > 0)
			out.write((const char*)&vecs.at(0), num * sizeof(T));
	}

	inline void cxndLoadStr(std::fstream& in, std::string& str)
	{
		int size = 0;
		cxndLoadT(in, size);
		if (size > 0)
		{
			str.resize(size);
			in.read((char*)str.data(), size);
		}
	}

	inline void cxndSaveStr(std::fstream& out, const std::string& str)
	{
		int size = (int)str.size();
		cxndSaveT(out, size);
		if(size > 0)
			out.write(str.c_str(), size);
	}

	inline void cxndLoadStrs(std::fstream& in, std::vector<std::string>& strs)
	{
		int size = 0;
		cxndLoadT(in, size);
		if (size > 0)
		{
			strs.resize(size);
			for (int i = 0; i < size; ++i)
				cxndLoadStr(in, strs.at(i));
		}
	}

	inline void cxndSaveStrs(std::fstream& out, const std::vector<std::string>& strs)
	{
		int size = (int)strs.size();
		cxndSaveT(out, size);
		for (int i = 0; i < size; ++i)
			cxndSaveStr(out, strs.at(i));
	}
}

#define cxndLoadComplexVectorT(in, vecs) \
{ \
	int num = 0; \
	ccglobal::cxndLoadT(in, num);  \
	if (num > 0) \
	{ \
		vecs.resize(num); \
		for (int i = 0; i < num; ++i) \
			_load(in, vecs.at(i)); \
	} \
}

#define cxndSaveComplexVectorT(out, vecs) \
{ \
	int num = (int)vecs.size(); \
	ccglobal::cxndSaveT(out, num); \
	if (num > 0) \
	{ \
		for (int i = 0; i < num; ++i) \
			_save(out, vecs.at(i)); \
	} \
}

#endif // CCGLOBAL_SERIAL_H