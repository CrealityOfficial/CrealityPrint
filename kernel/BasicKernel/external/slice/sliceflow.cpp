#include "sliceflow.h"
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QUuid>

#include "qtusercore/string/resourcesfinder.h"
#include "qtusercore/module/systemutil.h"

#include "slice/ansycworker.h"
#include "slice/sliceattain.h"
#include "slice/previewworker.h"

#include "slice/slicepreviewflow.h"
#include "internal/render/slicepreviewscene.h"

#include "cxkernel/interface/jobsinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/renderinterface.h"
#include "interface/uiinterface.h"
#include "interface/selectorinterface.h"
#include "interface/spaceinterface.h"
#include "interface/modelinterface.h"
#include "cxkernel/interface/iointerface.h"
#include "interface/cloudinterface.h"
#include "interface/machineinterface.h"

#include "interface/commandinterface.h"
#include "interface/appsettinginterface.h"
#include "kernel/kernelui.h"

#include "qcxutil/quazip/quazipfile.h"
#include "slice/calibrate.h"
#include "cxgcode/gcodeloadjob.h"
#include "calibratescenecreator.h"

#include "kernel/kernel.h"
#include "usbprintcommand.h"

#if defined(CUSTOM_SLICE_HEIGHT_SETTING_ENABLED) && defined(CUSTOM_PARTITION_PRINT_ENABLED)
//chengfei 定制切片
#include "chengfei/chengfeislice.h"
#include "chengfei/ansycworkerchengfei.h"
#endif

#include <menu/ccommandlist.h>

namespace creative_kernel
{
    SliceFlow::SliceFlow(QObject* parent) : QObject(parent)
    {
        m_slicePreviewFlow = new SlicePreviewFlow();
        m_previewWorker = new PreviewWorker(this);
		m_calibrate = new Calibrate(this);
#if defined(CUSTOM_SLICE_HEIGHT_SETTING_ENABLED) && defined(CUSTOM_PARTITION_PRINT_ENABLED)
        m_cSlice = new ChengFeiSlice(this);
#endif
    }

    SliceFlow::~SliceFlow()
    {

    }

    void SliceFlow::registerContext()
    {
        registerContextObject("kernel_slice_model", m_slicePreviewFlow);
		registerContextObject("kernel_slice_calibrate", m_calibrate);
#if defined(CUSTOM_SLICE_HEIGHT_SETTING_ENABLED) && defined(CUSTOM_PARTITION_PRINT_ENABLED)
        registerContextObject("cf_slice", m_cSlice);
#endif
        m_slicePreviewFlow->registerContext();
    }

    void SliceFlow::initialize()
    {
        m_slicePreviewFlow->initialize();
        cxkernel::registerOpenHandler(m_previewWorker);
        registerRenderGraph(m_slicePreviewFlow);

        CCommandList::getInstance()->addActionCommad(new UsbPrintCommand(this), "");
    }

    void SliceFlow::uninitialize()
    {
        m_slicePreviewFlow->setSliceAttain(nullptr);
        cxkernel::unRegisterOpenHandler(m_previewWorker);
    }

    void SliceFlow::onSliceMessage(const QString& message) {
        if (QStringLiteral("need_support_structure") == message) {
            Q_EMIT supportStructureRequired();
        }
    }

    void SliceFlow::onSliceSuccess(SliceAttain* attain)
    {
        qDebug() << QString("Slice : SliceAttain before geometry . [%1]").arg(currentProcessMemory());
        m_slicePreviewFlow->setSliceAttain(attain);
        qDebug() << QString("Slice : SliceAttain after geometry . [%1]").arg(currentProcessMemory());

        if (attain)
        {
            auto imageCallback = [this](const QImage& image)
            {
                SliceAttain* attain = m_slicePreviewFlow->attain();

                if (attain)
                {
#if _DEBUG
                    image.save(attain->tempImageFileName());
#endif
                    attain->saveTempGCode();

                    QImage tmpImage = image;
                    attain->saveTempGCodeWithImage(tmpImage);
                }
                /*m_slicePreviewFlow->requestPreview(nullptr);
                qDebug() << QString("SliceFlow : requestPreview Success.");*/

                m_slicePreviewFlow->endRequest();
                QMetaObject::invokeMethod(this, "saveTempGCodeSuccess");
                setCommandRender();
            };

            preview();
            m_slicePreviewFlow->requestPreview(imageCallback);
            qDebug() << QString("SliceFlow : requestPreview.");

            setKernelPhase(KernelPhaseType::kpt_preview);
        }

        qDebug() << QString("Slice : slice success. [%1]").arg(currentProcessMemory());
    }

    void SliceFlow::saveTempGCodeSuccess()
    {
        if (m_slicePreviewFlow->attain())
        {
            emit sliceAttainChanged();
        }
    }

    void SliceFlow::onSliceFailed()
    {
        qDebug() << QString("Slice : slice failed. [%1]").arg(currentProcessMemory());
        setKernelPhase(KernelPhaseType::kpt_prepare);
    }

    void SliceFlow::handle(const QStringList& fileNames)
    {
    }

    QString SliceFlow::filter()
    {
        QString _filter = "GCode File (*.gcode)";
        return _filter;
    }

