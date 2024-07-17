#include "settings.h"

#include <buildinfo.h>

namespace qtuser_core {

  VersionSettings::VersionSettings() : Settings{} {
    beginGroup(QStringLiteral("%1.%2").arg(QString::number(PROJECT_VERSION_MAJOR))
                                      .arg(QString::number(PROJECT_VERSION_MINOR)));
  }

}  // namespace qtuser_core
