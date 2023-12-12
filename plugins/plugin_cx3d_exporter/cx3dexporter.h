#ifndef _NULLSPACE_CX3D_1589849922902_H
#define _NULLSPACE_CX3D_1589849922902_H
#include "qtusercore/module/creativeinterface.h"
#include <QtCore/QUrl>

class Cx3dAutoSaveProject;
namespace creative_kernel
{
	class CX3DImporterCommand;
	class CX3DExporterCommand;
	class CX3DExporter : public QObject, public CreativeInterface
	{
		Q_OBJECT
		Q_PLUGIN_METADATA(IID "creative.CX3DExporter")
		Q_INTERFACES(CreativeInterface)
	public:
		CX3DExporter(QObject* parent = nullptr);
		virtual ~CX3DExporter();

		void setOpenedProjectPath(const QUrl& url);
		QUrl openedPath();
	protected:
		QString name() override;
		QString info() override;

		void initialize() override;
		void uninitialize() override;
		void openDefualtProject();
	public slots:
		void update();
	protected:
		CX3DExporterCommand* m_exportcommand;
		CX3DImporterCommand* m_importcommand;
		Cx3dAutoSaveProject* m_autoCommand;

		QUrl m_openedUrl;
	};
}
#endif // _NULLSPACE_CX3D_1589849922902_H
