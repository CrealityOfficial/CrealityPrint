#ifndef _CREATIVE_KERNEL_JOB_1590379536521_H
#define _CREATIVE_KERNEL_JOB_1590379536521_H
#include "qtusercore/qtusercoreexport.h"
#include "qtusercore/module/progressor.h"
#include <QtCore/QSharedPointer>
#include <QtCore/QThread>

namespace qtuser_core
{
	class QTUSER_CORE_API Job: public QObject
	{
		Q_OBJECT
	public:
		Job(QObject* parent = nullptr);
		virtual ~Job();

		virtual bool showProgress() { return true; }
		virtual QString name();
		virtual QString description();
		virtual void failed();                        // invoke from main thread
		virtual void prepare();                       // invoke from main thread
		virtual void successed(qtuser_core::Progressor* progressor);                     // invoke from main thread
		virtual void work(Progressor* progressor);    // invoke from worker thread
	protected:
	};

	typedef QSharedPointer<Job> JobPtr;
}
#endif // _CREATIVE_KERNEL_JOB_1590379536521_H
