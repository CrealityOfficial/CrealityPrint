#ifndef _NULLSPACE_UNITTEST_1590155449305_H
#define _NULLSPACE_UNITTEST_1590155449305_H
#include <QObject>
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
        //
        static void nextBLTest();
        static bool enabled();
        static QString _3MFPath();
        static QString BLPath();
        static QString CompareResultPath();
        static void setEnabled(bool enable);
        static void set3mfPath(const QString& path);
        static void setBLPath(const QString&blpath);
        static void setCompareResultPath(const QString&path);
        static void setFuncType(int type);

    };
    class BASIC_KERNEL_API UnitTestFlow : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString slicerBaseLinePath READ slicerBaseLinePath WRITE setSlicerBaseLinePath NOTIFY slicerBaseLinePathChanged)
        Q_PROPERTY(QString compareErrorPath READ compareErrorPath WRITE setCompareErrorPath NOTIFY slicerCPErrorPathChanged)
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

    Q_SIGNALS:
        void slicerBaseLinePathChanged();
        void slicerCPErrorPathChanged();
    private:
        SlicerUnitType m_unitType;
        QString m_strSlicerBaseLinePath;
        QString m_strCompareErrorPath;
    };
}
#endif // _NULLSPACE_UNITTEST_1590155449305_H
