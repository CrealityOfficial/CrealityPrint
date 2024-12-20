#include "capturepreviewworker.h"
#include "qtusercore/module/systemutil.h"
#include "qtusercore/module/progressortracer.h"

#include <QtCore/QTimeLine>
#include <QtCore/QTime>
#include<QStandardPaths>

#include "interface/spaceinterface.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/machineinterface.h"
#include "interface/printerinterface.h"
#include "external/interface/camerainterface.h"
#include "external/interface/renderinterface.h"
#include "external/interface/reuseableinterface.h"
#include "external/interface/spaceinterface.h"
#include "external/interface/uiinterface.h"
#include "external/slice/sliceattain.h"
#include "external/data/modeln.h"
#include "external/data/modelgroup.h"

#include <qtuser3d/camera/screencamera.h>
#include "qtuser3d/trimesh2/camera.h"
#include "qtuser3d/trimesh2/conv.h"
#include "qtuser3d/math/box3d.h"

#include <Qt3DRender/QCamera>
#include <QCoreApplication>
#include "../slice/slicepreviewflow.h"
#include "internal/multi_printer/printer.h"
#include "data/modeln.h"

namespace creative_kernel
{
	CapturePreviewWorker::CapturePreviewWorker(QObject* parent)
    {
        connect(this, &CapturePreviewWorker::sig_mainThreadWork, this, &CapturePreviewWorker::mainThreadWork, Qt::BlockingQueuedConnection);
    }

	CapturePreviewWorker::~CapturePreviewWorker()
    {

    }

	void CapturePreviewWorker::setSlicePreviewFlow(SlicePreviewFlow* flow)
	{
		m_flow = flow;
	}

	QString CapturePreviewWorker::name()
	{
		return "CapturePreviewWorker.";
	}

	QString CapturePreviewWorker::description()
	{
		return "CapturePreviewWorker.";
	}

	void CapturePreviewWorker::failed() 
    {

    }

	void CapturePreviewWorker::successed(qtuser_core::Progressor* progressor) 
    {
        emit sig_capturePreviewSuccessed();
    }

    void CapturePreviewWorker::mainThreadWork()
    {
		///* 初始化 */
		//m_captureNum = printersCount();
		//m_isBusy = false;

		//auto imageCallback = [this](const QImage& image)
		//{
		//	m_printer->setPicture(image);
		//	m_isBusy = false;
		//};
		//
  //      for (int i = 0, count = printersCount(); i < count; ++i)
  //      {
		//	m_printer = getPrinter(i);
		//	m_modelsOnPrinter = m_printer->modelsInside();

		//	if (needReCapture())
		//	{
		//		m_printer->setIsSliced(false);
		//		m_printer->setAttain(NULL);
		//		m_isBusy = true;
		//		m_flow->requestPreview(m_modelsOnPrinter, imageCallback);
		//		waitForCompleted();
		//		m_flow->endRequest(m_printer->picture());
		//	}
		//	else 
		//	{
		//		m_printer->setIsSliced(true);
		//	}
  //      }
    }

	void CapturePreviewWorker::work(qtuser_core::Progressor* progressor) 
    {
		emit sig_mainThreadWork();
    }

	bool CapturePreviewWorker::needReCapture()
	{
		if (m_printer->isDirty())
			return true;

		bool isModelDirty = false;
		for (auto m : m_modelsOnPrinter)
		{
			if (m->isDirty())
			{
			 	isModelDirty = true;
				break;
			}
		}
		if (isModelDirty)
			return true;

		return false;
	}

	void CapturePreviewWorker::waitForCompleted()
	{
		while (m_isBusy)
		{
			QThread::msleep(20);
			QCoreApplication::processEvents();
		}
	}
}