#pragma once

#ifndef CXCLOUD_TOOL_VERSION_H_
#define CXCLOUD_TOOL_VERSION_H_

#include <QtCore/QString>

#include "cxcloud/interface.hpp"

namespace cxcloud {

  class CXCLOUD_API Version final {
   public:
    Version(int64_t major = 0, int64_t minor = 0, int64_t patch = 0, int64_t build = -1);
    Version(const Version&) = default;
    Version(Version&&) = default;
    auto operator=(const Version&) -> Version& = default;
    auto operator=(Version&&) -> Version& = default;

    ~Version() = default;

   public:
    Version(const QString&);
    auto operator=(const QString&) -> Version&;
    static auto FromString(const QString&) -> Version;
    auto toString() const -> QString;

   public:
    auto getMajor() const -> uint32_t;
    auto setMajor(int64_t major) -> void;

    auto getMinor() const -> uint32_t;
    auto setMinor(int64_t minor) -> void;

    auto getPatch() const -> uint32_t;
    auto setPatch(int64_t patch) -> void;

    auto getBuild() const -> uint32_t;
    auto setBuild(int64_t build) -> void;

   public:
    auto operator==(const Version& other) const -> bool;
    auto operator!=(const Version& other) const -> bool;
    auto operator<(const Version& other) const -> bool;
    auto operator<=(const Version& other) const -> bool;
    auto operator>(const Version& other) const -> bool;
    auto operator>=(const Version& other) const -> bool;

    auto isEqualTo(const Version& other) const -> bool;
    auto isLessThan(const Version& other) const -> bool;
    auto isGreaterThan(const Version& other) const -> bool;

   private:
    int64_t major_{ -1 };
    int64_t minor_{ -1 };
    int64_t patch_{ -1 };
    int64_t build_{ -1 };
  };

  CXCLOUD_API auto VersionToString(const Version&) -> QString;
  CXCLOUD_API auto StringToVersion(const QString&) -> Version;

  CXCLOUD_API auto operator""_v(const char* string, size_t size) -> Version;

}  // namespace cxcloud

#endif  // CXCLOUD_TOOL_VERSION_H_
