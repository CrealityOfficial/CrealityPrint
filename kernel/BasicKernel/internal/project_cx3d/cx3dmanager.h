#ifndef _NULLSPACE_CX3D_1589849922902_H
#define _NULLSPACE_CX3D_1589849922902_H
#include <QtCore/QUrl>
#include <QObject>
#include "basickernelexport.h"
#include "data/modelgroup.h"
#include "autosavejob.h"
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
		Q_PROPERTY(int projectTypeIndex READ projectTypeIndex WRITE setProjectTypeIndex NOTIFY projectTypeIndexChanged)

		void createProject();
		Q_INVOKABLE void accept();
		Q_INVOKABLE void triggerAutoSave(QList<creative_kernel::ModelGroup*> modelgroup,AutoSaveJob::SaveType type);
		Q_INVOKABLE void stopAutoSave();
		Q_INVOKABLE void startAutoSave();
		Q_INVOKABLE void newCX3D();
		Q_INVOKABLE void saveCX3D(bool newBuild);
		Q_INVOKABLE void saveAsCX3D(bool newBuild);
		Q_INVOKABLE void saveAs3mf(const QString& filepath);
		QString getProjectName();
		Q_INVOKABLE QUrl defaultPath();
		Q_INVOKABLE void setDefaultPath(const QString& filePath);
		Q_INVOKABLE QUrl defaultName();
		Q_INVOKABLE void exportSinglePlate();
		Q_INVOKABLE void exportSinglePlate(const QUrl& url);
		Q_INVOKABLE void exportAllPlate();
		Q_INVOKABLE QString saveTempGCode3mf();
		Q_INVOKABLE void openProject(const QString& filepath, bool quiet=false);
		//��ԭƽ̨ԭʼ״̬
		Q_INVOKABLE void cancelSave();
		Q_INVOKABLE void initNewProject();
		Q_INVOKABLE bool isProjectOpened();

		Q_INVOKABLE QString getMessageText();
		void savePostProcess(bool result, const QString& msg);				//������Ŀ�ĺ���
		void initialize() ;
		void uninitialize() ;
		void setQuiet(bool quiet) {m_quiet=quiet;}

		void setProjectTypeIndex(int typeIndex);
		int projectTypeIndex();
		bool isNewBuild() { return m_newBuild; }
	signals:
		void projectTypeIndexChanged();

	protected:
		CX3DExporterCommand* m_exportcommand;
		CX3DImporterCommand* m_importcommand;
		Cx3dAutoSaveProject* m_autoCommand;

		QUrl m_openedUrl;
	private:
		bool m_projectNeedSave;
		bool m_newBuild = false;
		bool m_quiet = false;
		QString m_strMessageText;
	};
}
#endif // _NULLSPACE_CX3D_1589849922902_H
