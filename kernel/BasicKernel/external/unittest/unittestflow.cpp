#include "unittestflow.h"
#include <QtCore/QDebug>
#include "external/interface/uiinterface.h"
#include "interface/printerinterface.h"
#include "cxkernel/interface/iointerface.h"
#include "slice/ansycslicer.h"
#include "kernel/kernel.h"
#include "interface/unittestinterface.h"
#include <QSettings>
#include <QDir>
#include "interface/commandinterface.h"
#include "interface/projectinterface.h"
#include "slice/sliceflow.h"
#include <QCoreApplication>
#include <QDateTime>
#include <internal/project_cx3d/cx3dmanager.h>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include "buildinfo.h"
namespace creative_kernel
{
    
    static QString m_3mfPath = "";
    static QString m_BlPath = "";
    static QString m_resultPath = "";
    static bool m_enabled = false;
    static bool g_message_enabled = true;
    static int m_curFuncType = 1;     //0:generate 1:compare 2:update
    static int m_cur_file_index = 0;
    static bool g_only_slice_enabled = false;   //false :基线对比   true: 完整切片流程，并保存切片数据
    static QStringList m_scpListFiles;  //一个scp列表文件
    static QVector<MyJsonMessage>m_jsonVec;
    QVariant BaseSettings::getSettingValue(const QString& key, const QVariant& defaultValue)
    {
        QSettings setting;
        setting.beginGroup("baseline_config");
        QVariant val = setting.value(key, defaultValue);
        setting.endGroup();

        return val;
    }

