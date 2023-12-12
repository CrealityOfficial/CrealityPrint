#ifndef CXKERNEL_POLYGON_TRIMESHSERIAL_1691656350835_H
#define CXKERNEL_POLYGON_TRIMESHSERIAL_1691656350835_H
#include "topomesh/interface/idata.h"
#include "ccglobal/serial.h"

namespace topomesh
{
	class TOPOMESH_API PolygonsWrapper : public ccglobal::Serializeable
	{
	public:
		PolygonsWrapper();
		~PolygonsWrapper();

		TriPolygons polys;

		int version() override;
		bool save(std::fstream& out, ccglobal::Tracer* tracer) override;
		bool load(std::fstream& in, int ver, ccglobal::Tracer* tracer) override;
	};

	TOPOMESH_API void loadPolys(std::fstream& in, TriPolygons& polys);
	TOPOMESH_API void savePolys(std::fstream& out, const TriPolygons& polys);

	class TOPOMESH_API PolygonSerial : public ccglobal::Serializeable
	{
	public:
		PolygonSerial();
		virtual ~PolygonSerial();

		TriPolygon poly;

		int version() override;
		bool save(std::fstream& out, ccglobal::Tracer* tracer) override;
		bool load(std::fstream& in, int ver, ccglobal::Tracer* tracer) override;
	};
}

#endif // CXKERNEL_POLYGON_TRIMESHSERIAL_1691656350835_H