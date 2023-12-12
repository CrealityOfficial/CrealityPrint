#ifndef CCGLOBAL_SERIAL_H
#define CCGLOBAL_SERIAL_H
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <functional>

#include "ccglobal/log.h"
#include "ccglobal/tracer.h"

namespace ccglobal
{
	class Serializeable
	{
	public:
		virtual ~Serializeable() {}

		virtual int version() = 0;
		virtual bool save(std::fstream& out, ccglobal::Tracer* tracer) = 0;
		virtual bool load(std::fstream& in, int ver, ccglobal::Tracer* tracer) = 0;
	};

	template<class T>
	bool cxndSave(T& serializeable, const std::string& fileName, ccglobal::Tracer* tracer = nullptr)
	{
		std::fstream out(fileName, std::ios::out | std::ios::binary);
		if (!out.is_open())
		{
			LOGE("cxndSave error. [%s]", fileName.c_str());
			out.close();
			return false;
		}


		int ver = serializeable.version();
		out.write((const char*)&ver, sizeof(int));
		bool result = serializeable.save(out, tracer);

		out.close();
		return result;
	}
	
	template<class T>
	bool cxndLoad(T& serializeable, const std::string& fileName, ccglobal::Tracer* tracer = nullptr)
	{
		std::fstream in(fileName, std::ios::in | std::ios::binary);
		if (!in.is_open())
		{
			LOGE("cxndSave error. [%s]", fileName.c_str());
			return false;
		}

		int ver = -1;
		in.read((char*)&ver, sizeof(int));
		bool result = serializeable.load(in, ver, tracer);

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