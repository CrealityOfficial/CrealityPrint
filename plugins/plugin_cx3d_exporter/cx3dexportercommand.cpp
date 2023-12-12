#include "cx3dexportercommand.h"
#include "cx3dexportjob.h"
#include "cx3dexporter.h"

#include "cxkernel/interface/jobsinterface.h"
#include "cx3dsubmenurecentproject.h"
#include "kernel/projectinfoui.h"
#include "kernel/kernelui.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"
#include "cxkernel/interface/iointerface.h"
#include "interface/spaceinterface.h"
#include "interface/machineinterface.h"

namespace creative_kernel
{
    CX3DExporterCommand::CX3DExporterCommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_shortcut = "Ctrl+Shift+S";
        m_actionname = tr("Save As Project");
        m_actionNameWithout = "Save As Project";
        m_eParentMenu = eMenuType_File;

        addUIVisualTracer(this);

        disconnect(getKernelUI()->getUI("footer"), SIGNAL(acceptButton()), this, SLOT(slotOpenLocalFolder()));
        connect(getKernelUI()->getUI("footer"), SIGNAL(acceptButton()), this, SLOT(slotOpenLocalFolder()));

        m_strMessageText = tr("Save Finished, Open Local Folder");
    }

    CX3DExporterCommand::~CX3DExporterCommand()
    {
    }

    void CX3DExporterCommand::execute()
    {
        sensorAnlyticsTrace("File Menu", "Save Project as");
        cxkernel::saveFile(this, QString("%1_%2.cxprj").arg(currentMachineName()).arg(mainModelName()));
    }

    QString CX3DExporterCommand::filter()
    {
        QString _filter = "Project File (*.cxprj)";
        return _filter;
    }

    QUrl CX3DExporterCommand::historyPath()
    {
        CX3DExporter* exporter = qobject_cast<CX3DExporter*>(parent());
        return exporter->openedPath();
    }

    void CX3DExporterCommand::handle(const QString& fileName)
    {
        saveProject(fileName, true, this);
    }

    void CX3DExporterCommand::onThemeChanged(ThemeCategory category)
    {
    }

    void CX3DExporterCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Save As Project") + "    " + m_shortcut;
    }

    void CX3DExporterCommand::slotOpenLocalFolder()
    {
        cxkernel::openLastSaveFolder();
    }

    QString CX3DExporterCommand::getMessageText()
    {
        return m_strMessageText;
    }

    void CX3DExporterCommand::accept()
    {
        cxkernel::openLastSaveFolder();
    }
}
