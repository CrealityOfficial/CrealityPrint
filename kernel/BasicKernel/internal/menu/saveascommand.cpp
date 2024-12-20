#include "saveascommand.h"

#include "data/modeln.h"

#include "kernel/kernelui.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/commandinterface.h"
#include "interface/selectorinterface.h"
#include "interface/uiinterface.h"
#include "cxkernel/interface/iointerface.h"
#include "cxkernel/data/trimeshutils.h"
#include "qtuser3d/trimesh2/conv.h"
#include "msbase/mesh/merge.h"

#include "interface/spaceinterface.h"
#include <QCoreApplication>
#include <QFileInfo>
#include "data/spaceutils.h"

namespace creative_kernel
{
    SaveAsCommand::SaveAsCommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_shortcut = "Ctrl+Shift+S";
        
        m_actionname = QCoreApplication::translate("MenuCmdObj", "Model File");  //tr("Save STL");
        m_actionNameWithout = "Save STL";
        m_insertKey = "saveSTL";
        m_eParentMenu = eMenuType_File;

        addUIVisualTracer(this,this);
    }

    SaveAsCommand::~SaveAsCommand()
    {
    }

    void SaveAsCommand::execute()
    {
        sensorAnlyticsTrace("File Menu", "Save as STL");
        QList<ModelN*> selections = selectionms();
        if (selections.size() == 0)
        {
            requestQmlDialog(this, "messageNoModelDlg");
            return;
        }

        cxkernel::saveFile(this, QString("%1.stl").arg(QFileInfo(mainModelName()).completeBaseName()));
    }

    void SaveAsCommand::onThemeChanged(ThemeCategory category)
    {
    }

    void SaveAsCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = QCoreApplication::translate("MenuCmdObj", "Model File");  //tr("Save STL") + "    " + m_shortcut;
    }

    QString SaveAsCommand::filter()
    {
        QString _filter = QString("Mesh File (*.stl)");
        return _filter;
    }

    void SaveAsCommand::handle(const QString& fileName)
    {
        std::vector<TriMeshPtr> meshs;
        QList<ModelN*> models = selectionms();
        size_t size = models.size();

        if (size > 0)
        {
            for (size_t i = 0; i < models.size(); i++)
            {
                TriMeshPtr newMesh = models.at(i)->createGlobalMeshWithNormal();
                if (newMesh.get())
                {
                    meshs.push_back(newMesh);
                }
            }

            trimesh::TriMesh* newmodel = msbase::mergeMeshes(meshs);
            cxkernel::saveMesh(newmodel, fileName);
        }

        m_fileName = fileName;

        m_strMessageText = tr("Save Finished, Open Local Folder");
        requestQmlDialog(this, "messageDlg");
    }

    QString SaveAsCommand::getMessageText()
    {
        return m_strMessageText;
    }

    void SaveAsCommand::accept()
    {
        openFileLocation(m_fileName);
    }
}
