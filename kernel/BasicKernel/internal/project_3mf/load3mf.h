#ifndef _LOAD3MF_H
#define _LOAD3MF_H
#include "menu/actioncommand.h"
#include "qtusercore/module/job.h"
#include "data/rawdata.h"
#include "data/interface.h"
#include "common_3mf.h"

namespace creative_kernel
{
	class Load3MFCommand : public ActionCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		Load3MFCommand(QObject* parent = nullptr);
		virtual ~Load3MFCommand();

		Q_INVOKABLE void execute();
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	};

	class Load3MFJob : public qtuser_core::Job
	{
		Q_OBJECT
	public:
		Load3MFJob(QObject* parent = nullptr);
		virtual ~Load3MFJob();

		void setFileName(const QString& fileName);
		void setOpenProject(bool open_project);
	protected:
		QString name();
		QString description();
		void failed();                        // invoke from main thread
		void successed(qtuser_core::Progressor* progressor);                     // invoke from main thread
		void work(qtuser_core::Progressor* progressor);    // invoke from worker thread
		creative_kernel::ModelNType getModelNTypeBySubType(const std::string& stype);
		void createSceneData();

	protected:
		QString m_fileName;

		QString m_projectConfigFile;
		common_3mf::Scene3MF m_scene;
		SceneCreateData m_scene_create_data;

		bool m_open_project = true;	// defualt is open 3mf
		bool m_read3mfState = false;
	};

	struct MeshWithName
	{
		TriMeshPtr mesh;
		std::string name;
	};

	std::vector<MeshWithName> load_meshes_from_3mf(const QString& file_name, ccglobal::Tracer* tracer = nullptr);
}

#endif // _LOAD3MF_H
