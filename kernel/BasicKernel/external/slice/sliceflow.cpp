#include "sliceflow.h"
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QUuid>
#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>

#include "qtusercore/string/resourcesfinder.h"
#include "qtusercore/module/systemutil.h"
#include <qtusercore/util/settings.h>

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
#include "interface/printerinterface.h"
#include "interface/reuseableinterface.h"

#include "interface/commandinterface.h"
#include "interface/appsettinginterface.h"
#include "kernel/kernelui.h"

#include "internal/multi_printer/printer.h"
#include "internal/multi_printer/printermanager.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/printmachine.h"

#include "slice/calibrate.h"
#include "cxgcode/gcodeloadjob.h"
#include "calibratescenecreator.h"

#include "kernel/kernel.h"
#include <cxcloud/service_center.h>
#include "usbprintcommand.h"
#include "qtuser3d/camera/cameracontroller.h"
#include "interface/camerainterface.h"
#include "external/message/sliceenginefailmessage.h"
#include "external/message/previewemptymessage.h"
#include "internal/parameter/parametermanager.h"
#include "unittest/unittestflow.h"
#if defined(CUSTOM_SLICE_HEIGHT_SETTING_ENABLED) && defined(CUSTOM_PARTITION_PRINT_ENABLED)
//chengfei 定制切片
#include "chengfei/chengfeislice.h"
#include "chengfei/ansycworkerchengfei.h"
#endif

#include <menu/ccommandlist.h>

#include "slicepreviewlistmodel.h"

