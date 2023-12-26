#include "load3mf.h"

#include "data/modeln.h"

#include "kernel/kernelui.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/modelinterface.h"
#include "interface/commandinterface.h"
#include "interface/selectorinterface.h"
#include "interface/uiinterface.h"
#include "cxkernel/interface/iointerface.h"
#include "cxkernel/data/trimeshutils.h"
#include "qtuser3d/trimesh2/conv.h"
#include "msbase/mesh/merge.h"
#include <QtWidgets/QFileDialog>
#include "interface/spaceinterface.h"

#include "cxkernel/utils/utils.h"

#include "3mfimproter.h"

#include "cxkernel/interface/modelninterface.h"
#include "cxkernel/data/trimeshutils.h"

#include "cxkernel/utils/modelfrommeshjob.h"
#include "cxkernel/interface/jobsinterface.h"

#include "interface/machineinterface.h"
#include "msbase/mesh/deserializecolor.h"  
#include "cxkernel/interface/jobsinterface.h"
#include "qtusercore/module/progressortracer.h"

namespace creative_kernel
{

    Load3MFCommand::Load3MFCommand(QObject* parent)
        :ActionCommand(parent)
    {
        //m_shortcut = "Ctrl+S";
        m_actionname = tr("Load 3MF");
        m_actionNameWithout = "Load 3MF";
        m_insertKey = "load3MF";
        m_eParentMenu = eMenuType_File;

        addUIVisualTracer(this);
    }

    Load3MFCommand::~Load3MFCommand()
    {
    }

    void Load3MFCommand::onThemeChanged(ThemeCategory category)
    {
    }

    void Load3MFCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Load 3MF") + "    " + m_shortcut;
    }

    QString Load3MFCommand::filter()
    {
        QString _filter = QString("Mesh File (*.3mf)");
        return _filter;
    }

    void updateColor(trimesh::TriMesh* mesh,std::vector<std::string>& colors)
    {
        /*if (!colors.empty())
        {
            TriMeshPtr _mesh(msbase::mergeColorMeshes(mesh, colors));
            creative_kernel::applyMaterialColorsToMesh(_mesh.get());

            mesh->colors = _mesh->colors;
            mesh->flags = _mesh->flags;
        }*/
    }

    void Load3MFCommand::execute()
    {
        sensorAnlyticsTrace("File Menu", "Load 3MF");
        //cxkernel::saveFile(this, QString("%1.stl").arg(mainModelName()));

        QStringList fileNames;
        auto f = [this](const QString& file) {
            cxkernel::openFileWithName(file);
        };

        QString title = QObject::tr("OpenFile");
        QString filter = "Mesh file(*.3mf)";
        QSettings setting;
        QString lastPath = setting.value("dialogLastPath", "").toString();
#ifdef Q_OS_LINUX
        fileNames = QFileDialog::getOpenFileNames(
            nullptr, title,
            lastPath, filter, nullptr, QFileDialog::DontUseNativeDialog);
#else
        fileNames = QFileDialog::getOpenFileNames(
            nullptr, title,
            lastPath, filter);
#endif
        if (fileNames.isEmpty())
            return;

        QList<qtuser_core::JobPtr> jobs;
        for (auto& file : fileNames)
        {
            Load3MFJob* loadJob = new Load3MFJob(this);
            loadJob->setFileName(file);
            jobs.push_back(qtuser_core::JobPtr(loadJob));
            cxkernel::executeJobs(jobs);
        }


        //_BBS_3MF_Importer bbs_3mf_importer;
        //for (auto file : fileNames)
        //{
        //    std::vector<trimesh::TriMesh*> meshs;
        //    std::vector<std::vector<std::string>> colors;
        //    bbs_3mf_importer.load3MF(cxkernel::qString2String(file), meshs, colors);

        //    QString shortName = file;
        //    QStringList stringList = shortName.split("/");
        //    if (stringList.size() > 0)
        //        shortName = stringList.back();

        //    for (int i = 0;i < meshs.size();i++)
        //    {
        //        trimesh::TriMesh* mesh = meshs[i];
        //        cxkernel::ModelCreateInput input;
        //        input.mesh.reset(mesh);
        //        input.fileName = file;
        //        input.name = shortName;
        //        input.type = cxkernel::ModelNDataType::mdt_file;
        //        input.colors = colors[i % colors.size()];
        //        updateColor(input.mesh.get(), input.colors);
        //        addModelFromCreateInput(input);
        //    }
        //}
    }

    void Load3MFCommand::handle(const QString& fileName)
    {
        std::vector<trimesh::TriMesh*> meshs;
        QList<ModelN*> models = selectionms();
        size_t size = models.size();
    }

    QString Load3MFCommand::getMessageText()
    {
        return m_strMessageText;
    }

    void Load3MFCommand::accept()
    {
        openFileLocation(m_fileName);
    }

    Load3MFJob::Load3MFJob(QObject* parent)
        : Job(parent)
    {
    }

    Load3MFJob::~Load3MFJob()
    {
    }

    void Load3MFJob::setFileName(const QString& fileName)
    {
        m_fileName = fileName;
    }

    QString Load3MFJob::name()
    {
        return "";
    }

    QString Load3MFJob::description()
    {
        return "";
    }

    void Load3MFJob::failed()
    {
        qDebug() << "";
    }

    void Load3MFJob::successed(qtuser_core::Progressor* progressor)
    {
    
    }

    void Load3MFJob::work(qtuser_core::Progressor* progressor)
    {
        qtuser_core::ProgressorTracer tracer(progressor);

        _BBS_3MF_Importer bbs_3mf_importer;
        std::vector<trimesh::TriMesh*> meshs;
        std::vector<std::vector<std::string>> colors;
        std::vector<std::vector<std::string>> seams;
        std::vector<std::vector<std::string>> supports;
        bbs_3mf_importer.load3MF(cxkernel::qString2String(m_fileName), meshs, colors, seams, supports);

        QString shortName = m_fileName;
        QStringList stringList = shortName.split("/");
        if (stringList.size() > 0)
            shortName = stringList.back();

        for (int i = 0; i < meshs.size(); i++)
        {
            trimesh::TriMesh* mesh = meshs[i];
            cxkernel::ModelCreateInput input;
            input.mesh.reset(mesh);
            input.fileName = m_fileName;
            input.name = shortName;
            input.type = cxkernel::ModelNDataType::mdt_file;
            input.colors = colors[i % colors.size()];
            input.seams = seams[i % seams.size()];
            input.supports = supports[i % supports.size()];
            //updateColor(input.mesh.get(), input.colors);
            addModelFromCreateInput(input);
        }
    }
}
