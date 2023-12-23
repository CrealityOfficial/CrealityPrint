#pragma once

#ifndef CXCLOUD_TOOL_VERSION_H
#define CXCLOUD_TOOL_VERSION_H

#include <string>

#include <QtCore/QString>

#include "cxcloud/interface.hpp"

namespace cxcloud {

class CXCLOUD_API Version final {
public:
  Version(uint32_t major = 0, uint32_t minor = 0, uint32_t patch = 0, uint32_t build = 0);
  Version& operator=(const Version&) = default;
  Version& operator=(Version&&) = default;
  Version(const Version&) = default;
  Version(Version&&) = default;

  ~Version() = default;

public:
  Version& operator=(const QString&);
  Version(const QString&);
  QString toString() const;
  static Version FromString(const QString&);

public:
  uint32_t getMajor() const;
  void setMajor(uint32_t major);

  uint32_t getMinor() const;
  void setMinor(uint32_t minor);

  uint32_t getPatch() const;
  void setPatch(uint32_t patch);

  uint32_t getBuild() const;
  void setBuild(uint32_t build);

public:
  bool operator==(const Version& that) const;
  bool operator!=(const Version& that) const;
  bool operator<(const Version& that) const;
  bool operator<=(const Version& that) const;
  bool operator>(const Version& that) const;
  bool operator>=(const Version& that) const;

  bool isEqualTo(const Version& that) const;
  bool isSmallerThan(const Version& that) const;
  bool isBiggerThan(const Version& that) const;

private:
  uint32_t major_;
  uint32_t minor_;
  uint32_t patch_;
  uint32_t build_;
};

CXCLOUD_API QString VersionToString(const Version&);
CXCLOUD_API Version StringToVersion(const QString&);

CXCLOUD_API Version operator""_v(const char* string, size_t size);

}  // namespace cxcloud

#endif  // CXCLOUD_TOOL_VERSION_H
