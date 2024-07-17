#ifndef _NULLSPACE_CX3D_1589849922902_H
#define _NULLSPACE_CX3D_1589849922902_H
#include <QtCore/QUrl>
#include <QObject>
#include "basickernelexport.h"
class Cx3dAutoSaveProject;
namespace creative_kernel
{
	class CX3DImporterCommand;
	class CX3DExporterCommand;
	class BASIC_KERNEL_API CX3DManager : public QObject
	{
		Q_OBJECT
	public:
		CX3DManager(QObject* parent = nullptr);
		virtual ~CX3DManager();

		void setOpenedProjectPath(const QUrl& url);
		QUrl openedPath();
	public :
		Q_INVOKABLE bool hasAnyNeedSave() {
			return m_projectNeedSave;
		}
		void createProject();
		Q_INVOKABLE void triggerAutoSave();

		Q_INVOKABLE void newCX3D();
		Q_INVOKABLE void saveCX3D(bool newBuild);
		Q_INVOKABLE void saveAsCX3D(bool newBuild);
		Q_INVOKABLE void openProject(const QString& filepath);
		//还原平台原始状态
		Q_INVOKABLE void cancelSave();
		Q_INVOKABLE void initNewProject();

		void savePostProcess();				//保存项目的后处理
		void initialize() ;
		void uninitialize() ;
	protected:
		CX3DExporterCommand* m_exportcommand;
		CX3DImporterCommand* m_importcommand;
		Cx3dAutoSaveProject* m_autoCommand;

		QUrl m_openedUrl;
	private:
		bool m_projectNeedSave;
		bool m_newBuild = false;
	};
}
#endif // _NULLSPACE_CX3D_1589849922902_H
