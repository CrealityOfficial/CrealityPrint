#ifndef _NULLSPACE_ANSYCWORKERCHENGFEI_1590248311419_H
#define _NULLSPACE_ANSYCWORKERCHENGFEI_1590248311419_H
#include "qtusercore/module/job.h"
#include <QtCore/QThread>
#include "us/usettings.h"
#include "chengfeislice.h"

namespace creative_kernel
{
	class SliceAttain;
	class AnsycWorkerCF : public qtuser_core::Job
	{
		Q_OBJECT
	public:
		AnsycWorkerCF(const chengfeiSplit& chengfeiSplit,QObject* parent = nullptr);
		virtual ~AnsycWorkerCF();

		void setRemainAttain(SliceAttain* attain);
	protected:
		QString name() override;
		QString description() override;
		void failed() override;
		void successed(qtuser_core::Progressor* progressor) override;
		void work(qtuser_core::Progressor* progressor) override;

		bool needReSlice();
	signals:
		void sliceSuccess(SliceAttain* attain);
		void sliceFailed();

	protected:
		SliceAttain* m_sliceAttain;
		QScopedPointer<SliceAttain> m_remainAttain;

	private:
		chengfeiSplit m_chengfeiSplit;
	};
}
#endif // _NULLSPACE_ANSYCWORKERCHENGFEI_1590248311419_H
