#ifndef UnitTestTool_H_
#define UnitTestTool_H_

#include <QVariant>
#include <QObject>
#include <QString>
#include <QPointer>
#include <QTranslator>
#include <QtQml/QQmlApplicationEngine>

class BaseSettings : public QObject
{
public:
    static QVariant getSettingValue(const QString& key, const QVariant& defaultValue);
    static void setSettingValue(const QString& key, const QVariant& value);
};

bool readErrorFiles(const QString dirPath, QString& resultMessage);
void loadText(const QString& fileName, QString& message);

class UnitTestTool : public QObject {
    
    Q_OBJECT
    Q_PROPERTY(QString _3mfPath READ slicer3MFPath WRITE set3MFPath NOTIFY mfPathChanged)
    Q_PROPERTY(QString slicerBLPath READ slicerBLPath WRITE setSlicerBLPath NOTIFY slicerBLPathChanged)
    Q_PROPERTY(QString compareSliceBLPath READ compareSliceBLPath WRITE setCompareSlicerBLPath NOTIFY slicerComparePathChanged)

    Q_PROPERTY(QString message READ message  NOTIFY messageChanged)
        
public:
    
    void initTranslator();
public:
  explicit UnitTestTool(QObject* parent = nullptr);

  virtual ~UnitTestTool();

  void setQmlEngine(QQmlEngine* engine);

  Q_INVOKABLE void changeLanguage(int index);

  QString slicer3MFPath() const;
  void set3MFPath(const QString& strPath);

  QString slicerBLPath() const;
  void setSlicerBLPath(const QString& strPath);

  QString compareSliceBLPath()const;
  void setCompareSlicerBLPath(const QString& strPath);

  QString message()const;
  Q_INVOKABLE void doTest(const int& type = 1);

Q_SIGNALS:
    void mfPathChanged();
    void slicerBLPathChanged();
    void slicerComparePathChanged();

    void messageChanged();
private:
    QString m_3mfPath;
    QString m_slicerBLPath;
    QString m_slicerComparePath;

    QString m_mess_text;
private:
    QString _languageTs;
    QTranslator* m_translator;
    QQmlEngine* m_qmlEngine;
};

#endif // !DUMPTOOL_DUMPTOOL_H_
