#ifndef SLICECOMMAND_1594085332399_H
#define SLICECOMMAND_1594085332399_H

#include<QObject>
#include"laserscene.h"
#include"laserplugin.h"

class LaserExportFile : public QObject
{
    Q_OBJECT
public:
    explicit LaserExportFile(QObject *parent = nullptr);
    virtual ~LaserExportFile();

    void setParentObj(LaserScene* laserScene)
    {
        m_laserScene = laserScene;
    };
    Q_INVOKABLE QObject* getLaserScene();
    void localFileSaveSuccess(QObject* object);
    Q_INVOKABLE void sliceUnPreview();
    int onExportSuccess(QObject* object);

    bool copyFile(QString srcPath, QString dstPath, bool coverFileIfExist);
signals:
    void successSliceShow(QObject *repotObject);
private :
    LaserScene* m_laserScene = nullptr;

    QObject* m_footStatus;


};

#endif // SLICECOMMAND_1594085332399_H
