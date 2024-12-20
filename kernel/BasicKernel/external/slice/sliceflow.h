#ifndef _NULLSPACE_SLICEPLUGIN_1590155449305_H
#define _NULLSPACE_SLICEPLUGIN_1590155449305_H

#include <buildinfo.h>

#include "data/interface.h"
#include "qtusercore/module/cxhandlerbase.h"
#include <QImage>
#include <QAbstractListModel>

namespace creative_kernel
{
    class Printer;
    class CapturePreviewWorker;
	class Calibrate;
    class PreviewWorker;
	class SliceAttain;
	class SlicePreviewFlow;
    class AnsycWorker;
    class SlicePreviewListModel;
#if defined(CUSTOM_SLICE_HEIGHT_SETTING_ENABLED) && defined(CUSTOM_PARTITION_PRINT_ENABLED)
    class ChengFeiSlice;
#endif
    class BASIC_KERNEL_API SliceFlow : public QObject, public qtuser_core::CXHandleBase, public KernelPhase, public SpaceTracer
    {
        Q_OBJECT
        Q_PROPERTY(QObject* sliceAttain READ sliceAttain NOTIFY sliceAttainChanged)
        Q_PROPERTY(QObject* cacheSliceAttain READ cacheSliceAttain CONSTANT)
        Q_PROPERTY(QAbstractListModel* slicePreviewListModel READ slicePreviewListModel NOTIFY slicePreviewListModelChanged)
        Q_PROPERTY(int sliceIndex READ sliceIndex WRITE setSliceIndex NOTIFY sliceIndexChanged)
        Q_PROPERTY(float sliceCompletionRate READ sliceCompletionRate NOTIFY sliceCompletionRateChanged)
        Q_PROPERTY(int engineType READ getEngineTypeIndex CONSTANT)
        Q_PROPERTY(bool printerVisible READ printerVisible WRITE setPrinterVisible NOTIFY printerVisibleChanged)
        Q_PROPERTY(bool isSlicing READ isSlicing WRITE setSliceState NOTIFY sliceStateChanged)
        Q_PROPERTY(bool isPreviewGCodeFile READ isPreviewGCodeFile NOTIFY startActionChanged)
        Q_PROPERTY(QStringList modelInsideColors READ getModelInsideColors NOTIFY modelInsideColorsChanged)

    public:
        enum StartAction 
        {
            SliceSingle = 0,
            SliceAll,
            PreviewGCodeFile
        };

        SliceFlow(QObject* parent = nullptr);
        virtual ~SliceFlow();

        void registerContext();
        Q_INVOKABLE QStringList getModelInsideUseColors();
        Q_INVOKABLE QStringList getModelInsideColors();
        Q_INVOKABLE QString getTypeByColorIndex(int index);
        void initialize();
        void uninitialize();
        QObject* sliceAttain();
        QObject* cacheSliceAttain();
		void loadGCode(const QString& fileName);

        Q_INVOKABLE void sliceAll();
        Q_INVOKABLE void slice(int index);
        void slice(Printer* printer);
        void slice(QList<Printer*> printers);
#if defined(CUSTOM_SLICE_HEIGHT_SETTING_ENABLED) && defined(CUSTOM_PARTITION_PRINT_ENABLED)
        Q_INVOKABLE void sliceChengfei();
#endif
        Q_INVOKABLE void saveGCodeLocal();
        Q_INVOKABLE void saveGCodeLocal(const QUrl& path);
        Q_INVOKABLE QString getExportName();
        Q_INVOKABLE QString get3MfExportName();
        Q_INVOKABLE void setExportName(QString exportName,int type);
        Q_INVOKABLE void set3mfExportName(QString exportName);
        /// @return saved file path if successed, or empty
        QString saveCurrentGcodeToDir(const QString& dir_path);
        bool saveCurrentGcodeAsFile(const QString& file_path);

