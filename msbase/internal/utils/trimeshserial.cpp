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
}