#ifndef MSBASE_TRIMESHSERIAL_1691656350835_H
#define MSBASE_TRIMESHSERIAL_1691656350835_H
#include "msbase/interface.h"
#include "ccglobal/serial.h"
#include "trimesh2/TriMesh.h"

namespace msbase
{
	typedef std::vector<trimesh::vec3> CXNDPolygon;
	typedef std::vector<CXNDPolygon> CXNDPolygons;

	MSBASE_API trimesh::TriMesh* loadTrimesh(std::fstream& in);
	MSBASE_API void saveTrimesh(std::fstream& out, trimesh::TriMesh* mesh);

	MSBASE_API bool loadTrimesh(std::fstream& in, trimesh::TriMesh& mesh);
	MSBASE_API void saveTrimesh(std::fstream& out, const trimesh::TriMesh& mesh);

	class MSBASE_API CXNDPolygonsWrapper : public ccglobal::Serializeable
	{
	public:
		CXNDPolygonsWrapper();
		~CXNDPolygonsWrapper();

		CXNDPolygons polys;

		int version() override;
		bool save(std::fstream& out, ccglobal::Tracer* tracer) override;
		bool load(std::fstream& in, int ver, ccglobal::Tracer* tracer) override;
	};

	MSBASE_API void loadPolys(std::fstream& in, CXNDPolygons& polys);
	MSBASE_API void savePolys(std::fstream& out, const CXNDPolygons& polys);
}

#endif // MSBASE_TRIMESHSERIAL_1691656350835_H