    void SliceFlow::handle(const QString& fileName)
    {
        if (m_slicePreviewFlow->attain())
        {
            QString fileName_Utf8 = fileName.toUtf8();
            QString sourceFile = currentGCodeImageFileName();
            if (!currentMachineSupportExportImage())
                sourceFile = currentGCodeFileName();

            if(qtuser_core::copyFileToPath(sourceFile, fileName_Utf8))
            {
                m_strMessageText = tr("Save Finished, Open Local Folder");
                requestQmlDialog(this, "messageDlg");
            }
        }
    }

    QString SliceFlow::gcodeFileName()
    {
        QString name;
        if (m_slicePreviewFlow->attain())
            name = m_slicePreviewFlow->attain()->sliceName();
        return name;
    }

    QString SliceFlow::gcodeThumbnail()
    {
        QString name;
        if (m_slicePreviewFlow->attain())
            name = m_slicePreviewFlow->attain()->tempGcodeThumbnail();
        return name;
    }

    QString SliceFlow::currentGCodeFileName()
    {
        QString name;
        if (m_slicePreviewFlow->attain())
            name = m_slicePreviewFlow->attain()->tempGCodeFileName();
        return name;
    }

    QString SliceFlow::currentGCodeImageFileName()
    {
        QString name;
        if (m_slicePreviewFlow->attain())
            name = m_slicePreviewFlow->attain()->tempGCodeImageFileName();
        return name;
    }

    void SliceFlow::accept()
    {
        cxkernel::openLastSaveFolder();
    }

    void SliceFlow::cancel()
    {

    }

    QString SliceFlow::getMessageText()
    {
        return m_strMessageText;
    }

    QString SliceFlow::saveCurrentGcodeToDir(const QString& dir_path) {
        auto file_path = QStringLiteral("%1/%2.gcode")
            .arg(dir_path).arg(QUuid::createUuid().toString(QUuid::StringFormat::Id128));

        if (!saveCurrentGcodeAsFile(file_path)) {
            file_path.clear();
        }

        return file_path;
    }

    bool SliceFlow::saveCurrentGcodeAsFile(const QString& file_path) {
        SliceAttain* attain = m_slicePreviewFlow->attain();
        if (!attain) {
            return false;
        }

        attain->saveGCode(file_path, nullptr);
        return true;
    }

    void SliceFlow::saveGCodeLocal()
    {
        cxkernel::saveFile(this, getExportName() + ".gcode");
        sensorAnlyticsTrace("G-code Import", "G-code Import");
    }

    QString SliceFlow::getExportName()
    {
        QString name = "temp";
        SliceAttain* attain = m_slicePreviewFlow->attain();
        if (attain)
        {
            if (attain->isFromFile())
            {
                return attain->fileNameFromGcode();
            }

			bool fromCloud = attain->sourceFileName().contains("myGcodes");
			if (fromCloud)
			{
				name = attain->sliceName();
			}
			else {
				name = QString("%1_%2").arg(mainModelName()).arg(currentMachineName());
				foreach(QString material, currentMaterials())
				{
					name += QString("_%1").arg(material);
				}
				int printTime = attain->printTime();
				name += printTime < 3600 ? QString("_%1m").arg((int)printTime / 60 % 60) : QString("_%1h%2m").arg((int)printTime / 3600).arg((int)printTime / 60 % 60);
			}
        }
        return name;
    }

    void SliceFlow::onStartPhase()
    {
        //preview();
    }

    void SliceFlow::onStopPhase()
    {
        setCommandRender();
    }

    void SliceFlow::slice()
    {
        sensorAnlyticsTrace("Slice", "Start Slicing");

        qDebug() << QString("Slice : start slicing. [%1]").arg(currentProcessMemory());
        QSharedPointer<AnsycWorker> worker(new AnsycWorker());
        worker->setRemainAttain(m_slicePreviewFlow->takeAttain());
        connect(worker.data(), &AnsycWorker::sliceMessage, this, &SliceFlow::onSliceMessage);
        connect(worker.data(), &AnsycWorker::sliceSuccess, this, &SliceFlow::onSliceSuccess);
        connect(worker.data(), &AnsycWorker::sliceFailed , this, &SliceFlow::onSliceFailed );

        cxkernel::executeJob(worker);

        setContinousRender();
    }

#if defined(CUSTOM_SLICE_HEIGHT_SETTING_ENABLED) && defined(CUSTOM_PARTITION_PRINT_ENABLED)
    void SliceFlow::sliceChengfei()
    {
        sensorAnlyticsTrace("Slice", "Start Slicing");

        qDebug() << QString("Slice : start slicing. [%1]").arg(currentProcessMemory());
        QSharedPointer<AnsycWorkerCF> worker(new AnsycWorkerCF(m_cSlice->getChengfeiSplit()));
        worker->setRemainAttain(m_slicePreviewFlow->takeAttain());
        connect(worker.data(), SIGNAL(sliceSuccess(SliceAttain*)), this, SLOT(onSliceSuccess(SliceAttain*)));
        connect(worker.data(), SIGNAL(sliceFailed()), this, SLOT(onSliceFailed()));

        cxkernel::executeJob(worker);
    }
#endif

    void SliceFlow::preview()
    {
        renderRenderGraph(m_slicePreviewFlow);
    }

    QObject* SliceFlow::sliceAttain()
    {
        return m_slicePreviewFlow->attain();
    }

	void SliceFlow::loadGCode(const QString& fileName)
	{
		m_previewWorker->handle(fileName);
		return;
	}

}
