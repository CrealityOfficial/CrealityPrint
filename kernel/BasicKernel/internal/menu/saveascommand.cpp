#include "saveascommand.h"

#include "data/modeln.h"

#include "kernel/kernelui.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/modelinterface.h"
#include "interface/commandinterface.h"
#include "interface/selectorinterface.h"
#include "interface/uiinterface.h"
#include "cxkernel/interface/iointerface.h"

#include "stringutil/util.h"
#include "cxbin/save.h"
#include "mmesh/trimesh/trimeshutil.h"
#include "qcxutil/trimesh2/conv.h"
#include "interface/spaceinterface.h"

namespace creative_kernel
{
    SaveAsCommand::SaveAsCommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_shortcut = "Ctrl+S";
        m_actionname = tr("Save STL");
        m_actionNameWithout = "Save STL";
        m_insertKey = "saveSTL";
        m_eParentMenu = eMenuType_File;

        addUIVisualTracer(this);
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

        cxkernel::saveFile(this, QString("%1.stl").arg(mainModelName()));
    }

    void SaveAsCommand::onThemeChanged(ThemeCategory category)
    {
    }

    void SaveAsCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Save STL") + "    " + m_shortcut;
    }

    QString SaveAsCommand::filter()
    {
        QString _filter = QString("Mesh File (*.stl)");
        return _filter;
    }

    void SaveAsCommand::handle(const QString& fileName)
    {
        std::wstring strWname = fileName.toStdWString();
        std::string strname = stringutil::wstring2string(strWname);

        std::vector<trimesh::TriMesh*> meshs;
        QList<ModelN*> models = selectionms();
        size_t size = models.size();

        if (size > 0)
        {
            for (size_t i = 0; i < models.size(); i++)
            {
                bool isfanzhuan = models.at(i)->isFanZhuan();
                QMatrix4x4 matrix = models.at(i)->globalMatrix();
                trimesh::TriMesh* meshTemp = models.at(i)->mesh();
                trimesh::TriMesh* newMesh = new trimesh::TriMesh();
                *newMesh = *meshTemp;

                trimesh::fxform xf = qcxutil::qMatrix2Xform(matrix);
                int _size = meshTemp->vertices.size();
                for (int n = 0; n < _size; ++n)
                {
                    trimesh::vec3 v = meshTemp->vertices.at(n);
                    newMesh->vertices.at(n) = xf * v;
                }
                if (isfanzhuan)
                {
                    _size = newMesh->faces.size();
                    for (int n = 0; n < _size; ++n)
                    {
                        newMesh->faces.at(n)[1] = meshTemp->faces.at(n)[2];
                        newMesh->faces.at(n)[2] = meshTemp->faces.at(n)[1];
                    }
                }
                meshs.push_back(newMesh);
            }

            trimesh::TriMesh* newmodel = new trimesh::TriMesh();
            mmesh::mergeTriMesh(newmodel, meshs);
            cxbin::save(strname, newmodel, nullptr);
        }

        qDeleteAll(meshs);

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
