#ifndef _NULLSPACE_MESHIOINTERFACE_1589529147227_H
#define _NULLSPACE_MESHIOINTERFACE_1589529147227_H
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace trimesh
{
	class TriMesh;
}

namespace qtuser_core
{
	class Progressor;
}
class MeshIOInterface
{
public:
	virtual ~MeshIOInterface() {}

	virtual QString description() = 0;
	virtual QStringList saveSupportExtension() = 0;  // for example "stl ply"
	virtual QStringList loadSupportExtension() = 0;  // for example "stl"

	virtual trimesh::TriMesh* load(const QString& name, qtuser_core::Progressor* progressor) = 0;
	virtual void save(trimesh::TriMesh* mesh, const QString& name, qtuser_core::Progressor* progressor) = 0;
};

#endif // _NULLSPACE_MESHIOINTERFACE_1589529147227_H
