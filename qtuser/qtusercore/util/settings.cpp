#include "settings.h"

#include <buildinfo.h>

namespace qtuser_core {

  VersionSettings::VersionSettings() : Settings{} {
    beginGroup(QStringLiteral("%1.%2").arg(QString::number(PROJECT_VERSION_MAJOR))
                                      .arg(QString::number(0)));
  }

}  // namespace qtuser_core
