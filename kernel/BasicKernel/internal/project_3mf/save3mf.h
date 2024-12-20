#ifndef _SAVE3MF_H
#define _SAVE3MF_H
#include "menu/actioncommand.h"
#include "qtusercore/module/cxhandlerbase.h"
#include "qtusercore/module/job.h"
#include "data/interface.h"
#include "ccglobal/tracer.h"

namespace creative_kernel
{
	class Save3MFCommand : public ActionCommand
		, public UIVisualTracer
		, public qtuser_core::CXHandleBase 
	{
		Q_OBJECT
	public:
		Save3MFCommand(QObject* parent = nullptr);
		virtual ~Save3MFCommand();

		Q_INVOKABLE void execute();
		Q_INVOKABLE void accept();
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
		QString filter() override;
		void handle(const QString& fileName) override;
	protected:
		QString m_fileName;
	};


	class Save3MFJob : public qtuser_core::Job
	{
		Q_OBJECT
	public:
		Save3MFJob(QObject* parent = nullptr);
		virtual ~Save3MFJob();

		void setFileName(const QString& fileName);

		Q_INVOKABLE QString getMessageText();
		void setShowProgress(bool show);
		void setRemoveProjectCache(bool cache);
	private:
		QString m_strMessageText;
	protected:
		bool showProgress() override;
		QString name();
		QString description();
		void failed();                        // invoke from main thread
		void successed(qtuser_core::Progressor* progressor);                     // invoke from main thread
		void work(qtuser_core::Progressor* progressor);    // invoke from worker thread

	protected:
		QString m_fileName;
		bool m_showProgress;
		bool m_removeProjectCache;
	};
	void save_scene_2_3mf(const QString& file_name, QList<creative_kernel::ModelGroup*> mglist=QList<creative_kernel::ModelGroup*>(),ccglobal::Tracer* tracer = nullptr,bool show_progress=false);
	void save_current_scene_2_3mf(const QString& file_name, ccglobal::Tracer* tracer = nullptr,bool show_progress=false);
	void save_current_scene_2_3mf_without_group(const QString& file_name, ccglobal::Tracer* tracer = nullptr);
	void save_current_scene_2_3mf_with_group(const QString& file_name, QList<creative_kernel::ModelGroup*> mglist,ccglobal::Tracer* tracer = nullptr);
	void save_gcode_2_3mf(const QString& file_name, QList<int> plate_ids, bool save_scene = false);
}

#endif // _SAVE3MF_H
