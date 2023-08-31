#ifndef _NULLSPACE_ANSYCWORKER_1590248311419_H
#define _NULLSPACE_ANSYCWORKER_1590248311419_H
#include "qtusercore/module/job.h"
#include <QtCore/QThread>
#include "us/usettings.h"

namespace creative_kernel
{
	class SliceAttain;
	class AnsycWorker : public qtuser_core::Job
	{
		Q_OBJECT
	public:
		AnsycWorker(QObject* parent = nullptr);
		virtual ~AnsycWorker();

		void setRemainAttain(SliceAttain* attain);
	protected:
		QString name() override;
		QString description() override;
		void failed() override;
		void successed(qtuser_core::Progressor* progressor) override;
		void work(qtuser_core::Progressor* progressor) override;

		bool needReSlice();

	signals:
		void sliceMessage(const QString& message);
		void sliceSuccess(SliceAttain* attain);
		void sliceFailed();

	protected:
		SliceAttain* m_sliceAttain;
		QScopedPointer<SliceAttain> m_remainAttain;
	};
}
#endif // _NULLSPACE_ANSYCWORKER_1590248311419_H
