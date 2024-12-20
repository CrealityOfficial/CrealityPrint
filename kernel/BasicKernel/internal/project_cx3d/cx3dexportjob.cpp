#include "cx3dexportjob.h"
#include "cx3dwriter.h"
#include "cx3dreader.h"
#include "cx3dprojectmanager.h"
#include <QtCore/QFileInfo>
#include "cx3drenderex.h"
#include "cx3dautosaveproject.h"
#include "cx3dsubmenurecentproject.h"

#include "qtusercore/string/resourcesfinder.h"
#include "cxkernel/data/modelndataserial.h"

#include "interface/uiinterface.h"
#include "interface/projectinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/spaceinterface.h"
#include <interface/machineinterface.h>
#include "qtusercore/module/quazipfile.h"
#include "qtusercore/module/systemutil.h"
#include <QTemporaryDir>
#include <QDomDocument>
#include "interface/printerinterface.h"
#include "internal/project_3mf/load3mf.h"
#include "internal/project_3mf/save3mf.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/printmachine.h"
#include "data/kernelenum.h"
#include "external/interface/commandinterface.h"
using namespace creative_kernel;
Cx3dExportJob::Cx3dExportJob(QString saveFilename,JobType jobtype,QObject* parent)
    :m_saveFilename(saveFilename)
    , m_jobType(jobtype)
    , m_buildScene(nullptr)
    , m_mainThread(nullptr)
    , m_closeWindow(false)
    , m_successObject(nullptr)
    , m_showProgress(true)
{
    m_mainThread = QThread::currentThread();
    m_buildScene = new Cx3dScene(this);
}

Cx3dExportJob::~Cx3dExportJob()
{
}

void Cx3dExportJob::AddModel2Scence()
{
}

QString Cx3dExportJob::getMessageText()
{
    return tr("Cannot import project file");
}

void Cx3dExportJob::failed()                        // invoke from main thread
{
    if ((m_jobType == JobType::WRITEJOB) && m_closeWindow)
        requestQmlCloseAction();
    else
    {
        creative_kernel::requestQmlDialog(this, "messageDlg");
    }
    if(m_jobType == JobType::WRITEJOB && m_removeProjectCache)
        savePostProcess(false);
}

void Cx3dExportJob::setMachineProfile()
{

}

bool Cx3dExportJob::showProgress()
{
    return m_showProgress;
}

QString Cx3dExportJob::name()
{
    if (m_jobType == JobType::WRITEJOB)
    {
        return "AutoSaveJob";
    }

    return "ReadJob";    
}

void Cx3dExportJob::successed(qtuser_core::Progressor* progressor)
{
    if (m_jobType == JobType::READJOB)
    {
        m_buildScene->setScene();
    }
    else
    {

    }
    if (m_removeProjectCache)
    {
        creative_kernel::projectUI()->clearSecen(false);
        projectUI()->setProjectPath(m_saveFilename);
    }
    if (!(m_saveFilename.indexOf("/tmpProject/") > 0 || m_saveFilename.indexOf("cx3d_exporter/NewProject.cxprj") >0))
    {
        Cx3dSubmenuRecentProject::getInstance()->setMostRecentFile(m_saveFilename);
    }

    projectUI()->setProjectName(ProjectInfoUI::instance()->getNameFromPath(m_saveFilename));

    if((m_jobType == JobType::WRITEJOB) && m_closeWindow)
        requestQmlCloseAction();

    if((m_jobType == JobType::WRITEJOB) && m_successObject && !m_newBuild)
        requestQmlDialog("saveProjectDlg");
    if (m_jobType == JobType::WRITEJOB && m_removeProjectCache)
        savePostProcess(true);
}

void Cx3dExportJob::setCloseWindow(bool close)
{
    m_closeWindow = close;
}

void Cx3dExportJob::setCaches(const QList<creative_kernel::ModelN*>& models)
{
    m_caches = models;
}

void Cx3dExportJob::setExportSuccessObject(QObject* object)
{
    m_successObject = object;
}

void Cx3dExportJob::setShowProgress(bool show)
{
    m_showProgress = show;
}

