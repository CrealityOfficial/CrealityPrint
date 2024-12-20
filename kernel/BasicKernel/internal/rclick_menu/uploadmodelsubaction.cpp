#include "uploadmodelsubaction.h"

#include "interface/selectorinterface.h"
#include "interface/commandinterface.h"
#include "interface/cloudinterface.h"
#include "interface/uiinterface.h"
#include "kernel/kernelui.h"
#include "cxkernel/interface/iointerface.h"

namespace creative_kernel
{
    UploadModelSubAction::UploadModelSubAction(UpType type, const QString actionName, QObject* parent)
        : ActionCommand(parent), m_Type(type)
    {
        m_actionname = actionName;// tr("Upload Select Model");
        m_actionNameWithout = "Upload Select Model";
        m_bCheckable = false;
        addUIVisualTracer(this,this);
    }

    UploadModelSubAction::~UploadModelSubAction()
    {
    }

    void UploadModelSubAction::execute()
    {

        auto f = [this](const QStringList& files) {
            //openWithNames(files);
        };

        /*QString filter = csm.generateFilterFromHandlers(false);
        csm.dialogOpenFiles(filter, f);*/

        if (IsCloudLogined())
        {
          getKernelUI()->invokeModelUploadDialogFunc("upload",
                                                     QVariant::fromValue(m_Type == UPLOADLOCAL));
        }
        else
        {
            requestQmlDialog("idWarningLoginDlg");
        }
    }

    bool UploadModelSubAction::enabled()
    {
        if (m_Type == UPLOADLOCAL)
        {//
            return true;
        }
        else
        {//
            bool res = selectionms().size() > 0;
            return res;
        }
    }

    void UploadModelSubAction::onThemeChanged(ThemeCategory category)
    {
    }

    void UploadModelSubAction::onLanguageChanged(MultiLanguage language)
    {
        //m_actionname = tr("Upload Select Model");
    }

    QString UploadModelSubAction::filter()
    {
        QString _filter = "Mesh File (*.stl *.obj *.dae *.3mf *.3ds *.wrl *.off *.ply)";
        return _filter;
    }

    void UploadModelSubAction::handle(const QStringList& fileNames)
    {
        // uploadLocalModels(fileNames);
    }
}
