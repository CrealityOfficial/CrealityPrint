#include "cxkernel/kernel/const.h"

#include <fstream>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>

#include <buildinfo.h>

#include <qtusercore/module/systemutil.h>
#include <qtusercore/string/resourcesfinder.h>

namespace cxkernel {

  CXKernelConst::CXKernelConst(QObject* parent)
      : QObject{ parent } {
    QCoreApplication::setOrganizationName(QStringLiteral(ORGANIZATION));
    QCoreApplication::setOrganizationDomain(QStringLiteral("CX"));
    QCoreApplication::setApplicationName(QStringLiteral(PROJECT_NAME));
  }

  QString CXKernelConst::version() const {
    return QStringLiteral("V%1.%2.%3.%4").arg(QString::number(PROJECT_VERSION_MAJOR))
                                         .arg(QString::number(PROJECT_VERSION_MINOR))
                                         .arg(QString::number(PROJECT_VERSION_PATCH))
                                         .arg(QString::number(PROJECT_BUILD_ID));
  }

  QString CXKernelConst::versionExtra() const {
    return QStringLiteral(PROJECT_VERSION_EXTRA);
  }

  bool CXKernelConst::isReleaseVersion() const {
      return QString(PROJECT_VERSION_EXTRA) == QString("Release");
  }


  QString CXKernelConst::os() const {
    return QStringLiteral(BUILD_OS);
  }

  QString CXKernelConst::bundleName() const {
    return QStringLiteral(BUNDLE_NAME);
  }

  QString CXKernelConst::writableLocation(const QString& subDir, const QString& subSubDir) {
    QString path = QStringLiteral("%1/%2/").arg(getCanWriteFolder()).arg(subDir);
    if (!subSubDir.isEmpty()) {
      path += subSubDir;
    }

    QDir tempDir;
    if (!tempDir.exists(path)) {
      tempDir.mkpath(path);
    }

    return path;
  }

  bool CXKernelConst::useCXCloud()
  {
#if USE_CXCLOUD
      return true;
#else
      return false;
#endif
  }

}  // namespace cxkernel
