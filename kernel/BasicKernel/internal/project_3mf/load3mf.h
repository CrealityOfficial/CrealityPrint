#ifndef _LOAD3MF_H
#define _LOAD3MF_H
#include "menu/actioncommand.h"
#include "qtusercore/module/cxhandlerbase.h"
#include "qtusercore/module/job.h"
#include "cxkernel/data/modelndata.h"
#include "data/interface.h"
#include "internal/multi_printer/printermanager.h"
#include "internal/multi_printer/printersettings.h"
#include "common_3mf.h"


namespace creative_kernel
{
	class Load3MFCommand : public ActionCommand
		, public UIVisualTracer
		, public qtuser_core::CXHandleBase 
	{
		Q_OBJECT
	public:
		Load3MFCommand(QObject* parent = nullptr);
		virtual ~Load3MFCommand();

		Q_INVOKABLE void execute();
		Q_INVOKABLE QString getMessageText();
		Q_INVOKABLE void accept();
	private:
		QString m_strMessageText;
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
		QString filter() override;
		void handle(const QString& fileName) override;
	protected:
		QString m_fileName;
	};

	class Load3MFJob : public qtuser_core::Job
	{
		Q_OBJECT
	public:
		Load3MFJob(QObject* parent = nullptr);
		virtual ~Load3MFJob();

		void setFileName(const QString& fileName);
		void setOpenJob(bool jobType);		//type is open or load 3mf
	protected:
		QString name();
		QString description();
		void failed();                        // invoke from main thread
		void successed(qtuser_core::Progressor* progressor);                     // invoke from main thread
		void work(qtuser_core::Progressor* progressor);    // invoke from worker thread

	protected:
		QString m_fileName;
		QString m_projectConfigFile;

		common_3mf::Scene3MF m_scene;
		QMap<int, QList<cxkernel::ModelNDataPtr>> m_datas;
		QMap<int, QList<QMap<QString, QString>>> m_settings;
		QList<QMap<float, crslice2::Plate_Item>> m_plates;

		bool m_isOpenJob = true;	// defualt is open 3mf
	};

	struct MeshWithName
	{
		TriMeshPtr mesh;
		std::string name;
	};

	std::vector<MeshWithName> load_meshes_from_3mf(const QString& file_name, ccglobal::Tracer* tracer = nullptr);
}

#endif // _LOAD3MF_H
