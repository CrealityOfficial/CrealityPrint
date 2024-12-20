#include "msbase/utils/trimeshserial.h"

namespace msbase
{
	using namespace ccglobal;
	trimesh::TriMesh* loadTrimesh(std::fstream& in)
	{
		trimesh::TriMesh* mesh = nullptr;
		int have = 0;
		cxndLoadT<int>(in, have);
		if (have == 1)
		{
			mesh = new trimesh::TriMesh();
			cxndLoadVectorT(in, mesh->vertices);
			cxndLoadVectorT(in, mesh->faces);
			cxndLoadVectorT(in, mesh->faceUVs);
			cxndLoadVectorT(in, mesh->colors);

			cxndLoadT(in, mesh->bbox.min);
			cxndLoadT(in, mesh->bbox.max);
			mesh->bbox.valid = true;
		}

		return mesh;
	}

	bool loadTrimesh(std::fstream& in, trimesh::TriMesh& mesh)
	{
		int have = 0;
		cxndLoadT<int>(in, have);
		if (have == 1)
		{
			cxndLoadVectorT(in, mesh.vertices);
			cxndLoadVectorT(in, mesh.faces);
			cxndLoadVectorT(in, mesh.faceUVs);
			cxndLoadVectorT(in, mesh.colors);

			cxndLoadT(in, mesh.bbox.min);
			cxndLoadT(in, mesh.bbox.max);
			mesh.bbox.valid = true;
			return true;
		}

		return false;
	}

	void saveTrimesh(std::fstream& out, trimesh::TriMesh* mesh)
	{
		int have = mesh ? 1 : 0;
		cxndSaveT<int>(out, have);
		if (have)
		{
			cxndSaveVectorT(out, mesh->vertices);
			cxndSaveVectorT(out, mesh->faces);
			cxndSaveVectorT(out, mesh->faceUVs);
			cxndSaveVectorT(out, mesh->colors);

			cxndSaveT(out, mesh->bbox.min);
			cxndSaveT(out, mesh->bbox.max);
		}
	}

	void saveTrimesh(std::fstream& out, const trimesh::TriMesh& mesh)
	{
		int have = 1;
		cxndSaveT<int>(out, have);
		if (have)
		{
			cxndSaveVectorT(out, mesh.vertices);
			cxndSaveVectorT(out, mesh.faces);
			cxndSaveVectorT(out, mesh.faceUVs);
			cxndSaveVectorT(out, mesh.colors);

			cxndSaveT(out, mesh.bbox.min);
			cxndSaveT(out, mesh.bbox.max);
		}
	}

    void loadPoly(std::fstream& in, CXNDPolygon& poly)
    {
        cxndLoadVectorT(in, poly);
    }

    void savePoly(std::fstream& out, const CXNDPolygon& poly)
    {
        cxndSaveVectorT(out, poly);
    }

    void loadPolys(std::fstream& in, CXNDPolygons& polys)
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

	void savePolys(std::fstream& out, const CXNDPolygons& polys)
	{
		int size = (int)polys.size();
		cxndSaveT(out, size);
		for (int i = 0; i < size; ++i)
			cxndSaveVectorT(out, polys.at(i));
	}

    void loadGeometry(std::fstream& in, CXNDGeometry& geometry)
    {
        loadPoly(in, geometry.contour);
        loadPolys(in, geometry.holes);
    }

    void saveGeometry(std::fstream& out, const CXNDGeometry& geometry)
    {
        savePoly(out, geometry.contour);
        savePolys(out, geometry.holes);
    }

    void loadGeometrys(std::fstream& in, CXNDGeometrys& geometrys)
    {
        int size = 0;
        cxndLoadT(in, size);
        if (size > 0) {
            geometrys.resize(size);
            for (int i = 0; i < size; ++i)
                loadGeometry(in, geometrys.at(i));
        }
    }

    void saveGeometrys(std::fstream& out, const CXNDGeometrys& geometrys)
    {
        int size = (int)geometrys.size();
        cxndSaveT(out, size);
        for (int i = 0; i < size; ++i)
            saveGeometry(out, geometrys.at(i));
    }

