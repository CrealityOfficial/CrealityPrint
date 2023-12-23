#pragma once

#ifndef CXCLOUD_TOOL_FUCTION_H_
#define CXCLOUD_TOOL_FUCTION_H_

#include <stdint.h>

#include <QtCore/QChar>
#include <QtCore/QString>

#include "cxcloud/interface.hpp"

namespace cxcloud {

CXCLOUD_API inline constexpr double operator""_kb(unsigned long long size) noexcept {
  return size * 1024.0;
}

CXCLOUD_API inline constexpr double operator""_kb(long double size) noexcept {
  return size * 1024.0;
}

CXCLOUD_API inline constexpr double operator""_mb(unsigned long long size) noexcept {
  return size * 1024.0 * 1024.0;
}

CXCLOUD_API inline constexpr double operator""_mb(long double size) noexcept {
  return size * 1024.0 * 1024.0;
}

CXCLOUD_API inline constexpr double operator""_gb(unsigned long long size) noexcept {
  return size * 1024.0 * 1024.0 * 1024.0;
}

CXCLOUD_API inline constexpr double operator""_gb(long double size) noexcept {
  return size * 1024.0 * 1024.0 * 1024.0;
}

/// @brief trans file_name to current platform local disk legal name.
///        such as "my/file???" -> "my_file___" in windows.
CXCLOUD_API void ReplaceIllegalCharacters(QString& file_name, QChar const& legal_character = '_');

/// @brief trans base_name to a non-exist name in dir dir_path.
///        such as "base_name" -> "base_name_2", when "base_name" and "base_name_1" existed.
/// @param base_name param and result
CXCLOUD_API void ConvertNonRepeatingPath(QString const& dir_path, QString& base_name);

}  // namespace cxcloud

#endif  // CXCLOUD_TOOL_FUCTION_H_
