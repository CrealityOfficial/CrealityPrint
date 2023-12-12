#ifndef _NULLSPACE_SLICEPLUGIN_1590155449305_H
#define _NULLSPACE_SLICEPLUGIN_1590155449305_H

#include <buildinfo.h>

#include "data/interface.h"
#include "qtusercore/module/cxhandlerbase.h"

namespace creative_kernel
{
	class Calibrate;
    class PreviewWorker;
	class SliceAttain;
	class SlicePreviewFlow;
    class AnsycWorker;
#if defined(CUSTOM_SLICE_HEIGHT_SETTING_ENABLED) && defined(CUSTOM_PARTITION_PRINT_ENABLED)
    class ChengFeiSlice;
#endif
    class BASIC_KERNEL_API SliceFlow : public QObject, public qtuser_core::CXHandleBase, public KernelPhase
    {
        Q_OBJECT
        Q_PROPERTY(QObject* sliceAttain READ sliceAttain NOTIFY sliceAttainChanged)
    public:
        SliceFlow(QObject* parent = nullptr);
        virtual ~SliceFlow();

        void registerContext();

        void initialize();
        void uninitialize();
        QObject* sliceAttain();
		void loadGCode(const QString& fileName);

        Q_INVOKABLE void slice();
#if defined(CUSTOM_SLICE_HEIGHT_SETTING_ENABLED) && defined(CUSTOM_PARTITION_PRINT_ENABLED)
        Q_INVOKABLE void sliceChengfei();
#endif
        Q_INVOKABLE void saveGCodeLocal();
        Q_INVOKABLE QString getExportName();

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

        Q_SIGNAL void supportStructureRequired();

        SlicePreviewFlow* slicePreviewFlow();

    protected:
        QString filter() override;
        void onStopPhase() override;
        void onStartPhase() override;
        void handle(const QString& fileName) override;
        void handle(const QStringList& fileNames) override;

        void preview();
    signals:
        void sliceAttainChanged();

    public slots:
        void onSliceMessage(const QString& message);
        void onSliceSuccess(SliceAttain* attain, bool isRemain = true);
        void onSliceBeforeSuccess(SliceAttain* attain);
        void onSliceFailed();
        void saveTempGCodeSuccess();

        void gcodeLayerChanged(int layer);

    protected:
        PreviewWorker* m_previewWorker;
        SlicePreviewFlow* m_slicePreviewFlow;
        QString m_strMessageText;
		Calibrate* m_calibrate;
        QSharedPointer<AnsycWorker> m_worker;


#if defined(CUSTOM_SLICE_HEIGHT_SETTING_ENABLED) && defined(CUSTOM_PARTITION_PRINT_ENABLED)
        ChengFeiSlice* m_cSlice;
#endif
    };
}
#endif // _NULLSPACE_SLICEPLUGIN_1590155449305_H
