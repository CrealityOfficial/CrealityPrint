#include "photomeshplugin.h"
#include "photo2mesh.h"

#include "cxkernel/interface/iointerface.h"

PhotoMeshPlugin::PhotoMeshPlugin(QObject* parent)
	: QObject(parent)
{
	m_photo2Mesh = new Photo2Mesh(this);
}

PhotoMeshPlugin::~PhotoMeshPlugin()
{
}

QString PhotoMeshPlugin::name()
{
	return "PhotoMeshPlugin";
}

QString PhotoMeshPlugin::info()
{
	return "PhotoMeshPlugin";
}

void PhotoMeshPlugin::initialize()
{
	cxkernel::registerOpenHandler(this);
}

void PhotoMeshPlugin::uninitialize()
{
}

QString PhotoMeshPlugin::filter()
{
	QString _filter = "Image File (*.bmp *.jpg *.jpeg *.png)";
	return _filter;
}

void PhotoMeshPlugin::handle(const QString& fileName)
{
	QStringList fileNames;
	fileNames << fileName;
	handle(fileNames);
};

void PhotoMeshPlugin::handle(const QStringList& fileNames)
{
	m_photo2Mesh->processFiles(fileNames);
};
