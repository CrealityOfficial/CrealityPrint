#include "meshdatawrapper.h"
#include <QFile>
#include <QIODevice>
#include "msbase/utils/trimeshserial.h"
#include "dataidentifier.h"
#include <QTimer>
#include "crslice2/repair.h"

namespace creative_kernel
{
    MeshDataWrapper::MeshDataWrapper(int id, cxkernel::MeshDataPtr data) :
        DataSerial(id, "mesh_data")
    {
        m_needRepair = false;
        m_isCleared = false;
        m_data = data;
		generateIdentifier();
    }

    MeshDataWrapper::~MeshDataWrapper() 
    {
        m_storeLaterTimer.stop();
        // if (m_serialized)
        // {
        //     QString fileName = serialFileName();
        //     QFile::remove(fileName);
        // }
    }
	
	bool MeshDataWrapper::load()
    {
        // if (!m_serialized)
        //     return true;

        QString fileName = serialFileName();
        std::string cfileName = fileName.toStdString();
		std::fstream in(cfileName, std::ios::in | std::ios::binary);
		if (!in.is_open())
		{
            qWarning() << "open mesh file failed, file id: " << ID() << ", mesh size: " << m_identifier->size();
			
            QFile file(fileName);
            if (!file.open(QIODevice::ReadOnly))
            {
                qWarning() << file.errorString();
            }
            
            return false;
		}
        m_isCleared = false;

		cxkernel::MeshData* data = new cxkernel::MeshData();
        TriMeshPtr mesh(msbase::loadTrimesh(in));
		data->mesh = mesh;

        TriMeshPtr hull(msbase::loadTrimesh(in));
		data->hull = hull;
			
        ccglobal::cxndLoadT<trimesh::dvec3>(in, data->offset);

		m_data.reset(data);
        
		in.close();

		return true;

    }

	bool MeshDataWrapper::store()
    {
        //  if (m_serialized)
        //      return true;
        QString fileName = serialFileName();
		QByteArray cdata = fileName.toLocal8Bit();
        std::string cfileName = std::string(cdata);
		std::fstream out(cfileName, std::ios::out | std::ios::binary);
		if (!out.is_open())
		{
			out.close();
			return false;
		}
        m_serialized = true;

		msbase::saveTrimesh(out, m_data->mesh.get());
		msbase::saveTrimesh(out, m_data->hull.get());
		ccglobal::cxndSaveT<trimesh::dvec3>(out, m_data->offset);

		out.close();
        // m_data.reset();
        return true;
    }
	
	void MeshDataWrapper::generateIdentifier()
    {
        m_identifier->generate(m_data->mesh.get());
    }
	
    void MeshDataWrapper::clear()
    {
        qWarning() << "remove mesh data: id " << ID() << ", size " << m_data->mesh->vertices.size();
        m_isCleared = true;
        m_data.reset();
    }

	cxkernel::MeshDataPtr MeshDataWrapper::data()
    {
        if (m_isCleared)
            load();
        
        clearLater();
        return m_data;
    }

	void MeshDataWrapper::checkMesh()
    {
        cxkernel::MeshDataPtr meshData = data();
#ifdef Q_OS_WINDOWS
        crslice2::CheckInfo info = crslice2::checkMesh(meshData->mesh.get());
        if (!info.warning_icon_name.empty())
            m_needRepair = true;
        else
            m_needRepair = false;
#else
		m_needRepair = false;
#endif
    }

    bool MeshDataWrapper::needRepair()
    {
        return m_needRepair;
    }
}