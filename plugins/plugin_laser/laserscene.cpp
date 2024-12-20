#include <QtQml/QQmlProperty>
#include <QtDebug>
#include <QPainter>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QQmlApplicationEngine>
#include <QFileInfo>
#include <QFontDatabase>
#include <QSettings>
#include <QQmlContext>
#include <math.h>
#include <QQuickView>

#include "laserscene.h"
#include "drawobject.h"
#include "laserexportfile.h"
#include "qtusercore/string/stringtool.h"
#include "interface/camerainterface.h"
#include "interface/selectorinterface.h"
#include "interface/spaceinterface.h"
//#include "interface/modelinterface.h"
#include "cxkernel/interface/undointerface.h"
#include "interface/uiinterface.h"
#include "interface/commandinterface.h"
#include "cxkernel/interface/iointerface.h"
#include "interface/machineinterface.h"
#include "qtusercore/property/qmlpropertysetter.h"
#include "qtusercore/module/systemutil.h"
#include"c2dlib.h"
#include"lasergcodework.h"
#include "cxkernel/interface/jobsinterface.h"
#include"c2dlib.h"

#include "kernel/kernel.h"
#include "data/modelspaceundo.h"
#include "PlotterUndo.h"
#include "kernel/translator.h"
#include "kernel/kernelui.h"


#define LASER_IMAGE_PROVIDER "image://laserImgProvider/"
#define PLOTTER_IMAGE_PROVIDER "image://plotterImgProvider/"
using namespace creative_kernel;
using namespace qtuser_3d;
using namespace qtuser_qml;

QRect getModuleRect(QImage* previewImage);
QImage createImageWithOverlay(const QImage& baseImage, const QImage& overlayImage);
LaserScene::LaserScene(QObject* parent, SCENETYPE type)
    : QObject(parent)
    , m_tempSave(false)
{
    m_type = type;
    sharpCnt = 0;
    m_plotterUndo = new PlotterUndo(this);
    m_LaserUndo = new PlotterUndo(this);
    m_pDrawObjectModel = new DrawObjectModel(this);
    m_imgProvider = new LaserImgProvider();
    m_pLaserSettings = new LaserSettings();
    m_pPlotterSettings = new PlotterSettings();
    m_3in1exporter = new CX2DLib();
    m_laserExport = new LaserExportFile();
    m_laserExport->setParentObj(this);
    QQmlEngine* engine = getKernelUI()->getQmlEngine();

    m_rightPanel = getKernelUI()->appWindow();
    //m_rightPanel = engine->rootObjects().first();
    if(m_type==SCENETYPE::LASER)
    {
        engine->rootContext()->setContextProperty("laserScene", this);
        if(m_rightPanel)
        {
            QObject* pObjLaserPanel = m_rightPanel->findChild<QObject*>("objLaserItem");
            if(pObjLaserPanel)
            {
                writeObjectProperty(pObjLaserPanel, "control", this);
                writeObjectProperty(pObjLaserPanel, "settingsModel", this->m_pLaserSettings);
            }
        }
        m_laserscene = getKernelUI()->getUI("glmainview")->parent()->findChild<QObject*>("laserView");
        if(m_laserscene)
        {
            writeObjectProperty(m_laserscene, "control", this);
            writeObjectProperty(m_laserscene, "itemModel", this->drawObjectModel());
        }
        registerImageProvider(QLatin1String("laserImgProvider"), m_imgProvider);
        m_imgProvider->setProviderUrl(LASER_IMAGE_PROVIDER);
    }
    if(m_type==SCENETYPE::PLOTTER)
    {
        if(m_rightPanel)
        {
            QObject* pObjLaserPanel = m_rightPanel->findChild<QObject*>("objPlotterItem");
            if(pObjLaserPanel)
            {
                writeObjectProperty(pObjLaserPanel, "control", this);
                writeObjectProperty(pObjLaserPanel, "settingsModel", this->m_pPlotterSettings);
            }
        }
        m_laserscene = getKernelUI()->getUI("glmainview")->parent()->findChild<QObject*>("plotterView");
        if(m_laserscene)
        {
            writeObjectProperty(m_laserscene, "control", this);
            writeObjectProperty(m_laserscene, "itemModel", this->drawObjectModel());
        }

        registerImageProvider(QLatin1String("plotterImgProvider"), m_imgProvider);
        m_imgProvider->setProviderUrl(PLOTTER_IMAGE_PROVIDER);
    }

    cxkernel::registerOpenHandler(this);
    onFDMView();

    slotLanguageChanged();
}

LaserScene::~LaserScene()
{
    delete m_pDrawObjectModel;
}

void LaserScene::uninitialize()
{
    if (m_type == SCENETYPE::LASER)
    {
        removeImageProvider(QLatin1String("laserImgProvider"));
    }
    if (m_type == SCENETYPE::PLOTTER)
    {
        removeImageProvider(QLatin1String("plotterImgProvider"));
    }
}

DrawObjectModel *LaserScene::drawObjectModel()
{
    return m_pDrawObjectModel;
}

