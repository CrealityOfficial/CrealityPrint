#include "crobject.h"
#include "jsonhelper.h"

#include "ccglobal/serial.h"

namespace crslice
{
	CrObject::CrObject()
	{
		m_settings.reset(new Settings());
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
	}
}