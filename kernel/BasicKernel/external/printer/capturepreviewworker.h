#ifndef _NULLSPACE_CAPTUREPREVIEWWORKER_1590248311419_H
#define _NULLSPACE_CAPTUREPREVIEWWORKER_1590248311419_H
#include "qtusercore/module/job.h"
#include <QtCore/QThread>
#include "us/usettings.h"
#include <QImage>
#include <qtuser3d/framegraph/colorpicker.h>
#include "trimesh2/Box.h"
#include <Qt3DRender/QFrameGraphNode>
#include "../slice/slicepreviewlistmodel.h"

namespace creative_kernel
{
	class SlicePreviewFlow;
	class Printer;
	class ModelN;
	class CapturePreviewWorker : public qtuser_core::Job
	{
		Q_OBJECT
	signals:
        void sig_mainThreadWork();
        void sig_capturePreviewSuccessed();

	public:
		CapturePreviewWorker(QObject* parent = nullptr);
		virtual ~CapturePreviewWorker();

		void execute();

		void setSlicePreviewFlow(SlicePreviewFlow* flow);

	protected:
		QString name() override;
		QString description() override;
		void failed() override;
		void successed(qtuser_core::Progressor* progressor) override;

        void mainThreadWork();

		void work(qtuser_core::Progressor* progressor) override;
		bool needReCapture();
		
		void waitForCompleted();

    protected:
		int m_captureNum;
		bool m_isBusy;
		Printer* m_printer;
		QList<ModelN*> m_modelsOnPrinter;

		qtuser_3d::ColorPicker* m_colorPicker;
		Qt3DRender::QFrameGraphNode* m_root;
		SlicePreviewFlow* m_flow;
	};
}
#endif // _NULLSPACE_CAPTUREPREVIEWWORKER_1590248311419_H
