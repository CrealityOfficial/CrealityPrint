#ifndef MAIN_THREAD_JOB_H
#define MAIN_THREAD_JOB_H
#include "qtusercore/module/job.h"
#include <QtCore/QThread>

namespace creative_kernel
{
	class SlicePreviewFlow;
	class Printer;
	class ModelN;
	class MainThreadJob : public qtuser_core::Job
	{
		Q_OBJECT
	signals:
		void sig_mainThreadWork();
		void sig_finished();

	public:
		MainThreadJob(QObject* parent = nullptr);
		virtual ~MainThreadJob();

	protected:
		QString name() override;
		QString description() override;
		void failed() override;
		void successed(qtuser_core::Progressor* progressor) override;
		void work(qtuser_core::Progressor* progressor) override;

	};
}
#endif // MAIN_THREAD_JOB_H
