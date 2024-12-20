#include "projectinfoui.h"
#include <QtCore/QSettings>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtQml/QQmlProperty>

#include "qtusercore/string/resourcesfinder.h"
#include "kernel/kernelui.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"
#include "interface/spaceinterface.h"
#include "kernel/kernel.h"
#include <boost/filesystem.hpp>
#include <QCoreApplication>
#include <qchar.h>
#include <qglobal.h>
#include <qlist.h>
#include "internal/project_cx3d/cx3dmanager.h"
#include "interface/shareddatainterface.h"
#ifdef Q_OS_WIN
#include <Windows.h>
#endif
#ifdef Q_OS_MACOS
#include <iostream>
#include <sys/sysctl.h>
#include <unistd.h>
#endif
#ifdef Q_OS_LINUX
#include <sys/sysinfo.h>
#include <unistd.h>
#include <stdio.h>
#endif
namespace creative_kernel
{
#ifdef Q_OS_WIN
    bool isProcessRunning(qint64 processId) {
           // 尝试打开进程
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
        if (hProcess == NULL) {
            // 无法打开进程，可能进程不存在
            if (GetLastError() == ERROR_INVALID_PARAMETER) {
                return false; // 进程不存在
            }
        } else {
            // 进程存在
            CloseHandle(hProcess); // 关闭句柄
            return true;
        }
        return false; // 出现其他错误或进程不存在
    }
    #elif defined Q_OS_MACOS
        bool isProcessRunning(pid_t pid) {
             int name[] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, pid};
            struct kinfo_proc info;
            size_t size = sizeof(info);
        
            // 获取指定PID的信息
            if (sysctl(name, sizeof(name) / sizeof(*name), &info, &size, NULL, 0) == -1) {
                // 如果无法获取信息，则进程不存在
                return false;
            }
        
