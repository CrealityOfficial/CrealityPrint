#pragma once

#include <qtusercore/util/settings.h>

#include "cxcloud/define.h"
#include "cxcloud/interface.hpp"

namespace cxcloud {

  /// @note VERSION_GTOUP: qtuser_core::VersionSettings's default group
  /// @brief default group at: ${VERSION_GTOUP}/cloud_service
  class CXCLOUD_API CloudSettings : public qtuser_core::VersionSettings {
    Q_OBJECT;

   public:
    explicit CloudSettings();
    ~CloudSettings() = default;

   public:
    auto getServerType() const -> ServerType;
    auto setServerType(ServerType type) -> void;
  };

  /// @note VERSION_GTOUP: qtuser_core::VersionSettings's default group
  /// @note SERVER_TYPE: see cxcloud::RealServerType in cxcloud/define.hpp
  /// @brief default group at: ${VERSION_SERVER_GTOUP}/server_${SERVER_TYPE}
  class CXCLOUD_API VersionServerSettings : public qtuser_core::VersionSettings {
    Q_OBJECT;

   public:
    explicit VersionServerSettings();
    ~VersionServerSettings() = default;
  };


}  // namespace cxcloud