        Q_INVOKABLE QString gcodeFileName();
        Q_INVOKABLE QString gcodeThumbnail();
        Q_INVOKABLE QString currentGCodeFileName();
        Q_INVOKABLE QString currentGCodeImageFileName();

        Q_INVOKABLE void accept();
        Q_INVOKABLE void cancel();
        Q_INVOKABLE QString getMessageText();

        Q_INVOKABLE void setStartAction(int action);
        bool isPreviewGCodeFile();

        float sliceCompletionRate();

        Q_SIGNAL void supportStructureRequired();

        SlicePreviewFlow* slicePreviewFlow();

        bool printerVisible();
        void setPrinterVisible(bool visible);

        void prepareGcodePreview();
    public:
        enum class EngineType : int {
            CURA = 0,
            OCRA = 1,
        };

        auto getEngineType() const -> EngineType;
        auto getEngineTypeIndex() const -> int;
        Q_INVOKABLE void setEngineTypeAndRestart(int type_index);
        Q_INVOKABLE void slicerCacheToGcode(const QString& cacheFile, const QString& outGCode);
    protected:
        QString filter() override;
        void onStopPhase() override;
        void onStartPhase() override;
        void handle(const QString& fileName) override;
        void handle(const QStringList& fileNames) override;

        void onModelAdded(ModelN* model) override;

        void preview();
        QAbstractListModel* slicePreviewListModel();

        void setSliceIndex(int index);
        int sliceIndex();

        bool isSlicing();
        void setSliceState(bool isSlicing); 

        void loadPrinter();
        void loadPrinter(int index);
        
        void focusPrinter(int index);
        void focusPrinter(Printer* printer);

        void updateSliceCompletionRate();

        void applyExtruderOffset();
        void resetExtruderOffset();

        void failReasonHandler(const QString& failReason);

        //void waitForCaptrue();
        void updateModelnsRenderType();

    signals:
        void sliceAttainChanged();
        void slicePreviewListModelChanged();
        void sliceIndexChanged();
        void sliceCompletionRateChanged();
        void printerVisibleChanged();
        void sliceStateChanged(); 
        void startActionChanged();
        void modelInsideColorsChanged();

    public slots:
        /* 切片 */
        void onPrepareSlice(Printer* printer);
        void onSliceMessage(const QString& message);
        void onSliceSuccess(SliceAttain* attain, Printer* printer);
        void onSliceFailed(const QString& failReason);
        void onParseGcodeSuccuess(SliceAttain* attain);
        void saveTempGCodeSuccess();

        void gcodeLayerChanged(SliceAttain* attain, int layer);

        void onLayerChanged();
        void onPrinterAttainChanged();

        Q_INVOKABLE void onPictureUpdated();


    protected:
        QList<bool> m_modelsState;

        PreviewWorker* m_previewWorker;
        QString m_strMessageText;
		Calibrate* m_calibrate;
        QSharedPointer<AnsycWorker> m_worker;
        QSharedPointer<CapturePreviewWorker> m_captureWorker;
        QImage m_previewImage;
        QScopedPointer<SliceAttain> m_singleSliceAttain;

        SlicePreviewFlow* m_slicePreviewFlow;

        SlicePreviewListModel* m_slicePreviewListModel;
        int m_sliceIndex { -1 };
        float m_sliceCompletionRate;

        bool m_isSlicing { false };
        int m_sliceCount { 0 };

        const EngineType engine_type_{ EngineType::CURA };

        int m_autoSliceFlag { 0 }; // 0:截图后自动单盘切片，1:截图后自动全盘切片

        bool m_printerVisible { true };

        int m_startAction { 0 };
        bool m_needOffscreenRender { false };

        bool m_requestSlice{ false };
        QString m_exportName;
        QString m_3mfExportName;
#if defined(CUSTOM_SLICE_HEIGHT_SETTING_ENABLED) && defined(CUSTOM_PARTITION_PRINT_ENABLED)
        ChengFeiSlice* m_cSlice;
#endif
    };
}
#endif // _NULLSPACE_SLICEPLUGIN_1590155449305_H