void Cx3dExportJob::setNewBuild(bool needBuild)
{
    m_newBuild = needBuild;
}
void Cx3dExportJob::setRemoveProjectCache(bool cache)
{
    m_removeProjectCache = cache;
}
void Cx3dExportJob::work(qtuser_core::Progressor* progressor)
{
    if(m_jobType == JobType::WRITEJOB)
    {
        if(!m_showProgress)
        {
            for (creative_kernel::ModelN* m : m_caches)
            {
                QFileInfo fileInfo(m->getSerialName());
                QString filename = fileInfo.fileName();
                filename = QString("%1/%2").arg(qtuser_core::getOrCreateAppDataLocation("tmpProject/models")).arg(filename);
                if (!QFileInfo(filename).exists())
                {
                    cxkernel::ModelNDataSerial serial;
                    serial.setMeshData(m->modelNData());
                    serial.setColorData(m->getColors2Facets());
                    serial.setSeamData(m->getSeam2Facets());
                    serial.setSupportData(m->getSupport2Facets());
             
                    //serial.setData(m->modelNData());
                    serial.save(filename, nullptr);
                }
            }
        }
        //Cx3dWriter writer(m_caches, m_saveFilename, progressor,nullptr,!m_showProgress);
        cx3d::Cx3dFileProject fileManager(m_saveFilename, progressor, nullptr, !m_showProgress, true);
        auto params_dir = qtuser_core::getOrCreateAppDataLocation("tmpProject/params");
        if (!creative_kernel::exportAllCurrentSettings(params_dir))
        {
            return;
        }
        QTemporaryDir temp_dir;
        auto zip_file = temp_dir.filePath("param") + ".zip";
        qtuser_core::compressDir(zip_file, params_dir);
        clearPath(params_dir);

        fileManager.setFieldData(cx3d::FieldName::PLATE, creative_kernel::getPrinterSerialData());

        QFile file(zip_file);
        if (file.open(QIODevice::ReadWrite))
        {
            fileManager.setFieldData(cx3d::FieldName::PARAM, file.readAll());
            file.close();
        }
        fileManager.setModelData(m_caches);
        fileManager.saveCx3dProject("1.0.0");
    }
    else
    {
        //Cx3dRenderEx reader(m_saveFilename, progressor);
        //reader.build();
        cx3d::Cx3dFileProject fileManager(m_saveFilename, progressor, nullptr, !m_showProgress, false);
        fileManager.readCx3dProject();
        QByteArray plate_data;
        fileManager.getFieldData(cx3d::FieldName::PLATE, plate_data);
        createPrinterFromProject(plate_data);
        QByteArray param_data;
        fileManager.getFieldData(cx3d::FieldName::PARAM, param_data);
        loadParamFromProject(param_data);
        m_buildScene->build(&fileManager, m_mainThread);
        //reader->deleteLater();
    }
}

void saveProject(const QString& projectName, bool showProgress, QObject* successObject, bool force,bool newBuild, bool removeProjectCache)
{
    if (!force && !cxkernel::jobExecutorAvaillable())
        return;
    creative_kernel::projectUI()->setInitProjectDirty(false);
    QFileInfo file(projectName);
   if (file.suffix().toLower() == "3mf" || file.suffix().toLower() == "cxprj")
    {
        QList<qtuser_core::JobPtr> jobs;
        Save3MFJob* loadJob = new Save3MFJob();
        loadJob->setFileName(projectName);
        loadJob->setShowProgress(showProgress);
        loadJob->setRemoveProjectCache(removeProjectCache);
        jobs.push_back(qtuser_core::JobPtr(loadJob));
        cxkernel::executeJobs(jobs);
    }

}
QString getCxprjVersion(QString projectPath) {
    QString version;
    QuaZip quaZip(projectPath);
    if (quaZip.open(QuaZip::mdUnzip)) {
        quaZip.setCurrentFile("Content.xml");
        QuaZipFile modelFile;
        modelFile.setZip(&quaZip);
        std::map<int, QString> meshMaps;
        if (modelFile.open(QIODevice::ReadOnly)) {
            QDomDocument doc;
            if (!doc.setContent(&modelFile)) {
                qDebug() << "parse xml error";
                modelFile.close();
                quaZip.close();
                return "";
            }

            auto docElem = doc.documentElement();
            if (!docElem.isNull() && "workspace" == docElem.tagName()) {
                version = docElem.attribute("Version");
            }
        }
        modelFile.close();
    }
    quaZip.close();
    return version;
}
void loadProject(const QString& projectName,bool bCleanTemp)
{
    qInfo() << "loadProject projectName =" << projectName;
    PrintMachine* pm = ParameterManager::currentMachine_s();
    if (pm)
    {
        pm->resetState();
    }
    
    setKernelPhase(KernelPhaseType::kpt_prepare);
    creative_kernel::projectUI()->clearSecen();

    if (bCleanTemp)
    {
        creative_kernel::projectUI()->deleteTempProject();
        creative_kernel::projectUI()->deleteTempGcodeAndPng();
    }
    QFileInfo file(projectName);
    if (file.suffix().toLower() == "cxprj")
    {
        //�ж�cxprj�ĸ�ʽ���ɵĸ�ʽ
        if (getCxprjVersion(projectName)=="1.0.0")
        {
            Cx3dExportJob* job3 = new Cx3dExportJob(projectName, Cx3dExportJob::JobType::READJOB);
            cxkernel::executeJob(job3);
        }
        else {
            QList<qtuser_core::JobPtr> jobs;
            Load3MFJob* loadJob = new Load3MFJob();
            loadJob->setFileName(projectName);
            jobs.push_back(qtuser_core::JobPtr(loadJob));
            cxkernel::executeJobs(jobs);
        }
       
    }
    else if(file.suffix().toLower() == "3mf")
    {
        qDebug() << "loadProject is 3mf";
        QList<qtuser_core::JobPtr> jobs;
        Load3MFJob* loadJob = new Load3MFJob();
        loadJob->setFileName(projectName);
        jobs.push_back(qtuser_core::JobPtr(loadJob));
        cxkernel::executeJobs(jobs);

    }
}

