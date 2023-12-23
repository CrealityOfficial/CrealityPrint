#include "cxcloud/tool/version.h"

#include <QtCore/QStringList>

namespace cxcloud {

Version::Version(uint32_t major, uint32_t minor, uint32_t patch, uint32_t build)
    : major_(major), minor_(minor), patch_(patch), build_(build) {}

Version& Version::operator=(const QString& string) {
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
  major_ = elements_size > 0 ? elements[0].toUInt() : 0;
  minor_ = elements_size > 1 ? elements[1].toUInt() : 0;
  patch_ = elements_size > 2 ? elements[2].toUInt() : 0;
  build_ = elements_size > 3 ? elements[3].toUInt() : 0;
  return *this;
}

Version::Version(const QString& string) {
  *this = string;
}

QString Version::toString() const {
  return QStringLiteral("%1.%2.%3.%4").arg(major_).arg(minor_).arg(patch_).arg(build_);
}

Version Version::FromString(const QString& string) {
  return Version{ string };
}

uint32_t Version::getMajor() const { return major_; }

void Version::setMajor(uint32_t major) { major_ = major; }

uint32_t Version::getMinor() const { return minor_; }

void Version::setMinor(uint32_t minor) { minor_ = minor; }

uint32_t Version::getPatch() const { return patch_; }

void Version::setPatch(uint32_t patch) { patch_ = patch; }

uint32_t Version::getBuild() const { return build_; }

void Version::setBuild(uint32_t build) { build_ = build; }

bool Version::operator==(const Version& that) const {
  return this->major_ == that.major_ &&
         this->minor_ == that.minor_ &&
         this->patch_ == that.patch_ &&
         this->build_ == that.build_;
}

bool Version::operator!=(const Version& that) const {
  return !(*this == that);
}

bool Version::operator<(const Version& that) const {
  return this->major_ < that.major_ ? true : this->major_ > that.major_ ? false :
         this->minor_ < that.minor_ ? true : this->minor_ > that.minor_ ? false :
         this->patch_ < that.patch_ ? true : this->patch_ > that.patch_ ? false :
         this->build_ < that.build_ ;
}

bool Version::operator<=(const Version& that) const {
  return *this == that || *this < that;
}

bool Version::operator>(const Version& that) const {
  return !(*this <= that);
}

bool Version::operator>=(const Version& that) const {
  return *this == that || *this > that;
}

bool Version::isEqualTo(const Version& that) const {
  return *this == that;
}

bool Version::isSmallerThan(const Version& that) const {
  return *this < that;
}

bool Version::isBiggerThan(const Version& that) const {
  return *this > that;
}

QString VersionToString(const Version& version) {
  return version.toString();
}

Version StringToVersion(const QString& string) {
  return Version{ string };
}

Version operator""_v(const char* string, size_t size) {
  return StringToVersion(QString{ string }.replace('\'', '.'));
}

}  // namespace cxcloud
