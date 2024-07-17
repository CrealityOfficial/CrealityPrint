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
#include <QCoreApplication>
#include <internal/project_cx3d/cx3dmanager.h>
namespace creative_kernel
{
    static QString m_3mfPath = "";
    static QString m_BlPath = "";
    static QString m_resultPath = "";
    static bool m_enabled = false;
    static int m_curFuncType = 1;     //0:generate 1:compare 2:update
    static int m_cur_file_index = 0;
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
     void BLCompareTestFlow::startBLTest()
    {
        if (!enabled())return;
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
        // 获取所有文件和目录
        QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot , QDir::Name);
        if (m_cur_file_index >= list.size())return;
        QFileInfo fileInfo = list.at(m_cur_file_index);

        // 如果是文件
        qDebug() << fileInfo.fileName();
        if (fileInfo.isFile()) {
            qDebug() << "Found file:" << fileInfo.absoluteFilePath();
            //cxkernel::openFileWithName(fileInfo.absoluteFilePath());
            if (fileInfo.suffix() == "3mf")
            {
                getKernel()->cx3dManager()->openProject(fileInfo.absoluteFilePath());
            }
            else
            {
                nextBLTest();
            }
        }
    }
    void BLCompareTestFlow::nextBLTest()
    {
        m_cur_file_index++;
        creative_kernel::setKernelPhase(KernelPhaseType::kpt_prepare);
        QDir dir(m_3mfPath);
        QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
        if (m_cur_file_index >= list.size())
        {
            QCoreApplication::quit();
            return;
        }
        creative_kernel::projectUI()->clearSecen();
        startBLTest();

    }
    bool BLCompareTestFlow::enabled()
    {
        return m_enabled;
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
    void BLCompareTestFlow::set3mfPath(const QString& path)
    {
        m_3mfPath = path;
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
    //void UnitTestFlow::_InitBaseLine()
    //{
    //    qDebug() << "_InitBaseLine()";
    //    Printer* pPrinter = getSelectedPrinter();
    //    SliceInput input;
    //    qDebug() << "produceSliceInput preStart";
    //    produceSliceInput(input, pPrinter);
    //    qDebug() << "produceSliceInput success";


    //}

   
}
