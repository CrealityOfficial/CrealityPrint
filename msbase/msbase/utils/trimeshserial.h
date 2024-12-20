#ifndef MSBASE_TRIMESHSERIAL_1691656350835_H
#define MSBASE_TRIMESHSERIAL_1691656350835_H
#include "msbase/interface.h"
#include "ccglobal/serial.h"
#include "trimesh2/TriMesh.h"

namespace msbase
{
	typedef std::vector<trimesh::vec3> CXNDPolygon;
	typedef std::vector<CXNDPolygon> CXNDPolygons;

    typedef std::vector<int> CXNDFacePatch;
    typedef std::vector<CXNDFacePatch> CXNDFacePatchs;
    
    struct CXNDGeometry {
        CXNDPolygon contour;
        CXNDPolygons holes;
        CXNDGeometry() {}
        CXNDGeometry(const CXNDPolygon& con) :contour(con) {}
        CXNDGeometry(const CXNDPolygon& con, const CXNDPolygons ho) :contour(con), holes(ho) {}
        ~CXNDGeometry()
        {
            contour.clear();
            holes.clear();
        }
    };
    typedef std::vector<CXNDGeometry> CXNDGeometrys;

	MSBASE_API trimesh::TriMesh* loadTrimesh(std::fstream& in);
	MSBASE_API void saveTrimesh(std::fstream& out, trimesh::TriMesh* mesh);

	MSBASE_API bool loadTrimesh(std::fstream& in, trimesh::TriMesh& mesh);
	MSBASE_API void saveTrimesh(std::fstream& out, const trimesh::TriMesh& mesh);

    class MSBASE_API CXNDPolygonWrapper : public ccglobal::Serializeable {
    public:
        CXNDPolygonWrapper();
        ~CXNDPolygonWrapper();

        CXNDPolygon poly;

        int version() override;
        bool save(std::fstream& out, ccglobal::Tracer* tracer) override;
        bool load(std::fstream& in, int ver, ccglobal::Tracer* tracer) override;
    };

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

    class MSBASE_API CXNDGeometryWrapper : public ccglobal::Serializeable {
    public:
        CXNDGeometryWrapper();
        ~CXNDGeometryWrapper();

        CXNDGeometry geometry;

        int version() override;
        bool save(std::fstream& out, ccglobal::Tracer* tracer) override;
        bool load(std::fstream& in, int ver, ccglobal::Tracer* tracer) override;
    };

    class MSBASE_API CXNDGeometrysWrapper : public ccglobal::Serializeable {
    public:
        CXNDGeometrysWrapper();
        ~CXNDGeometrysWrapper();

        CXNDGeometrys geometrys;

        int version() override;
        bool save(std::fstream& out, ccglobal::Tracer* tracer) override;
        bool load(std::fstream& in, int ver, ccglobal::Tracer* tracer) override;
    };

    class MSBASE_API CXNDFacePatchsWrapper : public ccglobal::Serializeable {
    public:
        CXNDFacePatchsWrapper();
        ~CXNDFacePatchsWrapper();

        CXNDFacePatchs facePatchs;

        int version() override;
        bool save(std::fstream& out, ccglobal::Tracer* tracer) override;
        bool load(std::fstream& in, int ver, ccglobal::Tracer* tracer) override;
    };
    MSBASE_API void loadPoly(std::fstream& in, CXNDPolygon& poly);
    MSBASE_API void savePoly(std::fstream& out, const CXNDPolygon& poly);

	MSBASE_API void loadPolys(std::fstream& in, CXNDPolygons& polys);
	MSBASE_API void savePolys(std::fstream& out, const CXNDPolygons& polys);

    MSBASE_API void loadGeometry(std::fstream& in, CXNDGeometry& geometry);
    MSBASE_API void saveGeometry(std::fstream& out, const CXNDGeometry& geometry);

    MSBASE_API void loadGeometrys(std::fstream& in, CXNDGeometrys& geometrys);
    MSBASE_API void saveGeometrys(std::fstream& out, const CXNDGeometrys& geometrys);

    MSBASE_API void loadFacePatchs(std::fstream& in, CXNDFacePatchs& facePatchs);
    MSBASE_API void saveFacePatchs(std::fstream& out, const CXNDFacePatchs& facePatchs);
}

#endif // MSBASE_TRIMESHSERIAL_1691656350835_H