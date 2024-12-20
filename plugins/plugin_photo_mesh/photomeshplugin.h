#ifndef _NULLSPACE_PluginExtendMesh_1589849922902_H
#define _NULLSPACE_PluginExtendMesh_1589849922902_H
#include "qtusercore/module/creativeinterface.h"
#include "qtusercore/module/cxhandlerbase.h"

class Photo2Mesh;
class PhotoMeshPlugin : public QObject
					  , public CreativeInterface
					  , public qtuser_core::CXHandleBase
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "creative.PhotoMeshPlugin")
	Q_INTERFACES(CreativeInterface)
public:
	PhotoMeshPlugin(QObject* parent = nullptr);
	virtual ~PhotoMeshPlugin();

protected:
	QString name() override;
	QString info() override;

	void initialize() override;
	void uninitialize() override;

	QString filter() override;
	void handle(const QString& fileName) override;
	void handle(const QStringList& fileNames) override;
protected:
	Photo2Mesh* m_photo2Mesh;
};
#endif // _NULLSPACE_PluginExtendMesh_1589849922902_H