            // 如果能获取信息，进程存在
            return true;
        }
    #else
     bool isProcessRunning(pid_t pid) {
        char procPath[64];
        sprintf(procPath, "/proc/%d/", pid);
    
        // 检查/proc目录下是否存在相应的进程信息文件
        if (access(procPath, F_OK) == 0) {
            return true; // 进程存在
        } else {
            return false; // 进程不存在
        }
    }
    #endif
        
    void writeTextFileUtf8(const QString &fileName, const QString &text) {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            // 处理错误，例如可以抛出异常或者返回错误标志
            return;
        }
    
        QTextStream out(&file);
        out.setCodec("UTF-8");
        out << text;
        file.close();
    }
    ProjectInfoUI* ProjectInfoUI::m_info=nullptr;
    ProjectInfoUI::ProjectInfoUI(QObject* parent)
        : QObject(parent)
        , m_callback(nullptr)
    {
        m_spaceDirty = false;
        
        
        m_strTmpCacheProjectPath = ":/cx3d/cx3d_exporter/.cxprj";
        
        QSettings setting;
        setting.beginGroup("AutoSave_Path");
        m_strProjectName = ".cxprj";
        m_strProjectPath = m_strTmpCacheProjectPath;
        m_strMessageText = tr("Do you Want to Save LastProject?");

        QString strFileName = setting.value("filePath", "default").toString();
        setting.endGroup();
        bool bTmpFileValid = false;
        bool blocked = false;
        if (!(strFileName == "default"))
        {
            m_strTempFilePath = strFileName;
            m_strTempFileDirPath = m_strTempFilePath.mid(0,m_strTempFilePath.lastIndexOf("/"));
            m_strProjectName = m_strTempFilePath.mid(m_strTempFilePath.lastIndexOf("/")+1);
            //check dir is lock by other process
            QFileInfo locker(m_strTempFileDirPath+"/lock.txt");
            
            if(locker.exists())
            {
                QFile f(locker.absoluteFilePath());
                if(f.open(QFile::ReadOnly))
                {
                    qint64 processid = QString::fromUtf8(f.readLine()).toLong();
                    if(isProcessRunning(processid))
                    {
                        blocked = true;
                    }

                }
                f.close();
            }
            //check origin is  exists
            QFileInfo origin(m_strTempFileDirPath+"/origin.txt");
            if(origin.exists() && !blocked)
            {
                QFile f(origin.absoluteFilePath());
                if(f.open(QFile::ReadOnly))
                {
                    QString originProjectPath = QString::fromUtf8(f.readLine());
                    QFileInfo originProject(originProjectPath);
                    if(originProject.exists())
                    {
                        setProjectPath(originProjectPath);
                        bTmpFileValid = true;
                    }
                    f.close();
                }
            }
            QFileInfo project(m_strTempFileDirPath+"/.cxprj");
            if(project.exists() && !bTmpFileValid && !blocked)
            {
                 setProjectName("Untitled.cxprj");
                 bTmpFileValid = true;
            }
        }
        if(!bTmpFileValid)
        {
            QString tmpProject = qtuser_core::getOrCreateTempDataLocation();
            m_strTempFileDirPath = tmpProject;
            m_strTempFilePath = m_strTempFileDirPath + "/.cxprj";
            setProjectName("Untitled.cxprj");
        }

        qint64 processId = QCoreApplication::applicationPid();
        writeTextFileUtf8(m_strTempFileDirPath+"/lock.txt",QString::number(processId));
    
        
        setSettingsData(m_strTempFilePath);
        QDir::setCurrent(m_strTempFileDirPath);
        addUIVisualTracer(this,this);
    }
    

    ProjectInfoUI::~ProjectInfoUI()
    {

    }
    ProjectInfoUI* ProjectInfoUI::instance()
    {
        //static ProjectInfoUI info;
        if (m_info == NULL)
            createInstance(NULL);
        return m_info;
    }
    ProjectInfoUI* ProjectInfoUI::createInstance(QObject* parent)
    {
        m_info = new ProjectInfoUI(parent);
        return m_info;
    }
    void ProjectInfoUI::setInitProjectDirty(bool bDirty)
    {
        m_bInitProjectDirty = bDirty;
        emit projectNameChanged();
    }
    bool ProjectInfoUI::spaceDirty()
    {
        if (modelns().size() == 0)return false;
        if(m_bInitProjectDirty) return true;
        if (!getKernel()->canUndo()) {
            return false;
        }
        return true;
    }

    QString ProjectInfoUI::getProjectNameNoSuffix()
    {
        int index = m_strProjectName.lastIndexOf(".");
        return m_strProjectName.left(index);
    }
    bool ProjectInfoUI::is3mf()
    {
        QFileInfo file(m_strProjectPath);
        if (file.exists())
        {
            return QString(file.suffix()).toLower() == "3mf";
        }
        return false;
    }
    QString ProjectInfoUI::getProjectName()
    {
        return m_strProjectName;
    }
 
    void ProjectInfoUI::setProjectName(QString strProName)
    {
        if(strProName == m_strProjectName)
            return;
        m_strProjectName = strProName;
        if(QFileInfo(m_strProjectPath).exists())
        {
            QDir dir(m_strTempFilePath);
            dir.rename(m_strTempFilePath,m_strTempFileDirPath + "/" + m_strProjectName);
            writeTextFileUtf8(m_strTempFileDirPath+"/origin.txt",m_strProjectPath);
        }
            
        m_strTempFilePath = m_strTempFileDirPath + "/" + m_strProjectName;
        setSettingsData(m_strTempFilePath);
        emit projectNameChanged();
    }

    QString ProjectInfoUI::getProjectPath()
    {
        return m_strProjectPath;
    }

    void ProjectInfoUI::setProjectPath(QString strProPath)
    {
        m_strProjectPath = strProPath;
    }

    void ProjectInfoUI::deleteTempProjectDirectory()
    {
        QDir::setCurrent(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        QDir tempDir(m_strTempFileDirPath);
        tempDir.removeRecursively();
        tempDir.remove(m_strTempFileDirPath);
        
    }
    QStringList listFiles(const QString& path) {
        QDir dir(path);
        QStringList dirList;
        if (!dir.exists() || dirList.size() > 0) {
            return dirList;
        }else{
            QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
            if(list.isEmpty())
            {
                //dirList.append(path);
            }
            Q_FOREACH (const QFileInfo& fileInfo, list) {
                if (fileInfo.isDir()) {
                    // 如果是文件夹，递归遍历
                    QStringList list = listFiles(fileInfo.absoluteFilePath());
                    dirList.append(list);
                } else {
                     qDebug() << fileInfo.fileName();
                    if(fileInfo.fileName()=="lock.txt")
                    {
                        QFile f(fileInfo.absoluteFilePath());
                        if(f.open(QFile::ReadOnly))
                        {
                            qint64 processid = QString::fromUtf8(f.readLine()).toLong();
                            if(!isProcessRunning(processid))
                            {
                                f.close();
                                dirList.append(fileInfo.absolutePath());
                            }
                            else
                            {
                                f.close();
                            }
                        }
                    }
                    
                }
            }
        }
        return dirList;
    }
    void ProjectInfoUI::cleanTempProjectDirectory()
    {
        QString tempProjectPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/creality_model/";
        QDir tempDir(tempProjectPath);
        QStringList idleDirList;
        QFileInfoList list = tempDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        idleDirList = listFiles(tempProjectPath);
   
        Q_FOREACH (const QString& dir, idleDirList) {
            QDir tempDir(dir);
            if(tempDir.exists())
            {
                tempDir.removeRecursively();
                tempDir.remove(dir);
            }
            
        }
     }
    void ProjectInfoUI::deleteTempGcodeAndPng()
    {
        QDir tempDir(m_strTempFileDirPath);

        for (const auto& entry : tempDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot)) {
            auto filename = entry.absoluteFilePath();
            auto suffix = entry.suffix();
            if (suffix == "gcode" || suffix == "png")
            {
                QFile(filename).remove();
            }
        }
    }

    void ProjectInfoUI::deleteTempProject()
    {
        QFile f(m_strTempFilePath);
        if(f.exists())
        {
            f.remove();
        }            
    }
    QString ProjectInfoUI::getAutoProjectPath()
    {
        return m_strTempFilePath;
    }

    void ProjectInfoUI::setSettingsData(QString file)
    {
        file = file.replace("file:///", "");
        QSettings setting;
        setting.beginGroup("AutoSave_Path");
        setting.setValue("filePath", file);
        setting.endGroup();
    }

    void ProjectInfoUI::updateProjectNameUI()
    {
        QObject* obj = getKernelUI()->getUI("footer");    //   getKernelUI()->footer();
        QObject* pProText = obj->findChild<QObject*>("ProjectTitle");
        if (pProText)
        {
            QQmlProperty::write(pProText, "text", QVariant::fromValue(m_strProjectName));
        }
    }

    QString ProjectInfoUI::getNameFromPath(QString path)
    {
        QString strName = "";
        QFileInfo info(path);
        // qDebug()<<"info.fileName()=" << info.fileName();
        strName = info.fileName();
        return strName;
    }

    void ProjectInfoUI::clearSecen(bool clearModel)
    {
        getKernel()->cx3dManager()->stopAutoSave();
        using namespace creative_kernel;
        clearProjectCache(clearModel);
        if (clearModel)
        {
            clearSharedData();
        }
        
        getKernel()->cx3dManager()->startAutoSave();
    }

    void ProjectInfoUI::requestMenuDialog(ProjectOpenCallback* callback)
    {
        m_callback = callback;
        requestQmlDialog(this, "messageDlg");
    }

    QString ProjectInfoUI::getMessageText()
    {
        return m_strMessageText;
    }

    void ProjectInfoUI::onThemeChanged(ThemeCategory category)
    {

    }

    void ProjectInfoUI::onLanguageChanged(MultiLanguage language)
    {
        m_strMessageText = tr("Do you Want to Save LastProject?");
    }
    bool ProjectInfoUI::availablePath()
    {

        QFile file(m_strProjectPath);
        if(m_strProjectPath.indexOf(m_strTempFileDirPath)>=0) return false;
        if (file.exists() && m_strProjectPath != m_strTmpCacheProjectPath) return true;
        return false;
    }
    void ProjectInfoUI::accept()
    {
        if (m_callback)
            m_callback->accept();
        m_callback = nullptr;
    }
    
    void ProjectInfoUI::cancel()
    {
        if (m_callback)
            m_callback->cancel();
        m_callback = nullptr;
    }

    void ProjectInfoUI::updateFileStateUI()
    {
        QObject* obj = getKernelUI()->getUI("footer");    // getKernelUI()->footer();
        QObject* pProText = obj->findChild<QObject*>("SavefinishShow");
        if (pProText)
        {
            QQmlProperty::write(pProText, "visible", QVariant::fromValue(true));
        }
    }
}