    void BaseSettings::setSettingValue(const QString& key, const QVariant& value)
    {
        QSettings setting;
        setting.beginGroup("baseline_config");
        setting.setValue(key, value);
        setting.endGroup();
    }
    void BLCompareTestFlow::setFuncType(int type)
    {
        m_curFuncType = type;
    }
    bool BLCompareTestFlow::messageEnabled()
    {
        return g_message_enabled;
    }
    void BLCompareTestFlow::setMessageEnabled(bool enable)
    {
        g_message_enabled = enable;
    }
    void BLCompareTestFlow::loadText(const QString& filename,QString&resultText)
    {
        QFile qFile(filename);
        if (!qFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug() << QString("loadSliceProfile [%1] failed.").arg(filename);
            return;
        }
        QTextStream in(&qFile);
        in.setCodec("utf-8");
        resultText = in.readLine();
        //while (!in.atEnd()) {
        //    resultText = in.readLine();
        //    // 处理 line
        //}
        qFile.close();
    }
    void BLCompareTestFlow::startBLTest()
    {
        if (!enabled())return;
        QString tmpBlPath = m_BlPath;
        //1.BLTEST 开始导入文件。
        QDir dir(m_3mfPath);
        //
        UnitTestFlow* unitFLow = unitTestFlow();
        unitFLow->setSlicerBaseLinePath(m_BlPath);
        unitFLow->setCompareErrorPath(m_resultPath);
        switch (m_curFuncType)
        {
        case 0:
            unitFLow->generateTest();
            break;
        case 1:
            unitFLow->compareTest();
            break;
        case 2:
            unitFLow->uploadTest();
            break;
        default:
            unitFLow->compareTest();
            break;
        }
        auto _3mfSourceDir = [=]() {
            {
                //源码路径
                const QString sourcePath = SOURCE_ROOT + QStringLiteral("/UnitTest");
                //当前可执行程序路径
                const QString curApplicationPath = QCoreApplication::applicationDirPath() + QStringLiteral("/UnitTest");
                
                QString m_3mfPath = "";
                bool _3mfRet = QFileInfo::exists(QStringLiteral("%1/Source/3MF/").arg(sourcePath));
                if (_3mfRet)
                {
                    m_3mfPath = QStringLiteral("%1/Source/3MF/").arg(sourcePath);
                }
                else
                {
                    _3mfRet = QFileInfo::exists(QStringLiteral("%1/Source/3MF/").arg(curApplicationPath));
                    if (_3mfRet)
                    {
                        m_3mfPath = QStringLiteral("%1/Source/3MF/").arg(curApplicationPath);
                    }
                }
                return m_3mfPath;
            };
        };
        // 获取所有文件和目录
        QFileInfoList list = entryFileInfoList(m_3mfPath);
        if (m_cur_file_index >= list.size())return;
        if (m_cur_file_index == 0)
        {
            m_jsonVec.clear();
            m_jsonVec.resize(list.size());
        }
        QFileInfo fileInfo = list.at(m_cur_file_index);
        // 如果是文件
        qDebug() << fileInfo.fileName();
        if (fileInfo.isFile()) {
            qDebug() << "Found file:" << fileInfo.absoluteFilePath();
            QString _3mfPath;
            loadText(fileInfo.absoluteFilePath(), _3mfPath);
            _3mfPath = _3mfPath.replace(">import", "").trimmed().replace("\"", "");
            qDebug() << "_3mfPath:" << _3mfPath;
            QString realPath = _3mfSourceDir()  +"/" + _3mfPath;
            QFileInfo _3mfInfo((realPath));
            if (!_3mfInfo.exists())
            {
                MyJsonMessage jsonMessage = m_jsonVec[m_cur_file_index];
                jsonMessage.path = fileInfo.absoluteFilePath();
                jsonMessage.result = "Failed:";
                jsonMessage.reason = "scp config path is not exist";
                jsonMessage.startTime = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss");
                jsonMessage.endTime = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss");
                m_jsonVec[m_cur_file_index] = jsonMessage;
                nextBLTest();
                return;
            }
            m_jsonVec[m_cur_file_index].path = fileInfo.absoluteFilePath();
            m_jsonVec[m_cur_file_index].startTime = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss");
            if (_3mfInfo.suffix() == "3mf")
            {
                removeResultCache();
                copyDirectoryStructureByFilter("3MF", realPath, tmpBlPath);
                unitFLow->setSlicerBaseLinePath(tmpBlPath);
                getKernel()->cx3dManager()->openProject(realPath);
            }
            else
            {
                nextBLTest();
            }
        }
    }
    void BLCompareTestFlow::endBLTest()
    {
        if (g_only_slice_enabled)
        {
            return;
        }
        QString filename = m_resultPath + "/result.errtxt";
        QFile qFile(filename);
        QString resultMess = "";
        if (!qFile.exists() || !qFile.open(QIODevice::ReadOnly))
        {
            MyJsonMessage jsonMessage = m_jsonVec[m_cur_file_index];
            jsonMessage.result = "Pass";
            
            jsonMessage.endTime = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss");
            m_jsonVec[m_cur_file_index] = jsonMessage;
            return;
        }
        while (qFile.atEnd() == false) {
            QString line = qFile.readLine().simplified();
            if (line.isEmpty()) {
                continue;
            }
            if (line.contains("error:"))
            {
                if (line.contains("baseline file generate failed!"))
                {
                    line = "error:baseline file has exist.generate failed!";
                }
                resultMess += line + "\n";
            }
            else
            {
                continue;
            }
        }
        qFile.close();

        m_jsonVec[m_cur_file_index].result = "Failed";
        m_jsonVec[m_cur_file_index].endTime = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss");
        m_jsonVec[m_cur_file_index].reason = resultMess;
    }
    void BLCompareTestFlow::endBLTestFlowError(const QString& error)
    {
        QString resultMess = "";
        if (error == "preSlicerError")
        {
            resultMess = "error:pre slicer is failed,can not slice";
        }
        m_jsonVec[m_cur_file_index].result = "Failed";
        m_jsonVec[m_cur_file_index].endTime = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss");
        m_jsonVec[m_cur_file_index].reason = resultMess;
        nextBLTest();
    }
    
    void BLCompareTestFlow::nextBLTest()
    {
        if (g_only_slice_enabled)
        {
            QCoreApplication::quit();
            return;
        } 
        m_cur_file_index++;
        creative_kernel::setKernelPhase(KernelPhaseType::kpt_prepare);
        QDir dir(m_3mfPath);
        QFileInfoList list = entryFileInfoList(m_3mfPath);
        if (m_cur_file_index >= list.size())
        {
            qDebug() << "list result = " << m_jsonVec.size();
            writeResultData();
            QCoreApplication::quit();
            return;
        }
        creative_kernel::projectUI()->clearSecen();
        startBLTest();

    }
    QList<QFileInfo> BLCompareTestFlow::entryFileInfoList(const QString& path) {
        QList<QFileInfo> findFilesInfo;
        
        //通过列表文件获取infos
        if (!m_scpListFiles.isEmpty())
        {
            for (auto file : m_scpListFiles)
            {
                QFileInfo fileInfo(file);
                if (fileInfo.isFile() && fileInfo.exists())
                {
                    findFilesInfo.append(fileInfo);
                }
            }
            return findFilesInfo;
        }
        QFileInfo fileInfo(path);
        //单个文件
        if (fileInfo.exists() && fileInfo.isFile()) {
            findFilesInfo.append(fileInfo);
            return findFilesInfo;
        }

        QDir dir(path);
        
        // 获取所有文件和目录
        QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

        for (int i = 0; i < list.size(); ++i) {
            QFileInfo fileInfo = list.at(i);

            // 如果是文件，检查扩展名
            qDebug() << fileInfo.fileName();
            if (fileInfo.isFile() && fileInfo.suffix().toLower() == "scp") {
                qDebug() << "Found file:" << fileInfo.absoluteFilePath();
                findFilesInfo.append(fileInfo);
            }

            // 如果是目录，递归调用
            if (fileInfo.isDir()) {
                findFilesInfo << entryFileInfoList(fileInfo.absoluteFilePath());

            }
        }
        return findFilesInfo;
    }