QObject *LaserScene::laserSettings()
{
    return m_pLaserSettings;
}

void LaserScene::addDragRectObject(QObject* obj, int sharpType)
{
    QString name = getTheDragRectObjectName(sharpType);
    addDragRectObject(obj, name);
    if (m_type == SCENETYPE::LASER)
    {
        m_LaserUndo->addDragSharp(this, obj, name, true);
    }
    else
    {
        m_plotterUndo->addDragSharp(this, obj, name, true);
    }
}
void LaserScene::setAddRectObject(QString sharpName, QObject* obj)
{
    QMetaObject::invokeMethod(m_laserscene, "addRectObject",
                              Q_ARG(QVariant, QVariant::fromValue(sharpName)),
                              Q_ARG(QVariant, QVariant::fromValue<QObject*>(obj)));
}
void LaserScene::addDragRectObject(QObject* obj, QString name)
{
    DrawObject* pDrawObject = new DrawObject(this);
    pDrawObject->setName(name);
    pDrawObject->setQmlObject(obj);
    m_pDrawObjectModel->addDrawObject(pDrawObject);
}
QString LaserScene::getTheDragRectObjectName(int sharpType)
{
    QString strSharpName = "";
    switch (sharpType)
    {
    case 1:
        strSharpName = "Sharp";
        break;
    case 2:
        strSharpName = "Ellipse";
        break;
    case 4:
        strSharpName = "Path";
        break;
    case 5:
        strSharpName = "Text";
        break;
    default:
        break;
    }

    strSharpName = strSharpName + QString::number(sharpCnt);
    sharpCnt++;
    return strSharpName;
}
void LaserScene::addRedoRectObject(QObject* obj, QString sharpName)
{
    addDragRectObject(obj, sharpName);
}
void LaserScene::objectChanged(QObject* obj, long long oldX, long long oldY, double oldWidth, double oldHeight, double oldRatation,
                               long long newX, long long newY, double newWidth, double newHeight, double newRatation)
{
    if (m_type == SCENETYPE::LASER)
    {
        m_LaserUndo->addObjectChanged(this, obj, oldX, oldY, oldWidth, oldHeight, oldRatation,
                                      newX, newY, newWidth, newHeight, newRatation, true);
        detectObject(obj, newX, newY, newWidth, newHeight, newRatation);
    }
    else
    {
        m_plotterUndo->addObjectChanged(this, obj, oldX, oldY, oldWidth, oldHeight, oldRatation,
                                        newX, newY, newWidth, newHeight, newRatation, true);
        detectObject(obj, newX, newY, newWidth, newHeight, newRatation);
    }
    
}
void LaserScene::setObjectChangedData(QObject* obj, long long x, long long y, double width,
                                      double height, double ratition)
{
    QQmlProperty::write(obj, "x", x);
    QQmlProperty::write(obj, "y", y);
    QQmlProperty::write(obj, "width", width);
    QQmlProperty::write(obj, "height", height);
    QQmlProperty::write(obj, "rotation", ratition);
}
QObject* LaserScene::selectObject(int index)
{
    //qDebug()<<index;
    DrawObject *pDrawObject = m_pDrawObjectModel->getData(index);
    if(pDrawObject)
    {
        if(pDrawObject->qmlObject())
        {
            QMetaObject::invokeMethod(pDrawObject->qmlObject()->parent(), "onRectSelected",Q_ARG(QVariant, QVariant::fromValue(pDrawObject->qmlObject())));
            setSelectObject(pDrawObject->qmlObject());
            return pDrawObject->qmlObject();
        }
    }
    return nullptr;
}

QObject* LaserScene::selectMulObject(QList<int> indexList)
{
  if (indexList.empty()) {
    clearSelectAllObject();
    setClearSelectAllObject();
    return nullptr;
  }

  if (indexList.size() == 1) {
    return selectObject(indexList.front());
  }

  QList<QObject*> drawObjectList;
  for (size_t i = 0; i < indexList.size(); i++) {
      DrawObject* pDrawObject = m_pDrawObjectModel->getData(indexList[i]);
      if (pDrawObject && pDrawObject->qmlObject()) {
          drawObjectList.append(pDrawObject->qmlObject());
      }
  }

  DrawObject* pDrawObject = m_pDrawObjectModel->getData(indexList[indexList.size() - 1]);
  if (pDrawObject && pDrawObject->qmlObject()) {
      QMetaObject::invokeMethod(pDrawObject->qmlObject()->parent(), "onRectMulSelected", Q_ARG(QVariant, QVariant::fromValue(drawObjectList)));
      return pDrawObject->qmlObject();
  }

  return nullptr;
}

