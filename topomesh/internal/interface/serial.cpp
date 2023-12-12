#include "topomesh/interface/serial.h"

namespace topomesh
{
	using namespace ccglobal;
	void loadPolys(std::fstream& in, TriPolygons& polys)
	{
		int size = 0;
		cxndLoadT(in, size);
		if (size > 0)
		{
			polys.resize(size);
			for (int i = 0; i < size; ++i)
				cxndLoadVectorT(in, polys.at(i));
		}
	}

	void savePolys(std::fstream& out, const TriPolygons& polys)
	{
		int size = (int)polys.size();
		cxndSaveT(out, size);
		for (int i = 0; i < size; ++i)
			cxndSaveVectorT(out, polys.at(i));
	}

	PolygonsWrapper::PolygonsWrapper()
	{

	}

	PolygonsWrapper::~PolygonsWrapper()
	{

	}

	int PolygonsWrapper::version()
	{
		return 0;
	}

	bool PolygonsWrapper::save(std::fstream& out, ccglobal::Tracer* tracer)
	{
		savePolys(out, polys);
		return true;
	}

	bool PolygonsWrapper::load(std::fstream& in, int ver, ccglobal::Tracer* tracer)
	{
		if (ver == 0)
		{
			loadPolys(in, polys);
			return true;
		}
		return false;
	}

	PolygonSerial::PolygonSerial()
	{

	}

	PolygonSerial::~PolygonSerial()
	{

	}

	int PolygonSerial::version()
	{
		return 0;
	}

	bool PolygonSerial::save(std::fstream& out, ccglobal::Tracer* tracer)
	{
		cxndSaveVectorT(out, poly);
		return true;
	}

	bool PolygonSerial::load(std::fstream& in, int ver, ccglobal::Tracer* tracer)
	{
		if (ver == 0)
		{
			cxndLoadVectorT(in, poly);
			return true;
		}
		return false;
	}
}