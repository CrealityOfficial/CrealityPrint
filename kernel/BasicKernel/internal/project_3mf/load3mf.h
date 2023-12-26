#ifndef _LOAD3MF_H
#define _LOAD3MF_H
#include "menu/actioncommand.h"
#include "qtusercore/module/cxhandlerbase.h"
#include "qtusercore/module/job.h"
#include "data/interface.h"

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

#endif // _LOAD3MF_H
