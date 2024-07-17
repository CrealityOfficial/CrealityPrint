#include "cxcloud/tool/version.h"

#include <QtCore/QStringList>

namespace cxcloud {

  Version::Version(int64_t major, int64_t minor, int64_t patch, int64_t build)
      : major_{ std::max<int64_t>(-1, major) }
      , minor_{ std::max<int64_t>(-1, minor) }
      , patch_{ std::max<int64_t>(-1, patch) }
      , build_{ std::max<int64_t>(-1, build) } {}

  Version::Version(const QString& string) {
    *this = string;
  }

  auto Version::operator=(const QString& string) -> Version& {
    auto elements = string.split(QStringLiteral("."));
    if (elements.empty()) {
      return *this;
    }

    auto& major = elements[0];
    major = major.toLower();
    if (major.startsWith(QStringLiteral("v"))) {
      major.replace(QStringLiteral("v"), QStringLiteral(""));
    }

    const auto elements_size = elements.size();
    major_ = elements_size > 0 ? elements[0].toLongLong() : -1;
    minor_ = elements_size > 1 ? elements[1].toLongLong() : -1;
    patch_ = elements_size > 2 ? elements[2].toLongLong() : -1;
    build_ = elements_size > 3 ? elements[3].toLongLong() : -1;
    return *this;
  }

  auto Version::FromString(const QString& string) -> Version {
    return { string };
  }

  auto Version::toString() const -> QString {
    if (major_ < 0) {
      return QStringLiteral("");
    }

    if (minor_ < 0) {
      return QStringLiteral("%1").arg(QString::number(major_));
    }

    if (patch_ < 0) {
      return QStringLiteral("%1.%2").arg(QString::number(major_), QString::number(minor_));
    }

    if (build_ < 0) {
      return QStringLiteral("%1.%2.%3").arg(QString::number(major_),
                                            QString::number(minor_),
                                            QString::number(patch_));
    }

    return QStringLiteral("%1.%2.%3.%4").arg(QString::number(major_),
                                             QString::number(minor_),
                                             QString::number(patch_),
                                             QString::number(build_));
  }

  auto Version::getMajor() const -> uint32_t {
    return static_cast<uint32_t>(std::max<int64_t>(0, major_));
  }

  auto Version::setMajor(int64_t major) -> void {
    major_ = std::max<int64_t>(-1, major);
  }

  auto Version::getMinor() const -> uint32_t {
    return static_cast<uint32_t>(std::max<int64_t>(0, minor_));
  }

  auto Version::setMinor(int64_t minor) -> void {
    minor_ = std::max<int64_t>(-1, minor);
  }

  auto Version::getPatch() const -> uint32_t {
    return static_cast<uint32_t>(std::max<int64_t>(0, patch_));
  }

  auto Version::setPatch(int64_t patch) -> void {
    patch_ = std::max<int64_t>(-1, patch_);
  }

  auto Version::getBuild() const -> uint32_t {
    return static_cast<uint32_t>(std::max<int64_t>(0, build_));
  }

  auto Version::setBuild(int64_t build) -> void {
    build_ = std::max<int64_t>(-1, build);
  }

  auto Version::operator==(const Version& other) const -> bool {
    auto& self = *this;
    return self.getMajor() == other.getMajor() &&
           self.getMinor() == other.getMinor() &&
           self.getPatch() == other.getPatch() &&
           self.getBuild() == other.getBuild();
  }

  auto Version::operator!=(const Version& other) const -> bool {
    auto& self = *this;
    return !(self == other);
  }

  auto Version::operator<(const Version& other) const -> bool {
    auto& self = *this;

    const auto self_major = self.getMajor();
    const auto self_minor = self.getMinor();
    const auto self_patch = self.getPatch();
    const auto self_build = self.getBuild();

    const auto other_major = other.getMajor();
    const auto other_minor = other.getMinor();
    const auto other_patch = other.getPatch();
    const auto other_build = other.getBuild();

    return self_major < other_major ? true : self_major > other_major ? false :
           self_minor < other_minor ? true : self_minor > other_minor ? false :
           self_patch < other_patch ? true : self_patch > other_patch ? false :
           self_build < other_build ;
  }

  auto Version::operator<=(const Version& other) const -> bool {
    auto& self = *this;
    return self == other || *this < other;
  }

  auto Version::operator>(const Version& other) const -> bool {
    auto& self = *this;
    return !(self <= other);
  }

  auto Version::operator>=(const Version& other) const -> bool {
    auto& self = *this;
    return self == other || *this > other;
  }

  auto Version::isEqualTo(const Version& other) const -> bool {
    auto& self = *this;
    return self == other;
  }

  auto Version::isLessThan(const Version& other) const -> bool {
    auto& self = *this;
    return self < other;
  }

  auto Version::isGreaterThan(const Version& other) const -> bool {
    auto& self = *this;
    return self > other;
  }

  auto VersionToString(const Version& version) -> QString {
    return version.toString();
  }

  auto StringToVersion(const QString& string) -> Version {
    return Version{ string };
  }

  auto operator""_v(const char* string, size_t size) -> Version {
    return StringToVersion(QString{ string }.replace('\'', '.'));
  }

}  // namespace cxcloud
