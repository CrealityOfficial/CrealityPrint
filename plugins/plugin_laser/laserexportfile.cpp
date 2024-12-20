#include "laserexportfile.h"

#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include <QDebug>
#include <interface/spaceinterface.h>

//#include "interface/modelinterface.h"
#include "interface/uiinterface.h"
#include "kernel/kernelui.h"
#include <QFileInfo>

using namespace creative_kernel;

LaserExportFile::LaserExportFile(QObject *parent)
    :QObject(parent)
{
    m_footStatus = getKernelUI()->getUI("footer");
}

LaserExportFile::~LaserExportFile()
{
}
 QObject* LaserExportFile::getLaserScene()
{
    return m_laserScene;
}
  void LaserExportFile::localFileSaveSuccess(QObject* object)
  {
      requestQmlDialog(this, "laserSaveSuccessDlg");
  }
  void LaserExportFile::sliceUnPreview()
  {
      QMetaObject::invokeMethod(m_footStatus, "showSlciePreviewStatus",  Q_ARG(QVariant, QVariant::fromValue(false)));
  }

  int LaserExportFile::onExportSuccess(QObject* object)
  {
      return 0;
  }

  bool LaserExportFile::copyFile(QString srcPath, QString dstPath, bool coverFileIfExist)
   {
       srcPath.replace("\\", "/");
       dstPath.replace("\\", "/");
       if (srcPath == dstPath) {
           return true;
       }

       if (!QFile::exists(srcPath)) {  //源文件不存在
           return false;
       }

       if (QFile::exists(dstPath)) {
           if (coverFileIfExist) {
               QFile::remove(dstPath);
           }
       }

       if (!QFile::copy(srcPath, dstPath)) {
           return false;
       }
       return true;
   }
