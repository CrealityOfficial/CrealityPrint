#ifndef _PREVIEWBASE_1603798647560_H
#define _PREVIEWBASE_1603798647560_H
#include "qtusercore/module/job.h"

namespace creative_kernel
{
	class SliceAttain;
	class PreviewBase : public qtuser_core::Job
	{
		Q_OBJECT
	public:
		PreviewBase(QObject* parent = nullptr);
		virtual ~PreviewBase();

		void setFileName(const QString& fileName);
	protected:
		QString name() override;
		QString description() override;
		void failed() override;
		void successed(qtuser_core::Progressor* progressor) override;

		SliceAttain* take();
	signals:
		void sliceSuccess(SliceAttain* attain);
		void sliceFailed();
	protected:
		SliceAttain* m_attain;
		QString m_fileName;
	};
}
#endif // _PREVIEWBASE_1603798647560_H