#pragma once

#ifndef CXCLOUD_TOOL_FUCTION_H_
#define CXCLOUD_TOOL_FUCTION_H_

#include <QtCore/QChar>
#include <QtCore/QString>

#include "cxcloud/define.h"
#include "cxcloud/interface.hpp"

namespace cxcloud {

  CXCLOUD_API constexpr inline auto ServerTypeToRealServerType(ServerType type) -> RealServerType {
    return type == ServerType::MAINLAND_CHINA ? RealServerType::MAINLAND_CHINA
                                              : RealServerType::INTERNATIONAL;
  };

  CXCLOUD_API constexpr inline auto RealServerTypeToServerType(RealServerType type) -> ServerType {
    return type == RealServerType::MAINLAND_CHINA ? ServerType::MAINLAND_CHINA : ServerType::OTHERS;
  };

  CXCLOUD_API constexpr inline auto operator""_kb(unsigned long long size) noexcept -> double {
    return size * 1024.0;
  }

  CXCLOUD_API constexpr inline auto operator""_kb(long double size) noexcept -> double {
    return size * 1024.0;
  }

  CXCLOUD_API constexpr inline auto operator""_mb(unsigned long long size) noexcept -> double {
    return size * 1024.0 * 1024.0;
  }

  CXCLOUD_API constexpr inline auto operator""_mb(long double size) noexcept -> double {
    return size * 1024.0 * 1024.0;
  }

  CXCLOUD_API constexpr inline auto operator""_gb(unsigned long long size) noexcept -> double {
    return size * 1024.0 * 1024.0 * 1024.0;
  }

  CXCLOUD_API constexpr inline auto operator""_gb(long double size) noexcept -> double {
    return size * 1024.0 * 1024.0 * 1024.0;
  }

  /// @brief trans file_name to current platform local disk legal name.
  ///        such as "my/file???" -> "my_file___" in windows.
  CXCLOUD_API auto ReplaceIllegalCharacters(QString& file_name,
                                            QChar const& legal_character = '_') -> void;

  /// @brief trans base_name to a non-exist name in dir dir_path.
  ///        such as "base_name" -> "base_name_2", when "base_name" and "base_name_1" existed.
  /// @param base_name param and result
  CXCLOUD_API auto ConvertNonRepeatingPath(QString const& dir_path, QString& base_name) -> void;

  CXCLOUD_API auto ModelRestrictionToString(ModelRestriction moderation) -> QString;
  CXCLOUD_API auto StringToModelRestriction(const QString& moderation) -> ModelRestriction;

  CXCLOUD_API auto CheckNetworkAvailable() -> bool;

}  // namespace cxcloud

#endif  // CXCLOUD_TOOL_FUCTION_H_
