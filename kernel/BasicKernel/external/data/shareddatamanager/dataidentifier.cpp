#include "dataidentifier.h"
#include <QCryptographicHash>
#include "cxkernel/data/meshdata.h"
#include <QElapsedTimer>
#include <QDebug>

namespace creative_kernel
{
    DataIdentifier::DataIdentifier()
    {

    }

    DataIdentifier::~DataIdentifier()
    {

    }

    void DataIdentifier::generate(const trimesh::TriMesh* mesh)
    {
        QElapsedTimer timer;
        timer.start();
        m_type = Mesh;
        m_size = mesh->faces.size();

        QByteArray serial = formatTriMesh(mesh);
        QCryptographicHash cry(QCryptographicHash::Sha3_256);
        cry.addData(serial);
        m_identifier = cry.result();

        qDebug() << "generate mesh identifier spend : " << timer.elapsed() << "ms";
    }

    void DataIdentifier::generate(const std::vector<std::string>& spreadData)
    {
        QElapsedTimer timer;
        timer.start();
        m_type = Spread;
        m_size = spreadData.size();

        QByteArray serial = formatSpreadData(spreadData);
        QCryptographicHash cry(QCryptographicHash::Sha3_256);
        cry.addData(serial);
        m_identifier = cry.result();

        qDebug() << "generate spread identifier spend : " << timer.elapsed() << "ms";
    }

    int DataIdentifier::size()
    {
        return m_size;
    }

    bool DataIdentifier::operator==(const DataIdentifier& other) const
    {
        return m_type == other.m_type &&
               m_size == other.m_size &&
               m_identifier == other.m_identifier;
    }

    bool DataIdentifier::operator!=(const DataIdentifier& other) const
    {
        return m_type != other.m_type ||
               m_size != other.m_size ||
               m_identifier != other.m_identifier;
    }

    QByteArray DataIdentifier::formatTriMesh(const trimesh::TriMesh* mesh)
    {
        QByteArray result;
        int size = sizeof(trimesh::vec3) * mesh->vertices.size();
        result = QByteArray((char*)mesh->vertices.data(), size);

        size = sizeof(trimesh::TriMesh::Face) * mesh->faces.size();
        result += QByteArray((char*)mesh->faces.data(), size);
        return result;
    }

    QByteArray DataIdentifier::formatSpreadData(const std::vector<std::string>& spreadData)
    {
        QString result;
        for (std::string faceSpread : spreadData)
        {
            result += (QString(faceSpread.data()) + "\n");
        }
        return result.toLatin1();
    }
}