/*
   Copyright (C) 2019 Peter S. Zhigalov <peter.zhigalov@gmail.com>

   This file is part of the `QtUtils' library.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "version.h"

#include <QDebug>
#include <QRegExp>
#include <QStringList>


static int prepareDetail(const Version &version, int detail)
{
    switch(detail)
    {
    case Version::DetailMajor:
    case Version::DetailMinor:
    case Version::DetailPatch:
    case Version::DetailBuild:
        return detail;
    default:
        break;
    }
    if(version.hasBuild())
        detail = Version::DetailBuild;
    else if(version.hasPatch())
        detail = Version::DetailPatch;
    else if(version.hasMinor())
        detail = Version::DetailMinor;
    else
        detail = Version::DetailMajor;
    return detail;
}

static int versionComponentFromString(const QString &component)
{
    QString string = component;
    string.remove(QRegExp(QString::fromLatin1("[^\\d]")));
    return string.toInt();
}

Version::Version(int major, int minor, int patch, int build, const QString& extra, const QString& os)
    : m_major(major)
    , m_minor(minor)
    , m_patch(patch)
    , m_build(build)
    , m_extra(extra)
    , m_os(os)
{}

Version::Version(const QString &version, const QString &delimeter)
    : m_major(-1)
    , m_minor(-1)
    , m_patch(-1)
    , m_build(-1)
    , m_extra("Alpha")
{
    const QStringList list = version.split(delimeter);
    switch(list.size())
    {
    default:
    case 4:
        setBuild(versionComponentFromString(list[3]));
        Q_FALLTHROUGH();
    case 3:
        setPatch(versionComponentFromString(list[2]));
        Q_FALLTHROUGH();
    case 2:
        setMinor(versionComponentFromString(list[1]));
        Q_FALLTHROUGH();
    case 1:
        setMajor(versionComponentFromString(list[0]));
        Q_FALLTHROUGH();
    case 0:
        break;
    }
    const QStringList list2 = version.split(" ");
    if (list2.size() >= 2)
    {
        m_extra = list2[1];
    }
    if (list2.size() >= 3)
    {
        m_os = list2[2];
    }
    
}

Version::Version(const Version &version)
    : m_major(version.major())
    , m_minor(version.minor())
    , m_patch(version.patch())
    , m_build(version.build())
    , m_extra("Alpha")
{}

Version::~Version()
{}

Version &Version::operator = (const Version &other)
{
    setMajor(other.major());
    setMinor(other.minor());
    setPatch(other.patch());
    setBuild(other.build());
    m_extra = other.extra();
    return *this;
}

bool Version::operator == (const Version &other) const
{
    return major() == other.major() && minor() == other.minor() && patch() == other.patch() && build() == other.build();
}

bool Version::operator != (const Version &other) const
{
    return !((*this) == other);
}

bool Version::operator < (const Version &other) const
{
    if(major() < other.major())
        return true;
    if(major() > other.major())
        return false;

    if(minor() < other.minor())
        return true;
    if(minor() > other.minor())
        return false;

    if(patch() < other.patch())
        return true;
    if(patch() > other.patch())
        return false;

    if(build() < other.build())
        return true;
    if(build() > other.build())
        return false;

    return false;
}

bool Version::operator <= (const Version &other) const
{
    return ((*this) == other) || ((*this) < other);
}

bool Version::operator > (const Version &other) const
{
    return ((*this) != other) && !((*this) < other);
}

bool Version::operator >= (const Version &other) const
{
    return ((*this) == other) || ((*this) > other);
}

bool Version::isValid() const
{
    return hasMajor();
}

bool Version::hasMajor() const
{
    return major() >= 0;
}

bool Version::hasMinor() const
{
    return minor() >= 0;
}

bool Version::hasPatch() const
{
    return patch() >= 0;
}

bool Version::hasBuild() const
{
    return build() >= 0;
}

int Version::major() const
{
    return m_major;
}

int Version::minor() const
{
    return m_minor;
}

int Version::patch() const
{
    return m_patch;
}

int Version::build() const
{
    return m_build;
}

QString Version::extra() const
{
    return m_extra;
}
QString Version::os() const
{
    return m_os;
}
void Version::setOs(QString os)
{
    m_os = os;
}
void Version::setMajor(int major)
{
    m_major = major;
}

void Version::setMinor(int minor)
{
    m_minor = minor;
}

void Version::setPatch(int patch)
{
    m_patch = patch;
}

void Version::setBuild(int build)
{
    m_build = build;
}

Version Version::shrinked(int detail) const
{
    detail = prepareDetail(*this, detail);
    Version result = *this;

    if(!result.hasMajor())
        result.setMajor(0);
    if(!result.hasMinor())
        result.setMinor(0);
    if(!result.hasPatch())
        result.setPatch(0);
    if(!result.hasBuild())
        result.setBuild(0);

    switch(detail)
    {
    case DetailMajor:
        result.setMajor(-1);
        Q_FALLTHROUGH();
    case DetailMinor:
        result.setMinor(-1);
        Q_FALLTHROUGH();
    case DetailPatch:
        result.setPatch(-1);
        Q_FALLTHROUGH();
    case DetailBuild:
        result.setBuild(-1);
        Q_FALLTHROUGH();
    default:
        break;
    }

    return result;
}

QString Version::versionFullName() const
{
    QString version = QString("V%1.%2.%3.%4 %5 %6").arg(m_major)
        .arg(m_minor).arg(m_patch).arg(m_build).arg(m_extra).arg(m_os);
    return version;
}

QString Version::versionName() const
{
    QString version = QString("V%1.%2.%3.%4").arg(m_major)
        .arg(m_minor).arg(m_patch).arg(m_build);
    return version;
}

QString Version::string(int detail, const QString &delimeter) const
{
    detail = prepareDetail(*this, detail);
    QString result;
    switch(detail)
    {
    case DetailBuild:
        result.prepend(delimeter + QString::number(build()));
        Q_FALLTHROUGH();
    case DetailPatch:
        result.prepend(delimeter + QString::number(patch()));
        Q_FALLTHROUGH();
    case DetailMinor:
        result.prepend(delimeter + QString::number(minor()));
        Q_FALLTHROUGH();
    case DetailMajor:
        result.prepend(QString::number(major()));
        Q_FALLTHROUGH();
    default:
        break;
    }
    return result;
}

QDebug operator << (QDebug debug, const Version &version)
{
    debug << (QString::fromLatin1("Version(%1)").arg(version.string())).toLatin1().data();
    return debug;
}

