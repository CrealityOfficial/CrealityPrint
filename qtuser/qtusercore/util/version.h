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

#if !defined (UPDATER_VERSION_H_INCLUDED)
#define UPDATER_VERSION_H_INCLUDED
#include "qtusercore/qtusercoreexport.h"

#include <QString>

class QDebug;

class QTUSER_CORE_API Version
{
public:
    enum Detail
    {
        DetailAuto = 0,
        DetailMajor = 1,
        DetailMinor = 2,
        DetailPatch = 3,
        DetailBuild = 4
    };

    explicit Version(int major = -1, int minor = -1, int patch = -1, int build = -1, const QString& extra = QString("alpha"), const QString& os = QString("win64"));
    explicit Version(const QString &version, const QString &delimeter = QString::fromLatin1("."));
    Version(const Version &version);
    ~Version();

    Version &operator = (const Version &other);
    bool operator == (const Version &other) const;
    bool operator != (const Version &other) const;
    bool operator < (const Version &other) const;
    bool operator <= (const Version &other) const;
    bool operator > (const Version &other) const;
    bool operator >= (const Version &other) const;

    bool isValid() const;

    bool hasMajor() const;
    bool hasMinor() const;
    bool hasPatch() const;
    bool hasBuild() const;

    int major() const;
    int minor() const;
    int patch() const;
    int build() const;
    QString extra() const;
    QString os() const;
    void setMajor(int major);
    void setMinor(int minor);
    void setPatch(int patch);
    void setBuild(int build);
    void setOs(QString os);
    Version shrinked(int detail) const;
    QString versionFullName() const;
    QString versionName() const;
    QString string(int detail = DetailAuto, const QString &delimeter = QString::fromLatin1(".")) const;

private:
    int m_major;
    int m_minor;
    int m_patch;
    int m_build;
    QString m_extra;
    QString m_os;
};

QTUSER_CORE_API QDebug operator << (QDebug debug, const Version &version);

#endif // UPDATER_VERSION_H_INCLUDED