    void loadFacePatchs(std::fstream& in, CXNDFacePatchs& facePatchs)
    {
        int size = 0;
        cxndLoadT(in, size);
        if (size > 0) {
            facePatchs.resize(size);
            for (int i = 0; i < size; ++i)
                cxndLoadVectorT(in, facePatchs.at(i));
        }
    }

    void saveFacePatchs(std::fstream & out, const CXNDFacePatchs& facePatchs)
    {
        int size = (int)facePatchs.size();
        cxndSaveT(out, size);
        for (int i = 0; i < size; ++i)
            cxndSaveVectorT(out, facePatchs.at(i));
    }

    CXNDPolygonWrapper::CXNDPolygonWrapper()
    {
    }
    CXNDPolygonWrapper::~CXNDPolygonWrapper()
    {
    }

    int CXNDPolygonWrapper::version()
    {
        return 0;
    }

    bool CXNDPolygonWrapper::save(std::fstream& out, ccglobal::Tracer* tracer)
    {
        savePoly(out, poly);
        return true;
    }

    bool CXNDPolygonWrapper::load(std::fstream& in, int ver, ccglobal::Tracer* tracer)
    {
        if (ver == 0) {
            loadPoly(in, poly);
            return true;
        }
        return false;
    }

	CXNDPolygonsWrapper::CXNDPolygonsWrapper()
	{

	}

	CXNDPolygonsWrapper::~CXNDPolygonsWrapper()
	{

	}

	int CXNDPolygonsWrapper::version()
	{
		return 0;
	}

	bool CXNDPolygonsWrapper::save(std::fstream& out, ccglobal::Tracer* tracer)
	{
		savePolys(out, polys);
		return true;
	}

	bool CXNDPolygonsWrapper::load(std::fstream& in, int ver, ccglobal::Tracer* tracer)
	{
		if (ver == 0)
		{
			loadPolys(in, polys);
			return true;
		}
		return false;
	}

    CXNDGeometryWrapper::CXNDGeometryWrapper()
    {
    }

    CXNDGeometryWrapper::~CXNDGeometryWrapper()
    {
    }

    int CXNDGeometryWrapper::version()
    {
        return 0;
    }

    bool CXNDGeometryWrapper::save(std::fstream& out, ccglobal::Tracer* tracer)
    {
        saveGeometry(out, geometry);
        return true;
    }

    bool CXNDGeometryWrapper::load(std::fstream& in, int ver, ccglobal::Tracer* tracer)
    {
        if (ver == 0) {
            loadGeometry(in, geometry);
            return true;
        }
        return false;
    }

    CXNDFacePatchsWrapper::CXNDFacePatchsWrapper()
    {

    }

    CXNDFacePatchsWrapper::~CXNDFacePatchsWrapper()
    {

    }

    int CXNDFacePatchsWrapper::version()
    {
        return 0;
    }

    bool CXNDFacePatchsWrapper::save(std::fstream& out, ccglobal::Tracer* tracer)
    {
        saveFacePatchs(out, facePatchs);
        return true;
    }

    bool CXNDFacePatchsWrapper::load(std::fstream& in, int ver, ccglobal::Tracer* tracer)
    {
        if (ver == 0) {
            loadFacePatchs(in, facePatchs);
            return true;
        }
        return false;
    }
    
    CXNDGeometrysWrapper::CXNDGeometrysWrapper()
    {
    }

    CXNDGeometrysWrapper::~CXNDGeometrysWrapper()
    {
    }

    int CXNDGeometrysWrapper::version()
    {
        return 0;
    }

    bool CXNDGeometrysWrapper::save(std::fstream& out, ccglobal::Tracer* tracer)
    {
        saveGeometrys(out, geometrys);
        return true;
    }

    bool CXNDGeometrysWrapper::load(std::fstream& in, int ver, ccglobal::Tracer* tracer)
    {
        if (ver == 0) {
            loadGeometrys(in, geometrys);
            return true;
        }
        return false;
    }

}