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

#if !defined (UPDATER_UPDATE_MANAGER_H_INCLUDED)
#define UPDATER_UPDATE_MANAGER_H_INCLUDED
#include "qtusercore/qtusercoreexport.h"
#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QTimer>

#include "qtusercore/updater/RemoteType.h"
#include "qtusercore/updater/ReleaseInfo.h"

class QTUSER_CORE_API UpdateManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(UpdateManager)
    Q_PROPERTY(bool autocheck READ autoCheck WRITE setAutoCheck NOTIFY autoCheckChanged)

	Q_PROPERTY(QString version READ version CONSTANT)
	Q_PROPERTY(QUrl url READ url CONSTANT)
	Q_PROPERTY(QString releaseNote READ releaseNote CONSTANT)
public:
    UpdateManager(RemoteType remoteType, const QString &owner, const QString &repo, const QString &currentVersion, const int app_type, const QString& build_os, bool autoCheck = false, QObject *parent = Q_NULLPTR);
    ~UpdateManager();
    Q_INVOKABLE void setAutoCheck(bool autoCheck);
    Q_INVOKABLE void skipVersion(QString version);
    Q_INVOKABLE QString getCurrentVersion();    //add by lisugui，use to aboutus menu
    Q_INVOKABLE void initAutoCheckVersionFlag();
	QString version();
	QUrl url();
	QString releaseNote();
Q_SIGNALS:
    void updateFound(QString version,QUrl url,QString releaseNote);
	void updateNotFound();
    void autoCheckChanged();
public Q_SLOTS:
    void checkForUpdates(bool force);
    Q_INVOKABLE void checkForUpdates();
    Q_INVOKABLE void forceCheckForUpdates();
    Q_INVOKABLE QString getProjectName();
    bool autoCheck();
private Q_SLOTS:
    void onUpdateNotFound(const ReleaseInfo &currentRelease);
    void onUpdateFound(const ReleaseInfo &currentRelease, const QList<ReleaseInfo> &newReleases);
    void onUpdateError(const QString &errorString);

public slots:
    void displayCurrentVersionInfo();
private:
    QString getUpdateLogFileName();

private:
    struct Impl;
    QScopedPointer<Impl> m_impl;
    QString m_currentVersion;

	QString m_releaseVersion;
	QUrl m_releaseUrl;
	QString m_releaseNote;
    bool m_initAutoCheckFlag;
};

#endif // UPDATER_UPDATE_MANAGER_H_INCLUDED