namespace creative_kernel
{
    SliceFlow::SliceFlow(QObject* parent)
        : QObject(parent)
        , engine_type_{ [] {
            qtuser_core::VersionSettings settings{};
            return static_cast<EngineType>(settings.value("engine_type", 0).toInt());
          }() }
    {
        m_slicePreviewFlow = new SlicePreviewFlow();
        m_previewWorker = new PreviewWorker(this);
		m_calibrate = new Calibrate(this);
        m_slicePreviewListModel = new SlicePreviewListModel(this);
        m_slicePreviewListModel->setCaptureFlow(m_slicePreviewFlow);

        connect(m_slicePreviewListModel, &SlicePreviewListModel::dataChanged, this, &SliceFlow::slicePreviewListModelChanged);

        auto printerManager = getPrinterManager();
        connect(printerManager, &PrinterMangager::signalDidSelectPrinter, this, [=] ()
        {
            setSliceIndex(getSelectedPrinter()->index());
        });


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

    void SliceFlow::onCaptureFinished()
    {
        m_isCaptured = true;
        updateModelnsRenderType(false);
        if (m_requestSlice)
        {
            // 执行切片
            if (m_startAction == SliceSingle)
            {
                Printer* printer = getSelectedPrinter();
                int i = printer->index();
                if (m_sliceIndex == i)
                {
                    m_slicePreviewFlow->setPrinter(printer);
                    loadPrinter(i);
                }
                else
                {
                    setSliceIndex(i);
                }

                if (!printer->isSliced())
                {
                    //m_slicePreviewFlow->setSliceAttain(NULL);
                    //emit sliceAttainChanged();
                    //m_sliceIndex = -1;
                    QList<Printer*> slicePrinters;
                    slicePrinters << printer;
                    slice(printer);
                }
            }
            else if (m_startAction == SliceAll)
            {
                Printer* printer = getSelectedPrinter();
                int i = printer->index();
                if (printer->isSliced())
                {
                    if (m_sliceIndex == i)
                    {
                        m_slicePreviewFlow->setPrinter(printer);
                        loadPrinter(i);
                    }
                    else
                    {
                        setSliceIndex(i);
                    }
                }
                sliceAll();
            }
            m_requestSlice = false;
        }
    }

    void SliceFlow::onPrepareSlice(Printer* printer)
    {
        setSliceIndex(printer->index());
        m_slicePreviewFlow->setSliceAttain(NULL);
        emit sliceAttainChanged();
        printer->setAttain(NULL);
        printer->clearSliceError();

        //waitForCaptrue(); //截图完成前不允许切片

        updateModelnsRenderType(false);

		setContinousRender();
    }

    void SliceFlow::onSliceMessage(const QString& message) {
        if (QStringLiteral("need_support_structure") == message) {
            Q_EMIT supportStructureRequired();
        }
    }

    void SliceFlow::onSliceSuccess(SliceAttain* attain, Printer* printer)
    {
        if (BLCompareTestFlow::enabled())
        {
            BLCompareTestFlow::nextBLTest();
            return;
        }

        int index = printer->index();
        m_slicePreviewListModel->update();

        QImage image = m_slicePreviewListModel->picture(index);
#if _DEBUG
        image.save(attain->tempImageFileName());
#endif
        attain->saveTempGCode();

        QImage tmpImage = image;
        attain->saveTempGCodeWithImage(tmpImage);

        m_sliceCount--;
        if (m_sliceCount == 0)
        {
            setSliceState(false);
            m_slicePreviewFlow->setSliceAttain(attain);
            emit sliceAttainChanged();
        }

        updateSliceCompletionRate();
        setCommandRender();
        QTimer::singleShot(200, [=] ()
        {
		    renderOneFrame();
        });

        
    }
    
    void SliceFlow::onGotSliceAttain(SliceAttain* attain)
    {
		setKernelPhase(KernelPhaseType::kpt_preview);

        m_singleSliceAttain.reset(attain);
        attain->saveTempGCode();
        QImage tmpImage;
        attain->saveTempGCodeWithImage(tmpImage);
        m_slicePreviewFlow->setSliceAttain(attain);

        saveTempGCodeSuccess();
    }

    void SliceFlow::saveTempGCodeSuccess()
    {
        if (m_slicePreviewFlow->attain())
        {
            emit sliceAttainChanged();
        }
    }

    void SliceFlow::onSliceFailed(const QString& failReason)
    {
        if (!m_worker.isNull())
        {
            disconnect(m_worker.data(), &AnsycWorker::gcodeLayerChanged, this, &SliceFlow::gcodeLayerChanged);
        }

        m_slicePreviewFlow->setSliceAttain(NULL);

        if (m_slicePreviewListModel->rowCount() <= 1)
        {
            qDebug() << QString("Slice : slice failed. [%1]").arg(currentProcessMemory());
            setKernelPhase(KernelPhaseType::kpt_prepare);
        }

        failReasonHandler(failReason);

        m_sliceCount--;
        setSliceState(false);
    }


    void SliceFlow::gcodeLayerChanged(SliceAttain* attain, int layer)
    {
        qDebug() << "SliceFlow gcodeLayerChanged thread " << QThread::currentThread() << "layer =" << layer;

        preview();
        setKernelPhase(KernelPhaseType::kpt_preview);

        m_slicePreviewFlow->previewSliceAttain(attain, layer);
        renderOneFrame();
    }
    
    void SliceFlow::updateModelnsRenderType(bool needOffscreenRender)
    {
        if (needOffscreenRender)
        {
            auto outsideModels = getAllModelsOutOfPrinter();
            for (ModelN* m : modelns())
            {
                if (!m)
                    continue;

                if (outsideModels.contains(m))
                {
                    m->setRenderType(ModelN::NoRender); 
                }
                else 
                {
                    m->setRenderType(ModelN::OffscreenRender);
                }
            }
        } 
        else 
        {
            //waitForCaptrue();   // 截图过程中不可以关闭渲染
            for (ModelN* m : modelns())
            {
                if (m)
                    m->setRenderType(ModelN::NoRender); 
            }
        }

        if (!m_slicePreviewFlow->isAttainDisplayCompletly())
        {   /* 无切片结果或切片结果未显示完全时显示虚影 */
            Printer* p = getSelectedPrinter();
            if (!p)
                return;

            for (ModelN* m : p->modelsInside())
            {
                if (m)
                    m->setRenderType(ModelN::OffscreenRender | ModelN::ViewRender);
            }
        }
    }
    
    void SliceFlow::onLayerChanged()
    {
        if (isRenderRenderGraph(m_slicePreviewFlow))
         updateModelnsRenderType(false);
    }
    
    void SliceFlow::onPrinterDirtyChanged()
    {
        Printer* printer = dynamic_cast<Printer*>(sender());
        if (!printer)
            return;

        if (printer->isDirty())
        {
            qDebug() << "update picture " << printer->index();
            updatePicture(printer->index());
        }
    }

    void SliceFlow::onPrinterAttainChanged()
    {
        Printer* printer = dynamic_cast<Printer*>(sender());
        if (!printer)
            return;

        if (printer->isDirty() && m_sliceIndex == printer->index())
        {
            //qDebug() << "onPrinterAttainChanged set sliceCache";
            SliceAttain* attain = dynamic_cast<SliceAttain*>(printer->sliceCache());
            if (m_slicePreviewFlow->attain() != attain)
            {
                m_slicePreviewFlow->setSliceAttain(attain);
                emit sliceAttainChanged();

                if (isRenderRenderGraph(m_slicePreviewFlow))
                    updateModelnsRenderType(false);
            }
        }
    }

    void SliceFlow::handle(const QStringList& fileNames)
    {
    }

    void SliceFlow::onModelAdded(ModelN* model) 
    {
        setKernelPhase(KernelPhaseType::kpt_prepare);
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
            //else
            //{
            //    //todo : 
            //    sourceFile = QString("%1/temp.gcode").arg(SLICE_PATH);
            //    qtuser_core::copyFileToPath(sourceFile, fileName_Utf8);
            //}
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
            name = m_slicePreviewFlow->attain()->tempGCodeFileName();
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

    QAbstractListModel* SliceFlow::slicePreviewListModel()
    {
        return m_slicePreviewListModel;
    }

    void SliceFlow::setSliceIndex(int index)
    {
        if (isRenderRenderGraph(m_slicePreviewFlow))
        {
            if (index != m_sliceIndex)
            {
                Printer* printer = getPrinter(m_sliceIndex);
                if (printer)
                   disconnect(printer, &Printer::signalAttainChanged, this, &SliceFlow::onPrinterAttainChanged);

                m_sliceIndex = index;
                
                focusPrinter(index);
                loadPrinter();
                emit sliceIndexChanged(); 

                printer = getPrinter(m_sliceIndex);
                if (printer)
                    connect(printer, &Printer::signalAttainChanged, this, &SliceFlow::onPrinterAttainChanged, Qt::UniqueConnection);
            }
        }
        else 
        {
            if (index != m_sliceIndex)
            {
                Printer* printer = getPrinter(m_sliceIndex);
                if (printer)
                   disconnect(printer, &Printer::signalAttainChanged, this, &SliceFlow::onPrinterAttainChanged);

                m_sliceIndex = index;
                SliceAttain* attain = m_slicePreviewListModel->attain(index);
                m_slicePreviewFlow->setSliceAttain(attain);

                emit sliceAttainChanged();
                emit sliceIndexChanged();

                printer = getPrinter(m_sliceIndex);
                if (printer)
                    connect(printer, &Printer::signalAttainChanged, this, &SliceFlow::onPrinterAttainChanged, Qt::UniqueConnection);
            }
        }

    }

    int SliceFlow::sliceIndex()
    {
        return m_sliceIndex;
    }
    
    bool SliceFlow::isSlicing()
    {
        return m_isSlicing;
    }

    void SliceFlow::setSliceState(bool isSlicing)
    {
        m_isSlicing = isSlicing;
        emit sliceStateChanged();
    }
    
    void SliceFlow::setStartAction(int action)
    {
        m_startAction = action;
        emit startActionChanged();
    }   

    bool SliceFlow::isPreviewGCodeFile()
    {
        return m_startAction == PreviewGCodeFile;
    }
    
    float SliceFlow::sliceCompletionRate()
    {
        return m_sliceCompletionRate;
    }
    
    void SliceFlow::loadPrinter()
    {
        loadPrinter(m_sliceIndex);
    }

    void SliceFlow::loadPrinter(int index)
    {
        Printer* printer = getPrinter(index);
        if (printer->checkVisiableModelError())
        {
            m_slicePreviewFlow->setSliceAttain(NULL);
        }
        else 
        {
            SliceAttain* attain = (SliceAttain*)printer->attain();
            m_slicePreviewFlow->setSliceAttain(attain);
            if (attain)
            {
                QImage image = printer->picture().copy();
                attain->saveTempGCodeWithImage(image);
            }
        }

        if (isRenderRenderGraph(m_slicePreviewFlow))
            updateModelnsRenderType(false);

        emit sliceAttainChanged();
    }

    void SliceFlow::focusPrinter(int index)
    {
        focusPrinter(getPrinter(index));
    }

    void SliceFlow::focusPrinter(Printer* printer)
    {
        if (printer == NULL)
            return;

        qtuser_3d::Box3D box = printer->globalBox();
        QVector3D dir(0.0f, 1.0f, -1.0f);
        QVector3D right(1.0f, 0.0f, 0.0f);
        QVector3D center = box.center();
        center.setZ(0);
        cameraController()->view(dir, right, &center);

        hidePrinter();
        if (m_printerVisible)
            showPrinter(printer->index());
        selectPrinter(printer->index());
        m_slicePreviewFlow->setPrinter(printer);
    }
    
    void SliceFlow::updateSliceCompletionRate()
    {
        float count = m_slicePreviewListModel->rowCount();
        float slicedCount = 0;
        for (int i = 0; i < count; ++i)
        {
            if (m_slicePreviewListModel->isSliced(i))
                slicedCount++;
        }
        float rate = slicedCount / count;

        m_sliceCompletionRate = rate;
        emit sliceCompletionRateChanged();
    }

    void SliceFlow::applyExtruderOffset()
    {
		auto parameterManager = getKernel()->parameterManager();
		QString offset = parameterManager->currentMachine()->settingsValue("extruder_offset");
        QStringList offsetSplit = offset.split("x");
        if (offsetSplit.count() != 2)
            return;
        
        setModelOffset(QVector2D(offsetSplit[0].toInt(), offsetSplit[1].toInt()));
    }

    void SliceFlow::resetExtruderOffset()
    {
        setModelOffset(QVector2D(0, 0));
    }

    void SliceFlow::failReasonHandler(const QString& failReason)
    {
        getSelectedPrinter()->setSliceError(failReason);
    }

    void SliceFlow::updatePicture(int index)
    {
        m_isCaptured = false;
        updateModelnsRenderType(true);
        m_slicePreviewListModel->updatePicture(index);
    }

    void SliceFlow::updateAllPicture()
    {
        updatePicture(-1);
    }

    //void SliceFlow::waitForCaptrue()
    //{
    //    while (!m_isCaptured)
    //    {
    //        QThread::msleep(40);
    //        renderOneFrame();
    //        // QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    //    }
    //}

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
        QString exportName = "temp";
        SliceAttain* attain = m_slicePreviewFlow->attain();
        if (attain)
        {
            if (attain->isFromFile())
            {
                return attain->fileNameFromGcode();
            }

			bool fromGcode = attain->sourceFileName().endsWith(".gcode");
			if (fromGcode)
			{
                exportName = attain->sliceName();
			}
			else {
                Printer* printer = getSelectedPrinter(); 
                cxgcode::GcodeExtruderTableModel* m = (cxgcode::GcodeExtruderTableModel*)m_slicePreviewFlow->getExtruderTableModel();
                int useMaterialIndex = m->getFirstItemMaterialIndex();

                QString name = (printer && printer->modelsInsideVisible().size() > 0) ? QFileInfo(printer->modelsInsideVisible().first()->objectName()).completeBaseName() : attain->sliceName();
                QString gcodeName = name.size() >= 7 ? name.left(7) : name;
                QString suffix = (QString("_%1").arg(currentMaterialsType(useMaterialIndex))).left(5);
                suffix = suffix.mid(0, suffix.lastIndexOf("-"));
                int printTime = attain->printTime();
                suffix += printTime < 3600 ? QString("_%1m").arg((int)printTime / 60 % 60) : QString("_%1h%2m").arg((int)printTime / 3600).arg((int)printTime / 60 % 60);
                QString combinedName = gcodeName + suffix;
                if (combinedName.length() > 200) {
                    QString  truncatedFileName = gcodeName.left(200 - suffix.length() + 1);
                    combinedName = truncatedFileName + suffix;
                }
                exportName = combinedName;
			}
        }
        return exportName;
    }

    void SliceFlow::onStartPhase()
    {
        /* 通用初始化 */
        m_isCaptured = true;
        renderRenderGraph(m_slicePreviewFlow);
        setPrinterPreviewMode();    // 切换盘样式
        applyExtruderOffset();      // 模型跟随挤出机偏移，为了模型虚影与切片结果对齐
        traceSpace(this);
        
        // 切换为离屏渲染模式
        for (ModelN* model : modelns())
        {
            // model->setRenderType(ModelN::NoRender);
            model->setBoxVisibility(false);
        }

        /* 切片预览模式和G-code文件预览模式初始化 */
        if (!isPreviewGCodeFile())
        {   /* 预览、单盘切片、所有盘切片 */

            /* 基本初始化 */ 
            hidePrinter();
            Printer* printer = getSelectedPrinter();
            int i = printer->index();
            showPrinter(i);

            if (modelns().isEmpty())
            {
                PreviewEmptyMessage* message = new PreviewEmptyMessage();
                getKernelUI()->requestMessage(message);
            }

            // 信号绑定
            auto parameterManager = getKernel()->parameterManager();
            connect(parameterManager, &ParameterManager::currentColorsChanged, this, &SliceFlow::updateAllPicture);

            connect(m_slicePreviewListModel, &SlicePreviewListModel::updatePictureFinished, this, &SliceFlow::onCaptureFinished, Qt::UniqueConnection);
            connect(m_slicePreviewFlow, &SlicePreviewFlow::stepsChanged, this, &SliceFlow::onLayerChanged, Qt::UniqueConnection);
            for (int i = 0, count = printersCount(); i < count; ++i)
                connect(getPrinter(i), &Printer::signalDirtyChanged, this, &SliceFlow::onPrinterDirtyChanged, Qt::UniqueConnection);

            // // 还原层配置
            // QVariantList shaderDatas;
            // setPreviewEffectSpecificColorLayers(shaderDatas);
            
            m_requestSlice = true;
            // 更新预览图
            updateAllPicture();
            

        }
        else 
        {   /* 导入G-code文件 */
            focusPrinter(0);
        }
    }

    void SliceFlow::onStopPhase()
    {
        /* 同步当前盘的切片结果，这里主要是校准返回时的处理 */
        if (isPreviewGCodeFile())
            m_slicePreviewFlow->setSliceAttain((SliceAttain*)getSelectedPrinter()->attain());

        m_isCaptured = true;
        // disconnect(this, &SliceFlow::sliceAttainChanged, this, &SliceFlow::onSliceAttainChanged);
        auto parameterManager = getKernel()->parameterManager();
        disconnect(parameterManager, &ParameterManager::currentColorsChanged, this, &SliceFlow::updateAllPicture);
        
        /* 还原状态 */
        for (auto model : modelns())
        {
            model->setRenderType(ModelN::ViewRender);
            model->setBoxVisibility(true);
        }
        
        getKernelUI()->destroyMessage(MessageType::PreviewEmpty);
        
        for (int i = 0, count = printersCount(); i < count; ++i)
        {
            disconnect(getPrinter(i), &Printer::signalDirtyChanged, this, &SliceFlow::onPrinterDirtyChanged);
        }

        unTraceSpace(this);
        resetExtruderOffset();
        setStartAction(0);
        //m_slicePreviewFlow->setSliceAttain(NULL);
        showPrinter();
        setPrinterEditMode();
        setCommandRender();

    }

    void SliceFlow::sliceAll()
    {
        QList<Printer*> printers;
        for (int i = 0, count = printersCount(); i < count; ++i)
        {
            auto p = getPrinter(i);
            if (!p->isSliced())
                printers << p;
        }

        if (printers.count() == 0)
        {
            setSliceIndex(getSelectedPrinter()->index());
        } 
        else
        {
            slice(printers);
        }
    }

    void SliceFlow::slice(int index)
    {
        Printer* p = getPrinter(index);
        slice(p);
    }

    void SliceFlow::slice(Printer* printer)
    {
        if (printer == NULL)
            return;

        QList<Printer*> printers;
        printers << printer;
        slice(printers);
    }

    void SliceFlow::slice(QList<Printer*> printers)
    {
        m_sliceCount = 0;
        for (auto p : printers)
        {
            if (!p->isDirty())
                continue;

            if (p->checkVisiableModelError())
                continue;

            sensorAnlyticsTrace("Slice", "Start Slicing");

            setSliceState(true);
            QSharedPointer<AnsycWorker> worker(new AnsycWorker());
            worker->setSlicePrinter(p);
            worker->setRemainAttain(m_slicePreviewFlow->takeAttain()); 
            connect(worker.data(), &AnsycWorker::prepareSlice, this, &SliceFlow::onPrepareSlice, Qt::BlockingQueuedConnection);
            connect(worker.data(), &AnsycWorker::sliceMessage, this, &SliceFlow::onSliceMessage);
            connect(worker.data(), &AnsycWorker::sliceSuccess, this, &SliceFlow::onSliceSuccess);
            connect(worker.data(), &AnsycWorker::sliceFailed, this, &SliceFlow::onSliceFailed);
            connect(worker.data(), &AnsycWorker::gcodeLayerChanged, this, &SliceFlow::gcodeLayerChanged);

            m_sliceCount++;
            cxkernel::executeJob(worker);
        }

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

    SlicePreviewFlow* SliceFlow::slicePreviewFlow()
    {
        return m_slicePreviewFlow;
    }

    bool SliceFlow::printerVisible()
    {
        return m_printerVisible;
    }

    void SliceFlow::setPrinterVisible(bool visible)
    {
        if (m_printerVisible != visible)
        {
            m_printerVisible = visible;
            emit printerVisibleChanged();

            getSelectedPrinter()->setVisibility(m_printerVisible);
        }    
    }

    auto SliceFlow::getEngineType() const -> EngineType {
        return engine_type_;
    }

    auto SliceFlow::getEngineTypeIndex() const -> int {
        return static_cast<int>(engine_type_);
    }

    auto SliceFlow::setEngineTypeAndRestart(int type_index) -> void {
        if (getEngineTypeIndex() == type_index)  {
            return;
        }

        qtuser_core::VersionSettings{}.setValue("engine_type", type_index);

        auto arguments = QCoreApplication::arguments();
        const auto program = arguments.takeFirst();
        QCoreApplication::quit();
        QProcess::startDetached(program, arguments);
    }
}
