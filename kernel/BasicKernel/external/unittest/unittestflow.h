#ifndef _NULLSPACE_UNITTEST_1590155449305_H
#define _NULLSPACE_UNITTEST_1590155449305_H
#include <QObject>
#include <QFileInfo>
#include <QFile>
#include "basickernelexport.h"
namespace creative_kernel
{
    enum SlicerUnitType
    {
        None = 0,
        Ganerate,
        Compare,
        Update
    };
    struct MyJsonMessage
    {
        QString path = "";
        QString startTime = "";
        QString endTime = "";
        QString result = "";
        QString reason = "";

    };
    class BaseSettings : public QObject
    {
    public:
        static QVariant getSettingValue(const QString& key, const QVariant& defaultValue);
        static void setSettingValue(const QString& key, const QVariant& value);
    };
    class BASIC_KERNEL_API BLCompareTestFlow :public QObject
    {
        Q_OBJECT
    public:
        //BLCompareTestFlow();
        //~BLCompareTestFlow() {}

        static void startBLTest();
        static void endBLTest();
        static void endBLTestFlowError(const QString & error);
        //
        static void nextBLTest();

        static bool enabled();
        static bool sliceCacheEnabled();
        static QString _3MFPath();
        static QString BLPath();
        static QString CompareResultPath();
        static void setEnabled(bool enable);
        static void setSliceCacheEnabled(bool enable);
        static void set3mfPath(const QString& path);
        static void setScpList(const QStringList& scpListPath);
        static void setBLPath(const QString&blpath);
        static void setCompareResultPath(const QString&path);
        static void setFuncType(int type);
        static bool messageEnabled();
        static void setMessageEnabled(bool enable);
        static void loadText(const QString& filename, QString& resultText);
        static QList<QFileInfo> entryFileInfoList(const QString& path);
        static bool removeResultCache();
        static void writeResultData();
        static void copyDirectoryStructureByFilter(const QString& filterName, const QString& sourcePath, QString& destinationPath);
    };
    class BASIC_KERNEL_API UnitTestFlow : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString slicerBaseLinePath READ slicerBaseLinePath WRITE setSlicerBaseLinePath NOTIFY slicerBaseLinePathChanged)
        Q_PROPERTY(QString compareErrorPath READ compareErrorPath WRITE setCompareErrorPath NOTIFY slicerCPErrorPathChanged)
    
        Q_PROPERTY(QString sliceCachePath READ sliceCachePath WRITE setSliceCachePath NOTIFY slicerPathChanged)
        Q_PROPERTY(QString gcodePath READ gcodePath WRITE setGcodePath NOTIFY slicerPathChanged)
    
        
    public:
        
        UnitTestFlow(QObject* parent = nullptr);
        virtual ~UnitTestFlow();

        void registerContext();

        void initialize();
        void uninitialize();

        Q_INVOKABLE int slicerUnitType()const
        {
            return static_cast<int>(m_unitType);
        }

        Q_INVOKABLE void generateTest();
        Q_INVOKABLE void compareTest();
        Q_INVOKABLE void uploadTest();
        
        QString slicerBaseLinePath() const;
        void setSlicerBaseLinePath(const QString& strPath);

        QString compareErrorPath()const;
        void setCompareErrorPath(const QString& strPath);

        QString sliceCachePath() const;
        void setSliceCachePath(const QString& strPath);

        QString gcodePath() const;
        void setGcodePath(const QString& strPath);

        Q_INVOKABLE void sliceGcodeFrom3mf(const QString &_3mffile);
        Q_INVOKABLE void sliceGcodeFromCache(const QString &_fileName);
    Q_SIGNALS:
        void slicerBaseLinePathChanged();
        void slicerCPErrorPathChanged();
        void slicerPathChanged();
    private:
        SlicerUnitType m_unitType;
        QString m_strSlicerBaseLinePath;
        QString m_strCompareErrorPath;

        QString m_SliceCachePath;
        QString m_GcodePath;
        QString m_3mfFile = "";
    };
}
#endif // _NULLSPACE_UNITTEST_1590155449305_H
