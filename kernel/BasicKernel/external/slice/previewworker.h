#ifndef _PREVIEWWORKER_1603796964788_H
#define _PREVIEWWORKER_1603796964788_H
#include "qtusercore/module/cxhandlerbase.h"
#include <QtCore/QObject>
#include <QtCore/QStringList>

namespace creative_kernel
{
	class SliceFlow;
	class PreviewWorker : public QObject,
		public qtuser_core::CXHandleBase
	{
	public:
		PreviewWorker(QObject* parent = nullptr);
		virtual ~PreviewWorker();
	public:

		QString filter() override;

		void handle(const QString& fileName) override;
	protected:
		SliceFlow* m_receiver;
	};
}

#endif // _PREVIEWWORKER_1603796964788_H