void LaserScene::createImageObject(const QString &filename)
{

    QVariant returnedValue;
    //QMetaObject::invokeMethod(m_laserscene, "createImageSharp");
    QMetaObject::invokeMethod(m_laserscene, "createImageSharp",Q_RETURN_ARG(QVariant, returnedValue));
    QSharedPointer<us::USettings> globalsetting(createCurrentGlobalSettings());
    int machine_width = globalsetting->settings().value("machine_width")->toInt();
    int machine_height = globalsetting->settings().value("machine_depth")->toInt();
    QString Machine = currentMachineName();
    if (Machine == "Ender-3 V2 Laser") {
        machine_width = machine_width - 60;
    }
    QObject* obj = qvariant_cast<QObject *>(returnedValue);
    QString showFileName = "";
    int imageW = 0;
    int imageH = 0;

    if(obj)
    {
        DrawObject *pDrawObject = new DrawObject(this);
        showFileName = filename.mid(filename.lastIndexOf("/")+1);
        pDrawObject->setName(showFileName);
        pDrawObject->setQmlObject(obj);
        QImage imgdata;
        if (showFileName.indexOf(".dxf") >= 0 || showFileName.indexOf(".svg") >= 0)
        {
            std::string strname = filename.toLocal8Bit().constData();
            {
                QImage imgload(filename);
                QRect moduleRect = getModuleRect(&imgload);
                imgload = imgload.copy(moduleRect);
                QImage baseImageDst_mask = QImage(imgload.size(), QImage::Format_ARGB32);
                baseImageDst_mask.fill(QColor(Qt::white));
                imgdata = createImageWithOverlay(baseImageDst_mask, imgload);
            }
        }
        else
        {
            imgdata.load(filename);
        }

        QImage scaled_image = imgdata.scaled(machine_width * 3 - 15, machine_height * 3 - 15, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        imageW = scaled_image.width();
        imageH = scaled_image.height();
        pDrawObject->setOriginImageData(imgdata);
        //int width = imgdata.widthMM();
        //int height = imgdata.heightMM();
        QQmlProperty::write(obj, "width", imageW);
        QQmlProperty::write(obj, "height", imageH);
        int w = this->width();//660
        int h = this->height();//660
        float x = (w- imageW)/2;
        float y = (h- imageH)/2+ imageH;
        QQmlProperty::write(obj, "x", x);
        QQmlProperty::write(obj, "y", y);
        QString id = QString::number((std::uintptr_t)(obj));
        m_imgProvider->setObject(id,pDrawObject);

        QQmlProperty::write(obj, "imageurl", m_imgProvider->providerUrl()+id);

        QQmlProperty::write(obj, "imageW", imageW);
        QQmlProperty::write(obj, "imageH", imageH);
        QQmlProperty::write(obj, "imageName", showFileName);

        m_pDrawObjectModel->addDrawObject(pDrawObject);
        //delete dataImg;
        if (m_type == SCENETYPE::LASER)
        {
            blackImage(obj, 168);
        }
        else
        {
            vectorImage(obj, 168);
        }
        selectObject(m_pDrawObjectModel->getIndex(obj));
        if (m_type == SCENETYPE::LASER)
        {
            m_LaserUndo->addDragSharp(this, obj, showFileName, true);
        }
        else
        {
            m_plotterUndo->addDragSharp(this, obj, showFileName, true);
        }
    }
    if (m_type == SCENETYPE::LASER)
    {
        QObject* pObjLaserPanel = m_rightPanel->findChild<QObject*>("objLaserItem");
        QMetaObject::invokeMethod(pObjLaserPanel, "initImageSettings");
    }
    else
    {
        QObject* pObjLaserPanel = m_rightPanel->findChild<QObject*>("objPlotterItem");
        QMetaObject::invokeMethod(pObjLaserPanel, "initImageSettings");
    }
}
void LaserScene::setSelectObject(QObject *obj)
{
    if (m_type == SCENETYPE::LASER) {
        QObject* pObjLaserPanel = m_rightPanel->findChild<QObject*>("objLaserItem");
        if (pObjLaserPanel) {
            QQmlProperty::write(pObjLaserPanel, "selShape", QVariant::fromValue(obj));
        }
    } else {
        QObject* pObjPlotterItem = m_rightPanel->findChild<QObject*>("objPlotterItem");
        if (pObjPlotterItem) {
            QQmlProperty::write(pObjPlotterItem, "selShape", QVariant::fromValue(obj));
        }
    }

    QList<int> indexList{ m_pDrawObjectModel->getIndex(obj) };
    QMetaObject::invokeMethod(m_laserscene, "syncSelectedIndexList", Q_ARG(QVariant, QVariant::fromValue(indexList)));
}
void LaserScene::setSelectMulObject(QList<QObject*> objectList)
{
    if (objectList.empty()) {
        clearSelectAllObject();
        setClearSelectAllObject();
        return;
    }

    if (objectList.size() == 1) {
        setSelectObject(objectList.front());
        return;
    }

    if (m_type == SCENETYPE::LASER) {
        QObject* pObjLaserPanel = m_rightPanel->findChild<QObject*>("objLaserItem");
        if (pObjLaserPanel) {
            QQmlProperty::write(pObjLaserPanel, "selShape", QVariant::fromValue(nullptr));
        }
    } else {
        QObject* pObjLaserPanel = m_rightPanel->findChild<QObject*>("objPlotterItem");
        if (pObjLaserPanel) {
            QQmlProperty::write(pObjLaserPanel, "selShape", QVariant::fromValue(nullptr));
        }
    }

    QList<int> indexList;
    for (int i = 0; i < objectList.size(); i++) {
        indexList.append(m_pDrawObjectModel->getIndex(objectList[i]));
    }
    QMetaObject::invokeMethod(m_laserscene, "syncSelectedIndexList", Q_ARG(QVariant, QVariant::fromValue(indexList)));
}
void LaserScene::clearSelectAllObject()
{
    DrawObject* pDrawObject = m_pDrawObjectModel->getData(0);
    if (pDrawObject && pDrawObject->qmlObject()) {
        QMetaObject::invokeMethod(pDrawObject->qmlObject()->parent(), "clearSelectedStatus");
    }
}
void LaserScene::setClearSelectAllObject()
{
    if (m_type == SCENETYPE::LASER) {
        QObject* pObjLaserPanel = m_rightPanel->findChild<QObject*>("objLaserItem");
        if (pObjLaserPanel) {
            QQmlProperty::write(pObjLaserPanel, "selShape", QVariant::fromValue(nullptr));
        }
    } else {
        QObject* pObjLaserPanel = m_rightPanel->findChild<QObject*>("objPlotterItem");
        if (pObjLaserPanel) {
            QQmlProperty::write(pObjLaserPanel, "selShape", QVariant::fromValue(nullptr));
        }
    }

    QMetaObject::invokeMethod(m_laserscene, "syncSelectedIndexList", Q_ARG(QVariant, QVariant::fromValue(QList<int>{})));
}
int LaserScene::width()
{
    QSharedPointer<us::USettings> globalsetting(createCurrentGlobalSettings());
    int machine_width = globalsetting->settings().value("machine_width")->toInt();
    QString Machine = currentMachineName();
    if (Machine == "Ender-3 V2 Laser")
    {
        machine_width = machine_width - 60;
    }
    return machine_width*3;
}
int LaserScene::height()
{
    QSharedPointer<us::USettings> globalsetting(createCurrentGlobalSettings());
    int machine_height = globalsetting->settings().value("machine_depth")->toInt();
    return machine_height*3;
}

void LaserScene::open()
{
    m_tempSave = false;
    cxkernel::openFile();
}

void LaserScene::handle(const QString& fileName)
{
    QFileInfo info(fileName);
    QString sufixx = info.suffix();
    if ("gcode" == sufixx) //save gcode
    {
        if (this->saveGcode(fileName))
        {
            saveLocalGcodeFileSuccess();
        }
        else
        {
            saveLocalGcodeFileFail();
        }

        m_tempSave = false;
    }
    else
    {
        this->createImageObject(fileName);
    }
}
void LaserScene::handle(const QStringList& fileNames)
{
    this->createImageObject(fileNames[0]);
}
void LaserScene::detectObject(QObject* obj, long long newX, long long newY, double newWidth, double newHeight, double newRatation)
{
    double platformMinX = m_origin.x();
    double platformMaxX = platformMinX + this->width();
    double platformMaxY = m_origin.y();
    double platformMinY = platformMaxY - this->height();

    double shapeMinX = newX;
    double shapeMaxX = shapeMinX + newWidth;
    double shapeMinY = newY;
    double shapeMaxY = shapeMinY + newHeight;

    //左下角
    double xLR = shapeMinX, yLR = shapeMinY;
    //左上角
    double xLF = shapeMinX, yLF = shapeMaxY;
    //右上角
    double xRF = shapeMaxX, yRF = shapeMaxY;
    //右下角
    double xRR = shapeMaxX, yRR = shapeMinY;

    //中心点
    double xCenter = shapeMinX + newWidth / 2, yCenter = shapeMinY + newHeight / 2;

    //旋转后的坐标
    double r = newRatation * 3.141592654 / 180;
    double newX1 = (xLR - xCenter) * cos(-r) - (yLR - yCenter) * sin(-r) + xCenter;
    double newY1 = (yLR - yCenter) * cos(-r) + (xLR - xCenter) * sin(-r) + yCenter;

    double newX2 = (xLF - xCenter) * cos(-r) - (yLF - yCenter) * sin(-r) + xCenter;
    double newY2 = (yLF - yCenter) * cos(-r) + (xLF - xCenter) * sin(-r) + yCenter;

    double newX3 = (xRF - xCenter) * cos(-r) - (yRF - yCenter) * sin(-r) + xCenter;
    double newY3 = (yRF - yCenter) * cos(-r) + (xRF - xCenter) * sin(-r) + yCenter;

    double newX4 = (xRR - xCenter) * cos(-r) - (yRR - yCenter) * sin(-r) + xCenter;
    double newY4 = (yRR - yCenter) * cos(-r) + (xRR - xCenter) * sin(-r) + yCenter;

    //判断
    bool bInRect1 = (platformMinX - newX1) * (platformMaxX - newX1) <= 0 && (platformMinY - newY1) * (platformMaxY - newY1) <= 0;
    bool bInRect2 = (platformMinX - newX2) * (platformMaxX - newX2) <= 0 && (platformMinY - newY2) * (platformMaxY - newY2) <= 0;
    bool bInRect3 = (platformMinX - newX3) * (platformMaxX - newX3) <= 0 && (platformMinY - newY3) * (platformMaxY - newY3) <= 0;
    bool bInRect4 = (platformMinX - newX4) * (platformMaxX - newX4) <= 0 && (platformMinY - newY4) * (platformMaxY - newY4) <= 0;

    if (!bInRect1 || !bInRect2 || !bInRect3 || !bInRect4)
    {
        //qDebug() << "Out of platform";
        QMetaObject::invokeMethod(obj, "showWarningColor", Q_ARG(QVariant, QVariant::fromValue(true)));
    }
    else
    {
        QMetaObject::invokeMethod(obj, "showWarningColor", Q_ARG(QVariant, QVariant::fromValue(false)));
    }
}
QString LaserScene::filter()
{
    QString _filter = QString("Image File (*.jpg *.bmp *.png *.dxf *.svg)");
    if (m_tempSave)
    {
        _filter = QString("GCode File (*.gcode)");
        m_tempSave = false;
    }
    return _filter;
}
QString LaserScene::filterKey()
{
    if(m_type == SCENETYPE::LASER)
        return "laser";
    if(m_type == SCENETYPE::PLOTTER)
        return "plotter";
    return "";
}

void LaserScene::onFDMView()
{
    cxkernel::setUndoStack(getKernel()->modelSpaceUndo());
}

void LaserScene::onLaserView()
{
    if (m_type == SCENETYPE::LASER)
    {
        cxkernel::setUndoStack(m_LaserUndo);
    } 
}

void LaserScene::onPlotterView()
{
    if (m_type == SCENETYPE::PLOTTER)
    {
        cxkernel::setUndoStack(m_plotterUndo);
    }
}

void LaserScene::slotLanguageChanged()
{
    if (m_switchModel)
    {
        QSettings setting;
        setting.beginGroup("language_perfer_config");
        QString lang = setting.value("language_type", "en.ts").toString();
        setting.endGroup();

        int langType = 0;
        if (lang == "en.ts")
        {
            langType = 0;
        }
        else
        {
            langType = 1;
        }
        QMetaObject::invokeMethod(m_switchModel, "onChangeLanguage", Q_ARG(QVariant, QVariant::fromValue(langType)));
    }
}
void LaserScene::updateImage(QObject *obj)
{
    QQmlProperty::write(obj, "imageurl", "");
    
    QString id = QString::number((std::uintptr_t)(obj));
    QQmlProperty::write(obj, "imageurl", m_imgProvider->providerUrl()+id);
    QMetaObject::invokeMethod(obj, "setAnimation", Q_ARG(QVariant, QVariant::fromValue(false)));
}
void LaserScene::vectorImage(QObject *obj,int value)
{
    QQmlProperty::write(obj, "filpModelValue", 0);
    int filpModelValue = QQmlProperty::read(obj, "filpModelValue").toInt();
    if (filpModelValue > 0) imageFlipModelValueChanged(obj, filpModelValue);
    DrawObject* drawObj = m_pDrawObjectModel->getDataByQmlObj(obj);
    drawObj->setImgType("vector");
    bool originalShow = QQmlProperty::read(obj, "originalShow").toBool();
    if (originalShow) return;
    value = QQmlProperty::read(obj, "threshold").toInt();
    //1.image 2 svg
    QMetaObject::invokeMethod(drawObj->qmlObject(), "setAnimation", Q_ARG(QVariant, QVariant::fromValue(true)));
    QPainterPath path;
    QImage* grad_image = new QImage;
    QImage* black_image = new QImage;
    QImage img = drawObj->originImageData();
    *black_image = CX2DLib::CXBlackWhitePicture(img, value);
    *grad_image = CX2DLib::CXImage2VectorImage(*black_image, path);
    drawObj->setPath(path);
    drawObj->setBigImageData(*grad_image);
    int width = QQmlProperty::read(obj, "width").toInt();
    int height = QQmlProperty::read(obj, "height").toInt();
    *grad_image = grad_image->scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    drawObj->setImageData(*grad_image);
    updateImage(obj);
    delete  grad_image;
    delete  black_image;
    bool reverse = QQmlProperty::read(obj, "reverse").toBool();
    if (reverse) imageReverseChanged(obj, reverse);
}

void LaserScene::DitheringImage(QObject* obj, int value)
{
    QQmlProperty::write(obj, "filpModelValue", 0);
    int filpModelValue = QQmlProperty::read(obj, "filpModelValue").toInt();
    if (filpModelValue > 0) imageFlipModelValueChanged(obj, filpModelValue);
    DrawObject* drawObj = m_pDrawObjectModel->getDataByQmlObj(obj);
    drawObj->setImgType("Dithering");
    bool originalShow = QQmlProperty::read(obj, "originalShow").toBool();
    if (originalShow) return;
    value = QQmlProperty::read(obj, "threshold").toInt();
    //1.image 2 svg
    QMetaObject::invokeMethod(drawObj->qmlObject(), "setAnimation", Q_ARG(QVariant, QVariant::fromValue(true)));
    QImage* grad_image = new QImage;
    QImage img = drawObj->originImageData();
    *grad_image = CX2DLib::CXDitheringPicture(img, value);
    drawObj->setBigImageData(*grad_image);
    int width = QQmlProperty::read(obj, "width").toInt();
    int height = QQmlProperty::read(obj, "height").toInt();
    *grad_image = grad_image->scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    drawObj->setImageData(*grad_image);
    updateImage(obj);
    delete  grad_image;
    bool reverse = QQmlProperty::read(obj, "reverse").toBool();
    if (reverse) imageReverseChanged(obj, reverse);
}

void LaserScene::blackImage(QObject *obj,int value)
{
    QQmlProperty::write(obj, "filpModelValue", 0);
    int filpModelValue = QQmlProperty::read(obj, "filpModelValue").toInt();
    if (filpModelValue > 0) imageFlipModelValueChanged(obj, filpModelValue);
    DrawObject* drawObj = m_pDrawObjectModel->getDataByQmlObj(obj);
    drawObj->setImgType("black");
    bool originalShow = QQmlProperty::read(obj, "originalShow").toBool();
    if (originalShow) return;
    value = QQmlProperty::read(obj, "threshold").toInt();
    QMetaObject::invokeMethod(drawObj->qmlObject(), "setAnimation", Q_ARG(QVariant, QVariant::fromValue(true)));
    QImage* grad_image = new QImage;
    // *grad_image = drawObj->originImageData().convertToFormat(QImage::Format_Grayscale8,Qt::AutoColor);
    QImage img = drawObj->originImageData();
    *grad_image = CX2DLib::CXBlackWhitePicture(img, value);
    drawObj->setBigImageData(*grad_image);
    int width = QQmlProperty::read(obj, "width").toInt();
    int height = QQmlProperty::read(obj, "height").toInt();
    *grad_image = grad_image->scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    drawObj->setImageData(*grad_image);
    updateImage(obj);
    delete  grad_image;
    bool reverse = QQmlProperty::read(obj, "reverse").toBool();
    if (reverse) imageReverseChanged(obj, reverse);
}

void LaserScene::grayImage(QObject *obj,int value)
{
    QQmlProperty::write(obj, "filpModelValue", 0);
    int filpModelValue = QQmlProperty::read(obj, "filpModelValue").toInt();
    if (filpModelValue > 0) imageFlipModelValueChanged(obj, filpModelValue);
    DrawObject* drawObj = m_pDrawObjectModel->getDataByQmlObj(obj);
    drawObj->setImgType("gray");
    bool originalShow = QQmlProperty::read(obj, "originalShow").toBool();
    if (originalShow) return;
    value = QQmlProperty::read(obj, "threshold").toInt();
    int contrastValue = QQmlProperty::read(obj, "contrastValue").toInt();
    int brightValue = QQmlProperty::read(obj, "brightnessValue").toInt();
    int setWhiteVal = QQmlProperty::read(obj, "whiteValue").toInt();
    if (setWhiteVal > 230) setWhiteVal = 230;
    QMetaObject::invokeMethod(drawObj->qmlObject(), "setAnimation", Q_ARG(QVariant, QVariant::fromValue(true)));
    QImage *grad_image = new QImage;
    // *grad_image = drawObj->originImageData().convertToFormat(QImage::Format_Grayscale8,Qt::AutoColor);
    QImage img = drawObj->originImageData();
    *grad_image = CX2DLib::CXGrayScalePicture(img, value, contrastValue, brightValue, setWhiteVal);
    drawObj->setBigImageData(*grad_image);
    int width = QQmlProperty::read(obj, "width").toInt();
    int height = QQmlProperty::read(obj, "height").toInt();
    *grad_image = grad_image->scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    drawObj->setImageData(*grad_image);
    updateImage(obj);
    delete  grad_image;
    bool reverse = QQmlProperty::read(obj, "reverse").toBool();
    if (reverse) imageReverseChanged(obj, reverse);
}
void LaserScene::imageThresholdChanged(QObject *obj,int threshold)
{
    DrawObject* drawObj = m_pDrawObjectModel->getDataByQmlObj(obj);
    QString imgType = drawObj->type();
    if ("gray" == imgType)
    {
        this->grayImage(obj,threshold);
    }
    else if ("black" == imgType)
    {
        this->blackImage(obj, threshold);
    }
    else if ("vector" == imgType)
    {
        this->vectorImage(obj, threshold);
    }
    else if ("Dithering" == imgType)
    {
        this->DitheringImage(obj, threshold);
    }
}
void LaserScene::imageOriginalShowChanged(QObject* obj, bool value)
{
    QQmlProperty::write(obj, "filpModelValue", 0);
    QQmlProperty::write(obj, "originalShow", value);
    if (value)
    {
        QImage* dst_image = new QImage;
        DrawObject* drawObj = m_pDrawObjectModel->getDataByQmlObj(obj);
        QMetaObject::invokeMethod(drawObj->qmlObject(), "setAnimation", Q_ARG(QVariant, QVariant::fromValue(true)));
        *dst_image = drawObj->originImageData();
        drawObj->setImageData(*dst_image);
        updateImage(obj);
        delete  dst_image;
    }
    else
    {
        int threshold = QQmlProperty::read(obj, "threshold").toInt();
        imageThresholdChanged(obj, threshold);
        bool reverse = QQmlProperty::read(obj, "reverse").toBool();
        if (reverse) imageReverseChanged(obj, reverse);
    }
}
void LaserScene::imageReverseChanged(QObject* obj, bool value)
{
    QQmlProperty::write(obj, "filpModelValue", 0);
    QQmlProperty::write(obj, "reverse", value);
    bool originalShow = QQmlProperty::read(obj, "originalShow").toBool();
    if (originalShow) return;
    QImage* dst_image = new QImage;
    DrawObject* drawObj = m_pDrawObjectModel->getDataByQmlObj(obj);
    QMetaObject::invokeMethod(drawObj->qmlObject(), "setAnimation", Q_ARG(QVariant, QVariant::fromValue(true)));
    *dst_image = drawObj->bigImageData();
    if (value)
    {
        int threshold = QQmlProperty::read(obj, "threshold").toInt();
        QString imgType = drawObj->type();
        if ("vector" == imgType)
        {
            *dst_image = drawObj->originImageData();
            QImage gray_image = CX2DLib::CXBlackWhitePicture(*dst_image, threshold);
            *dst_image = CX2DLib::CXReversalImgae(gray_image);
            QPainterPath path;
            *dst_image = CX2DLib::CXImage2VectorImage(*dst_image, path);
            drawObj->setPath(path);
        }
        else
        {
            *dst_image = CX2DLib::CXReversalImgae(*dst_image);
        }
    }
    drawObj->setDstImageData(*dst_image);
    int width = QQmlProperty::read(obj, "width").toInt();
    int height = QQmlProperty::read(obj, "height").toInt();
    *dst_image = dst_image->scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    drawObj->setImageData(*dst_image);
    updateImage(obj);
    delete  dst_image;
}
void LaserScene::m4modeChanged(QObject* obj, bool value)
{
    QQmlProperty::write(obj, "m4mode", value);
}
void LaserScene::imageFlipModelValueChanged(QObject* obj, int value)
{
    //0 : no flip; 1 : Vertical; 2: Horizontal; 3: Simultaneously
    QQmlProperty::write(obj, "filpModelValue", value);
    bool originalShow = QQmlProperty::read(obj, "originalShow").toBool();
    bool bUpDown = false;
    bool bRightLeft = false;
    switch (value)
    {
    case 0:
        bUpDown = false;
        bRightLeft = false;
        break;
    case 1:
        bUpDown = true;
        bRightLeft = false;
        break;
    case 2:
        bUpDown = false;
        bRightLeft = true;
        break;
    case 3:
        bUpDown = true;
        bRightLeft = true;
        break;
    default:
        break;
    }

    int threshold = QQmlProperty::read(obj, "threshold").toInt();
    QImage* dst_image = new QImage;
    DrawObject* drawObj = m_pDrawObjectModel->getDataByQmlObj(obj);
    QMetaObject::invokeMethod(drawObj->qmlObject(), "setAnimation", Q_ARG(QVariant, QVariant::fromValue(true)));
    QString imgType = drawObj->type();
    *dst_image = drawObj->bigImageData();
    if (originalShow) *dst_image = drawObj->originImageData();
    else if ("vector" == imgType)
    {
        *dst_image = drawObj->originImageData();
        *dst_image = CX2DLib::CXBlackWhitePicture(*dst_image, threshold);
    }

    *dst_image = CX2DLib::CXFlipImage(*dst_image, bUpDown, bRightLeft);
    bool reverse = QQmlProperty::read(obj, "reverse").toBool();
    if (reverse && !originalShow)  *dst_image = CX2DLib::CXReversalImgae(*dst_image);

    if ("vector" == imgType && !originalShow)
    {
        QPainterPath path;
        *dst_image = CX2DLib::CXImage2VectorImage(*dst_image, path);
        drawObj->setPath(path);
    }
    drawObj->setDstImageData(*dst_image);
    int width = QQmlProperty::read(obj, "width").toInt();
    int height = QQmlProperty::read(obj, "height").toInt();
    *dst_image = dst_image->scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    drawObj->setImageData(*dst_image);
    updateImage(obj);
    delete  dst_image;
}

void LaserScene::imageContrastChanged(QObject* obj, int value)
{
    value = QQmlProperty::read(obj, "threshold").toInt();
    grayImage(obj, value);
}
void LaserScene::imageBrightnessChanged(QObject* obj, int value)
{
    value = QQmlProperty::read(obj, "threshold").toInt();
    grayImage(obj, value);
}
void LaserScene::imageWhiteValueChanged(QObject* obj, int value)
{
    value = QQmlProperty::read(obj, "threshold").toInt();
    grayImage(obj, value);
}

void LaserScene::deleteObject(QObject *obj)
{
    int index =m_pDrawObjectModel->getIndex(obj);
    m_pDrawObjectModel->removeRows(index,1);

}
void LaserScene::deleteMulObject(QList<QObject*> objList)
{
    for (int i = 0; i < objList.size(); i++)
    {
        int index = m_pDrawObjectModel->getIndex(objList[i]);
        if (m_type == SCENETYPE::LASER)
        {
            m_LaserUndo->addDragSharp(this, objList[i], m_pDrawObjectModel->getData(index)->name(), true, true);
        }
        else
        {
            m_plotterUndo->addDragSharp(this, objList[i], m_pDrawObjectModel->getData(index)->name(), true, true);
        }

        m_pDrawObjectModel->removeRows(index, 1);
    }
}
void LaserScene::deleteUndoObject(QObject* obj)
{
    QMetaObject::invokeMethod(m_laserscene, "deleteUndoObject", Q_ARG(QVariant, QVariant::fromValue<QObject*>(obj)));
}
void LaserScene::genLaserGcode()
{
    if (m_pDrawObjectModel->objSize() <= 0)return;
    QSharedPointer<LaserGcodeWorker> worker(new LaserGcodeWorker());
    worker->setDrawObjModel(m_pDrawObjectModel);
    if (SCENETYPE::LASER == m_type)
    {
        worker->setGenType("laser");
        worker->setLaserSetting(m_pLaserSettings);
    }
    else
    {
        worker->setGenType("plotter");
        worker->setPlotterSetting(m_pPlotterSettings);
    }
    worker->setOrigin(m_origin);
    worker->setExporter(m_3in1exporter);
    connect(worker.data(), SIGNAL(genGcodeSuccess(QString)), this, SLOT(onGenGcodeSuccess(QString)));
    cxkernel::executeJob(worker);
}
void LaserScene::onGenGcodeSuccess(QString gcodeBuf)
{
    saveLocalGcodeFile();
}

void LaserScene::saveLocalGcodeFile()
{
    m_tempSave = true;
    cxkernel::saveFile(this, QString("%1.gcode").arg(QFileInfo(m_pDrawObjectModel->getData(0)->name()).baseName()));
}

void LaserScene::accept()
{
    cxkernel::openLastSaveFolder();
}

bool LaserScene::saveGcode(const QString& fileName)
{
    QString gcodeFile = getCanWriteFolder() + "/" + "tmpGcode.gcode";
    if (QFile::exists(gcodeFile))
    {
        if (QFile::exists(fileName))
        {
            QFile::remove(fileName);
        }

        bool ok = QFile::copy(gcodeFile, fileName);
        if (ok)
            return true;
        else
            return false;
    }
    else
    {
        return false;
    }
}


QString LaserScene::getTmpFilePeth()
{
    return m_strTmpFilePath;
}

void LaserScene::saveLocalGcodeFileSuccess()
{
    m_strMessageText = tr("Save Finished, Open Local Folder");
    requestQmlDialog(this, "messageDlg");
}

void LaserScene::saveLocalGcodeFileFail()
{
    m_strMessageText = tr("Save Gcode failed!");
    requestQmlDialog(this, "messageSingleBtnDlg");
}

QStringList LaserScene::getFontFamilys()
{
    QFontDatabase database;
    const QStringList fontFamilies = database.families();
    return fontFamilies;
}

QStringList LaserScene::getFontFamilysStyles(QString family)
{
    QFontDatabase database;
    const QStringList fontStyles = database.styles(family);
    return fontStyles;
}

QRect getModuleRect(QImage* previewImage)
{
    int imageW = previewImage->width();
    int imageH = previewImage->height();
    int dataLength = imageW * imageH * 4;
    std::vector<unsigned char> data(dataLength);
    memcpy(data.data(), previewImage->bits(), dataLength);
    int lineDataLength = imageW * 4;
    int minX = imageW, maxX = -1, minY = -1, maxY = -1;
    for (int i = 3; i < dataLength; i += 4)
    {
        if (data[i] != 0)
        {
            int x = (i % lineDataLength) / 4;
            int y = i / lineDataLength;
            if (minY == -1) minY = y;
            maxY = y;
            if (minX > x)
            {
                minX = x;
            }
            if (maxX < x)
            {
                maxX = x;
            }
        }
    }
    QRect moduleRect = QRect(minX, minY, maxX - minX, maxY - minY);
    return moduleRect;
}

QImage createImageWithOverlay(const QImage& baseImage, const QImage& overlayImage)
{
    QImage imageWithOverlay = QImage(baseImage.size(), QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&imageWithOverlay);

    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(imageWithOverlay.rect(), Qt::transparent);

    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawImage(0, 0, baseImage);

    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawImage(0, 0, overlayImage);

    painter.end();

    return imageWithOverlay;
}

