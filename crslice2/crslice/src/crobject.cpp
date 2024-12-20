#include "crobject.h"
#include "jsonhelper.h"

#include "ccglobal/serial.h"

namespace crslice2
{
	void loadXForm(trimesh::xform& xf, std::fstream& in)
	{
		in.read((char*)xf.data(), 16 * sizeof(double));
	}

	void saveXForm(const trimesh::xform& xf, std::fstream& out)
	{
		out.write((const char*)xf.data(), 16 * sizeof(double));
	}

	CrObject::CrObject()
		:modelType(0)
	{
		m_settings.reset(new Settings());
		m_xform = trimesh::xform();
	}

	CrObject::~CrObject()
	{

	}

	void CrObject::load(std::ifstream& in)
	{
		m_settings->load(in);
		int have = templateLoad<int>(in);

		if (have)
		{
			m_mesh.reset(new trimesh::TriMesh());
			templateLoad(m_mesh->faces, in);
			templateLoad(m_mesh->vertices, in);
		}
	}

	void CrObject::save(std::ofstream& out)
	{
		m_settings->save(out);
		int have = m_mesh ? 1 : 0;
		templateSave(have, out);

		if (m_mesh)
		{
			templateSave(m_mesh->faces, out);
			templateSave(m_mesh->vertices, out);
		}
	}

	void CrObject::load(std::fstream& in, int version)
	{
		m_settings->load(in);
		int have = 0;
		ccglobal::cxndLoadT(in, have);

		if (have)
		{
			m_mesh.reset(new trimesh::TriMesh());
			ccglobal::cxndLoadVectorT(in, m_mesh->faces);
			ccglobal::cxndLoadVectorT(in, m_mesh->vertices);

			if (version >= 100)
			{
				ccglobal::cxndLoadVectorT(in, m_mesh->flags);
			}
		}

		if (version >= 101)
		{
			ccglobal::cxndLoadT(in, modelType);
			loadXForm(m_xform, in);
			ccglobal::cxndLoadStrs(in, m_colors2Facets);
			ccglobal::cxndLoadStrs(in, m_support2Facets);
			ccglobal::cxndLoadStrs(in, m_seam2Facets);
		}
	}

	void CrObject::save(std::fstream& out, int version)
	{
		m_settings->save(out);
		int have = m_mesh ? 1 : 0;
		ccglobal::cxndSaveT(out, have);

		if (m_mesh)
		{
			ccglobal::cxndSaveVectorT(out, m_mesh->faces);
			ccglobal::cxndSaveVectorT(out, m_mesh->vertices);

			if (version >= 100)
			{
				ccglobal::cxndSaveVectorT(out, m_mesh->flags);
			}
		}

		if (version >= 101)
		{
			ccglobal::cxndSaveT(out, modelType);
			saveXForm(m_xform, out);
			ccglobal::cxndSaveStrs(out, m_colors2Facets);
			ccglobal::cxndSaveStrs(out, m_support2Facets);
			ccglobal::cxndSaveStrs(out, m_seam2Facets);
		}
	}
}