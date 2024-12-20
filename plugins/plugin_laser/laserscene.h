#ifndef LASERSCENE_H
#define LASERSCENE_H

#include <QObject>
#include <QtCore/QStandardPaths>
#include "drawobjectmodel.h"
#include "qtusercore/module/cxhandlerbase.h"
#include "laserimgprovider.h"
#include "lasersettings.h"
#include "plottersettings.h"
class CX2DLib;
class LaserExportFile;
class PlotterUndo;
class LaserScene : public QObject, 
    public qtuser_core::CXHandleBase
{
    Q_OBJECT
    Q_PROPERTY(DrawObjectModel * drawObjectModel READ drawObjectModel)
    Q_PROPERTY(int width READ width CONSTANT)
    Q_PROPERTY(int height READ height CONSTANT)
    Q_PROPERTY(QPoint origin READ origin WRITE setOrigin NOTIFY originChanged)
public:
    enum SCENETYPE {
        LASER,
        PLOTTER
    };
    LaserScene(QObject* parent,SCENETYPE type);
    virtual ~LaserScene();

    void uninitialize();

    Q_INVOKABLE DrawObjectModel *drawObjectModel();
    Q_INVOKABLE QObject* laserSettings();
    //Q_INVOKABLE void addDragRectObject(QObject *obj, int sharpType);
    Q_INVOKABLE void addDragRectObject(QObject* obj, int sharpType);
    void setAddRectObject(QString sharpName, QObject* obj);
    void addDragRectObject(QObject* obj, QString name);
    QString getTheDragRectObjectName(int sharpType);
    Q_INVOKABLE void addRedoRectObject(QObject* obj, QString sharpName);
    Q_INVOKABLE void objectChanged(QObject* obj, long long oldX, long long oldY, double oldWidth, double oldHeight, double oldRatation,
        long long newX, long long newY, double newWidth, double newHeight, double newRatation);
    void setObjectChangedData(QObject* obj, long long x, long long y, double width, double height, double ratition);
    Q_INVOKABLE QObject* selectObject(int index);
    Q_INVOKABLE QObject* selectMulObject(QList<int> indexList);
    Q_INVOKABLE void setSelectObject(QObject *obj);
    Q_INVOKABLE void setSelectMulObject(QList<QObject*> objectList);
    Q_INVOKABLE void clearSelectAllObject();
    Q_INVOKABLE void setClearSelectAllObject();
    Q_INVOKABLE void open();
    Q_INVOKABLE void vectorImage(QObject *obj,int value=127);
    Q_INVOKABLE void DitheringImage(QObject* obj, int value=127);
    Q_INVOKABLE void blackImage(QObject *obj,int value=127);
    Q_INVOKABLE void grayImage(QObject *obj,int value=127);
    Q_INVOKABLE void imageThresholdChanged(QObject *obj,int threshold);
    Q_INVOKABLE void imageOriginalShowChanged(QObject* obj, bool value);
    Q_INVOKABLE void imageReverseChanged(QObject* obj, bool value);
    Q_INVOKABLE void m4modeChanged(QObject* obj, bool value);
    Q_INVOKABLE void imageContrastChanged(QObject* obj, int value);
    Q_INVOKABLE void imageBrightnessChanged(QObject* obj, int value);
    Q_INVOKABLE void imageWhiteValueChanged(QObject* obj, int value);
    Q_INVOKABLE void imageFlipModelValueChanged(QObject* obj, int value);
    Q_INVOKABLE void genLaserGcode();
    Q_INVOKABLE void deleteObject(QObject *obj);
    Q_INVOKABLE void deleteMulObject(QList<QObject*> objList);
    Q_INVOKABLE void deleteUndoObject(QObject* obj);
    Q_INVOKABLE QString getMessageText() { return m_strMessageText; }
    Q_INVOKABLE void saveLocalGcodeFile();
    Q_INVOKABLE QStringList getFontFamilys();
    Q_INVOKABLE QStringList getFontFamilysStyles(QString family);
    Q_INVOKABLE void accept();

    void createImageObject(const QString &filename);
    int width();
    int height();
    QPoint origin() {return m_origin;}
    void setOrigin(QPoint origin) {m_origin=origin;}
    QString filter() override;
    QString filterKey() override;
    void handle(const QString& fileName) override;
    void handle(const QStringList& fileNames) override;
    void detectObject(QObject* obj, long long newX, long long newY, double newWidth, double newHeight, double newRatation);
    bool saveGcode(const QString& fileNmae);
    void saveLocalGcodeFileSuccess();
    void saveLocalGcodeFileFail();
    QString getTmpFilePeth();
signals:
    void originChanged();
public slots:
    void onGenGcodeSuccess(QString gcdoeBuf);
    void onFDMView();
    void onLaserView();
    void onPlotterView();

    void slotLanguageChanged();
private:
    void updateImage(QObject *obj);

private:
    DrawObjectModel *m_pDrawObjectModel;
    QObject* m_rightPanel;
    QObject* m_laserscene;
    LaserSettings* m_pLaserSettings;  
    PlotterSettings* m_pPlotterSettings;
    LaserImgProvider *m_imgProvider;
    CX2DLib* m_3in1exporter;
    QString m_gcodeBuf = "";
    QString m_prefixString = "";
    QString m_tailCode = "";
    QString m_strMessageText = "";
    SCENETYPE m_type = SCENETYPE::LASER;
    QString m_strTmpFilePath = "";
    LaserExportFile* m_laserExport= nullptr;
    QPoint m_origin;
    int sharpCnt;
    PlotterUndo* m_plotterUndo;
    PlotterUndo* m_LaserUndo;
    QObject* m_switchModel = NULL;

    bool m_tempSave;
};

#endif // LASERSCENE_H
