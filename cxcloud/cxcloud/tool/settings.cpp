#include "cxcloud/tool/settings.h"

#include "cxcloud/define.h"
#include "cxcloud/tool/function.h"

namespace cxcloud {

  namespace {

    static const auto SETTINGS_GROUP = QStringLiteral("cloud_service");
    static const auto SERVER_TYPE_KEY = QStringLiteral("server_type");

  }  // namespace

  CloudSettings::CloudSettings() : qtuser_core::VersionSettings{} {
    beginGroup(SETTINGS_GROUP);
  }

  auto CloudSettings::getServerType() const -> ServerType {
    static const auto DEFAULT_INDEX = static_cast<int>(DEFAULT_SERVER_TYPE);
    return static_cast<ServerType>(value(SERVER_TYPE_KEY, DEFAULT_INDEX).toInt());
  }

  auto CloudSettings::setServerType(ServerType type) -> void {
    setValue(SERVER_TYPE_KEY, static_cast<int>(type));
  }

  VersionServerSettings::VersionServerSettings() : qtuser_core::VersionSettings{} {
    static const auto KEY = QStringLiteral("%1/%2").arg(SETTINGS_GROUP, SERVER_TYPE_KEY);
    auto index = value(KEY, static_cast<int>(DEFAULT_SERVER_TYPE)).toInt();
    auto type = static_cast<ServerType>(index);
    auto real_type = ServerTypeToRealServerType(type);
    auto real_index = static_cast<int>(real_type);
    beginGroup(QStringLiteral("server_%1").arg(QString::number(real_index)));
  }

}  // namespace cxcloud
