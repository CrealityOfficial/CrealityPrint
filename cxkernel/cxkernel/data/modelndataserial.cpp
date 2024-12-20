#include "modelndataserial.h"

#include <QtCore/QDebug>
#include "msbase/utils/trimeshserial.h"

namespace cxkernel
{
	ModelNDataSerial::ModelNDataSerial()
	{

	}

	ModelNDataSerial::~ModelNDataSerial()
	{

	}

	void ModelNDataSerial::setData(ModelNDataPtr data)
	{
		m_data = data;
	}

	ModelNDataPtr ModelNDataSerial::getData()
	{
		return m_data;
	}

	void ModelNDataSerial::setMeshData(cxkernel::MeshDataPtr meshData)
	{
		m_meshData = meshData;
	}

	void ModelNDataSerial::setColorData(const std::vector<std::string>& colors)
	{
		m_colors = colors;
	}

	void ModelNDataSerial::setSeamData(const std::vector<std::string>& seams)
	{
		m_seams = seams;
	}

	void ModelNDataSerial::setSupportData(const std::vector<std::string>& supports)
	{
		m_supports = supports;
	}

	void ModelNDataSerial::load(const QString& fileName, ccglobal::Tracer* tracer)
	{
		if (m_data)
		{
			qDebug() << QString("ModelNDataSerial::load data must be null!");
			return;
		}

		m_data.reset(new ModelNData());

		QByteArray cdata = fileName.toLocal8Bit();
		bool result = ccglobal::cxndLoad(*this, std::string(cdata), tracer);

		if (!result)
			m_data.reset();
	}

	void ModelNDataSerial::save(const QString& fileName, ccglobal::Tracer* tracer)
	{
		if (!m_data)
		{
			qDebug() << QString("ModelNDataSerial::save data must be valid!");
			return;
		}

		QByteArray cdata = fileName.toLocal8Bit();
		bool result = ccglobal::cxndSave(*this, std::string(cdata), tracer);
		if(!result)
			qDebug() << QString("ModelNDataSerial::save error!");
	}

	int ModelNDataSerial::version()
	{
		//return 0;
		//return 1; //add spread
		return 2; // meshData + spread
	}

	bool ModelNDataSerial::save(std::fstream& out, ccglobal::Tracer* tracer)
	{
		if (version() >= 2)
		{
			TriMeshPtr mesh = m_meshData->mesh;
			msbase::saveTrimesh(out, mesh.get());

			msbase::saveTrimesh(out, m_meshData->hull.get());
			trimesh::vec3 _offset(m_meshData->offset);
			ccglobal::cxndSaveT<trimesh::vec3>(out, _offset);

			ccglobal::cxndSaveStrs(out, m_colors);
			ccglobal::cxndSaveStrs(out, m_seams);
			ccglobal::cxndSaveStrs(out, m_supports);
		}
		else
		{
			TriMeshPtr mesh = m_data->mesh;
			msbase::saveTrimesh(out, mesh.get());

			msbase::saveTrimesh(out, m_data->hull.get());
			ccglobal::cxndSaveT<trimesh::vec3>(out, m_data->offset);

			if (version() >= 1)
			{
				ccglobal::cxndSaveStrs(out, m_data->colors);
				ccglobal::cxndSaveStrs(out, m_data->seams);
				ccglobal::cxndSaveStrs(out, m_data->supports);
			}
		}
		return true;
	}

	bool ModelNDataSerial::load(std::fstream& in, int ver, ccglobal::Tracer* tracer)
	{
		if (version() >= 2)
		{
			TriMeshPtr mesh(msbase::loadTrimesh(in));
			if (!mesh)
				return false;

			m_meshData->mesh = mesh;

			TriMeshPtr hull(msbase::loadTrimesh(in));
			m_meshData->hull = hull;
			trimesh::vec3 _offset;
			ccglobal::cxndLoadT<trimesh::vec3>(in, _offset);
			m_meshData->offset = trimesh::dvec3(_offset);

			ccglobal::cxndLoadStrs(in, m_colors);
			ccglobal::cxndLoadStrs(in, m_seams);
			ccglobal::cxndLoadStrs(in, m_supports);

			if (!m_data)
			{
				m_data.reset(new ModelNData);
				m_data->mesh = m_meshData->mesh;
				m_data->hull = m_meshData->hull;
				m_data->offset = m_meshData->offset;

				m_data->colors = m_colors;
				m_data->seams = m_seams;
				m_data->supports = m_supports;
			}
		}
		else
		{
			TriMeshPtr mesh(msbase::loadTrimesh(in));
			if (!mesh)
				return false;

			m_data->mesh = mesh;
			m_data->input.description = QString("quick");
			m_data->input.mesh = mesh;

			TriMeshPtr hull(msbase::loadTrimesh(in));
			m_data->hull = hull;
			ccglobal::cxndLoadT<trimesh::vec3>(in, m_data->offset);

			if (version() >= 1)
			{
				ccglobal::cxndLoadStrs(in, m_data->colors);
				ccglobal::cxndLoadStrs(in, m_data->seams);
				ccglobal::cxndLoadStrs(in, m_data->supports);
			}
		}
		return true;

	}
}