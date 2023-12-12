#ifndef _GCODELOADJOB_1603798647560_H
#define _GCODELOADJOB_1603798647560_H
#include "cxgcode/interface.h"
#include "qtusercore/module/job.h"

namespace cxgcode
{
	class SliceAttain;
	class CXGCODE_API GCodeLoadJob : public qtuser_core::Job
	{
		Q_OBJECT
	public:
		GCodeLoadJob(QObject* parent = nullptr);
		virtual ~GCodeLoadJob();

		void setFileName(const QString& fileName);
		void setSlicePath(const QString& slicePath);
		void setTempPath(const QString& tempPath);
	protected:
		QString name() override;
		QString description() override;
		void failed() override;
		void successed(qtuser_core::Progressor* progressor) override;
		void work(qtuser_core::Progressor* progressor) override;

		SliceAttain* take();
	signals:
		void sliceSuccess(cxgcode::SliceAttain* attain);
		void sliceFailed();
	protected:
		SliceAttain* m_attain;
		QString m_fileName;

		QString m_slicePath;
		QString m_tempPath;
	};
}
#endif // _GCODELOADJOB_1603798647560_H