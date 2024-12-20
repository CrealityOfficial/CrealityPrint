#pragma once

#ifndef QTUSERCORE_UTIL_SETTINGS_H_
#define QTUSERCORE_UTIL_SETTINGS_H_

#include <QtCore/QSettings>

#include "qtusercore/qtusercoreexport.h"

//  The classes in this file will have the same operation as QSettings
//  but with different begin group.
namespace qtuser_core {

  /// @note QSETTINGS_GROUP: \HKEY_CURRENT_USER\SOFTWARE at Windows
  /// @note ORGANIZATION: see marco ORGANIZATION in <buildinfo.h>
  /// @note APPLICATION: see parameter in lastest QCoreApplication::setApplicationName call
  /// @brief default group at: QSETTINGS_GROUP/ORGANIZATION/APPLICATION/
  using Settings = QSettings;

  /// @note APPLICATION_GTOUP: Settings's default group
  /// @note MAJOR_VERSION: see marco PROJECT_VERSION_MAJOR in <buildinfo.h>
  /// @note MINOR_VERSION: see marco PROJECT_VERSION_MINOR in <buildinfo.h>
  /// @brief default group at: APPLICATION_GTOUP/MAJOR_VERSION.MINOR_VERSION
  class QTUSER_CORE_API VersionSettings : public Settings {
    Q_OBJECT;

   public:
    explicit VersionSettings();
    ~VersionSettings() = default;
  };

}  // namespace qtuser_core

//  If these classes still don't meet your needs,
//  you may need these classes in <cxcloud/tool/settings.h> in the "cxcloud" submodule:
namespace cxcloud {
  class CloudSettings;
  class VersionServerSettings;
}  // namespace cxcloud

#endif  // QTUSERCORE_UTIL_SETTINGS_H_