    void BLCompareTestFlow::writeResultData()
    {
        int _totalCount = m_jsonVec.size();
        int errCount = 0;
        int passCount = 0;
        QJsonArray result_array;
        QList<QString> pathLists;
        for (int index = 0; index < _totalCount; index++)
        {
            QJsonObject printerObj;
            printerObj.insert("path", m_jsonVec[index].path);
            printerObj.insert("startTime", m_jsonVec[index].startTime);
            printerObj.insert("endTime", m_jsonVec[index].endTime);
            printerObj.insert("result", m_jsonVec[index].result);
            printerObj.insert("reason", m_jsonVec[index].reason);
            result_array.push_back(printerObj);
            if (m_jsonVec[index].result.contains("Pass"))
            {
                passCount++;
            }
            else
            {
                errCount++;
                pathLists << m_jsonVec[index].path;
            }
        }
        //QJsonArray count_array;
        //{
            QJsonObject coun_object;
            coun_object["total"] = _totalCount;
            coun_object["passcount"] = passCount;
            coun_object["errorcount"] = errCount;
            //count_array.push_back(coun_object);
        //}
        
        auto writeResultJson = [=](){
            {
                QJsonObject rootObj;
                rootObj.insert("ResultList", result_array);
                rootObj.insert("Statistics", coun_object);
                QJsonDocument doc(rootObj);
                doc.toJson(QJsonDocument::Compact);
                // 打开文件
                QString strPath = m_resultPath + "/compare.result";
                QFile file(strPath);
                if (!file.open(QIODevice::WriteOnly)) {
                    qWarning("Couldn't open file.");
                    return;
                }
                // 写入文件
                file.write(doc.toJson());
                file.close();
            };
        };
        auto writeFailedList = [](QList<QString> paths)
        {
            QString strPath = m_resultPath + "/failedlist.json";
            QFile file(strPath);
            if (paths.size() == 0)
            {
                file.remove();
                return;
            }
            QJsonArray path_array;
            for (const auto& path : paths) {
                path_array.push_back(QJsonValue{ path });
            }
            QJsonDocument doc(path_array);
            doc.toJson(QJsonDocument::Compact);
            // 打开文件
            if (!file.open(QIODevice::WriteOnly)) {
                qWarning("Couldn't open file.");
                return;
            }
            // 写入文件
            file.write(doc.toJson());
            file.close();
        };
        writeResultJson();
        writeFailedList(pathLists);
    }

    bool BLCompareTestFlow::removeResultCache()
    {
        QString filename = m_resultPath + "/result.errtxt";
        QFile::remove(filename);
        return true;
    }
    void BLCompareTestFlow::copyDirectoryStructureByFilter(const QString& filterName,const QString& sourcePath, QString& destinationPath)
    {
        QFileInfo info(sourcePath);
        QStringList parts = info.absolutePath().split("/");
        int index = parts.indexOf(filterName);
        if (sourcePath.contains(filterName) && parts.indexOf(filterName) > 0)
        {
            if (info.isDir() || info.isFile())
            {
                for (int i = index + 1; i < parts.size(); ++i) {
                    if (i != index) {
                        destinationPath += "/" + parts.at(i);
                        QDir destDir(destinationPath);
                        if (!destDir.exists()) {
                            if (!destDir.mkpath(destinationPath)) {
                                continue;
                            }
                        }
                    }
                }
            }
        }
    }
    bool BLCompareTestFlow::enabled()
    {
        return m_enabled;
    }
    bool BLCompareTestFlow::sliceCacheEnabled()
    {
        return g_only_slice_enabled;
    }
    QString BLCompareTestFlow::_3MFPath()
    {
        return m_3mfPath;
    }
    QString BLCompareTestFlow::BLPath()
    {
        return m_BlPath;
    }
    QString BLCompareTestFlow::CompareResultPath()
    {
        return m_resultPath;
    }
    void BLCompareTestFlow::setEnabled(bool enable)
    {
        m_enabled = enable;
    }
    void BLCompareTestFlow::setSliceCacheEnabled(bool enable)
    {
        g_only_slice_enabled = enable;
    }
    void BLCompareTestFlow::set3mfPath(const QString& path)
    {
        m_3mfPath = path;
    }
    void BLCompareTestFlow::setScpList(const QStringList& scpListPath)
    {
        m_scpListFiles = scpListPath;
    }
    void BLCompareTestFlow::setBLPath(const QString& blpath)
    {
        m_BlPath = blpath;
    }
    void BLCompareTestFlow::setCompareResultPath(const QString& path)
    {
        m_resultPath = path;
    }

