#ifndef _NULLSPACE_ANSYCWORKER_1590248311419_H
#define _NULLSPACE_ANSYCWORKER_1590248311419_H
#include "qtusercore/module/job.h"
#include <QtCore/QThread>
#include "us/usettings.h"
#include "qtusercore/module/formattracer.h"

namespace creative_kernel
{
	class SliceAttainTracer : public qtuser_core::FormatTracer
	{
		Q_OBJECT
	public:
		SliceAttainTracer(qtuser_core::Progressor* progressor);
		virtual ~SliceAttainTracer() {}

		int indexCount() override;

		QString indexStr(int index, va_list vars) override;
	};

	class FormatSlice : public qtuser_core::FormatTracer
	{
		Q_OBJECT
	public:
		FormatSlice(qtuser_core::Progressor* progressor);
		virtual ~FormatSlice() {}

		int indexCount() override;

		QString indexStr(int index, va_list vars);
	};



	class SliceAttain;
	class AnsycWorker : public qtuser_core::Job
	{
		Q_OBJECT
	public:
		AnsycWorker(QObject* parent = nullptr);
		virtual ~AnsycWorker();
		SliceAttain* sliceAttain();
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
		void sliceSuccess(SliceAttain* attain, bool isRemain);
		void sliceBeforeSuccess(SliceAttain* attain);
		void sliceFailed();

	signals:
		void gcodeLayerChanged(int layer);

	protected:
		SliceAttain* m_sliceAttain;
		QScopedPointer<SliceAttain> m_remainAttain;
	};
}
#endif // _NULLSPACE_ANSYCWORKER_1590248311419_H
