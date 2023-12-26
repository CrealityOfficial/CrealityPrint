#ifndef _SAVE3MF_H
#define _SAVE3MF_H
#include "menu/actioncommand.h"
#include "qtusercore/module/cxhandlerbase.h"
#include "qtusercore/module/job.h"
#include "data/interface.h"

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

	private:
		QString m_strMessageText;
	protected:
		QString name();
		QString description();
		void failed();                        // invoke from main thread
		void successed(qtuser_core::Progressor* progressor);                     // invoke from main thread
		void work(qtuser_core::Progressor* progressor);    // invoke from worker thread

	protected:
		QString m_fileName;
	};
}

#endif // _SAVE3MF_H
