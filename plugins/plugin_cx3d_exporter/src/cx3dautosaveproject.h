#ifndef CX3DAUTOSAVEPROJECT_H
#define CX3DAUTOSAVEPROJECT_H
#include <QtCore/QObject>
#include <QTimer>

class  Cx3dAutoSaveProject : public QObject
{
    Q_OBJECT
public:
    Cx3dAutoSaveProject(QObject* parent = nullptr);
    virtual ~Cx3dAutoSaveProject();
    static Cx3dAutoSaveProject*instance();
    void openProject(QString strFilePath = "");
    void setSettingsData(QString file);
    QString getProjectPath();
    void start();
    void startBackupTask();
    void stop();
    void run();

    Q_INVOKABLE void accept();
    Q_INVOKABLE void reject();
public slots:
    void updateTmpTime();
    void slotMinutesChanged(float nMinute);

private:
    QTimer* tmp_timer;
    QString m_strTempFilePath;
    float m_fSeconds;
    bool m_bExit = false;

};

#endif // CX3DAUTOSAVEPROJECT_H