    UnitTestFlow::UnitTestFlow(QObject* parent)
        : QObject(parent)
        
    {
        m_unitType = SlicerUnitType::None;
        m_strSlicerBaseLinePath = BaseSettings::getSettingValue("slicer_baseline_path", QString("%1/").arg(SLICE_PATH)).toString();
        m_strCompareErrorPath = BaseSettings::getSettingValue("slicer_compare_path", QString("%1/").arg(SLICE_PATH)).toString();
        m_SliceCachePath = BaseSettings::getSettingValue("slicer_cache_path", QString("%1/").arg(SLICE_PATH)).toString();
        m_GcodePath = BaseSettings::getSettingValue("slicer_gcode_path", QString("%1/").arg(SLICE_PATH)).toString();
    }

    UnitTestFlow::~UnitTestFlow()
    {

    }

    void UnitTestFlow::registerContext()
    {
        registerContextObject("kernel_unittest", this);
    }

    void UnitTestFlow::initialize()
    {
        
    }

    void UnitTestFlow::uninitialize()
    {

    }

    void UnitTestFlow::generateTest()
    {
        qDebug() << "generateTest() ";
        m_unitType = SlicerUnitType::Ganerate;
    }
    void UnitTestFlow::compareTest()
    {
        qDebug() << "compareTest() ";
        m_unitType = SlicerUnitType::Compare;
    }
    void UnitTestFlow::uploadTest()
    {
        qDebug() << "uploadTest() ";
        m_unitType = SlicerUnitType::Update;
    }
    QString UnitTestFlow::slicerBaseLinePath() const {
        return m_strSlicerBaseLinePath;
    }
    void UnitTestFlow::setSlicerBaseLinePath(const QString& strPath) {
        m_strSlicerBaseLinePath = strPath;
        BaseSettings::setSettingValue("slicer_baseline_path", strPath);
        Q_EMIT slicerBaseLinePathChanged();
    }

    QString UnitTestFlow::compareErrorPath()const {
        return m_strCompareErrorPath;
    }
    void UnitTestFlow::setCompareErrorPath(const QString& strPath) {
        m_strCompareErrorPath = strPath;
        BaseSettings::setSettingValue("slicer_compare_path", strPath);
        Q_EMIT slicerCPErrorPathChanged();
    }

    QString UnitTestFlow::sliceCachePath() const
    {
        return m_SliceCachePath;
        
    }
    void UnitTestFlow::setSliceCachePath(const QString& strPath)
    {
        m_SliceCachePath = strPath;
        BaseSettings::setSettingValue("slicer_cache_path", strPath);
        Q_EMIT slicerPathChanged();
    }

    QString UnitTestFlow::gcodePath() const
    {
        return m_GcodePath;
    }
    void UnitTestFlow::setGcodePath(const QString& strPath)
    {
        m_GcodePath = strPath;
        BaseSettings::setSettingValue("slicer_gcode_path", strPath);
        Q_EMIT slicerPathChanged();
    }

    void UnitTestFlow::sliceGcodeFrom3mf(const QString& _3mffile)
    {
        g_only_slice_enabled = true;
        m_enabled = true;
        QFileInfo _3mfInfo((_3mffile));
        if (!_3mfInfo.exists())
        {
            return;
        }
        getKernel()->cx3dManager()->openProject(_3mffile);
    }
     void UnitTestFlow::sliceGcodeFromCache(const QString &_fileName)
    {
        g_only_slice_enabled = true;
        m_enabled = true;
        QFileInfo fileInfo((_fileName));
        if (!fileInfo.exists())
        {
            return;
        }
        QString gcodeName = m_GcodePath + "/" + fileInfo.completeBaseName() + "_cache.gcode";
        getKernel()->sliceFlow()->slicerCacheToGcode(_fileName, gcodeName);
        QCoreApplication::quit();
    }
}
