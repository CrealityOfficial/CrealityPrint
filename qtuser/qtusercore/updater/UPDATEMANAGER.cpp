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

#include "UpdateManager.h"
#include <QDateTime>
#include <QDebug>
#include <QPointer>
#include <QTimer>

#include <QSettings>
#include <QtCore/QCoreApplication>

#include "SettingsKeys.h"
#include "UpdateChecker.h"
#include <QFile>
#include "mdeditor.h"

static const qint64 DAYS_TO_NEXT_AUTO_CHECK = 1;

struct UpdateManager::Impl
{
    UpdateChecker *checker;
    QSettings updaterSettings;
    QTimer updateTimer;
    bool silent;
    bool inProgress;
    QString ownerstr;

    Impl(UpdateManager *manager, RemoteType remoteType, const QString &owner, const QString &repo, const QString &currentVersion, const int app_type, const QString& build_os, bool autoCheck)
        : checker(new UpdateChecker(remoteType, owner, repo, currentVersion, app_type, build_os, manager))
        , updaterSettings()
        , silent(false)
        , inProgress(false)
        , ownerstr(owner)
    {
        updaterSettings.setValue(KEY_AUTO_CHECK_FOR_UPDATES, updaterSettings.value(KEY_AUTO_CHECK_FOR_UPDATES, autoCheck));
        updaterSettings.setValue(KEY_SKIPPED_VERSION, updaterSettings.value(KEY_SKIPPED_VERSION, QString()));
        updaterSettings.setValue(KEY_LAST_CHECK_TIMESTAMP, updaterSettings.value(KEY_LAST_CHECK_TIMESTAMP, QString::fromLatin1("1970-01-01T00:00:00Z")));
    }
};

UpdateManager::UpdateManager(RemoteType remoteType, const QString &owner, const QString &repo, const QString &currentVersion, const int app_type, const QString& build_os, bool autoCheck, QObject *parent)
    : QObject(parent)
    , m_impl(new Impl(this, remoteType, owner, repo, currentVersion, app_type, build_os, autoCheck))
{
    m_initAutoCheckFlag = false;
    m_currentVersion=currentVersion;
    connect(m_impl->checker, SIGNAL(updateNotFound(const ReleaseInfo&)), this, SLOT(onUpdateNotFound(const ReleaseInfo&)));
    connect(m_impl->checker, SIGNAL(updateFound(const ReleaseInfo&, const QList<ReleaseInfo>&)), this, SLOT(onUpdateFound(const ReleaseInfo&, const QList<ReleaseInfo>&)));
    connect(m_impl->checker, SIGNAL(updateError(const QString&)), this, SLOT(onUpdateError(const QString&)));

    //connect(&m_impl->updateTimer, SIGNAL(timeout()), this, SLOT(displayCurrentVersionInfo()));

    m_impl->updateTimer.setInterval(2500);
    m_impl->updateTimer.setSingleShot(true);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    m_impl->updateTimer.setTimerType(Qt::CoarseTimer);
#endif
    if(autoCheck && m_impl->updaterSettings.value(KEY_AUTO_CHECK_FOR_UPDATES).toBool())
    {
        const QDateTime lastCheck = QDateTime::fromString(m_impl->updaterSettings.value(KEY_LAST_CHECK_TIMESTAMP).toString(), Qt::ISODate);
        if(!lastCheck.isValid() || QDateTime::currentDateTime() >= lastCheck.addDays(DAYS_TO_NEXT_AUTO_CHECK))
            m_impl->updateTimer.start();
    }

#ifdef _DEBUG
    m_releaseVersion = "DEBUG VERSION";
    m_releaseUrl = "DEBUG URL";
    m_releaseNote = "DEBUG NOTE";
#endif
}

UpdateManager::~UpdateManager()
{
}

void UpdateManager::setAutoCheck(bool autoCheck)
{
    m_impl->updaterSettings.setValue(KEY_AUTO_CHECK_FOR_UPDATES,autoCheck);
}

bool UpdateManager::autoCheck()
{
    return  m_impl->updaterSettings.value(KEY_AUTO_CHECK_FOR_UPDATES).toBool();
}

void UpdateManager::skipVersion(QString version)
{
    m_impl->updaterSettings.setValue(KEY_SKIPPED_VERSION,version);
}

void UpdateManager::checkForUpdates(bool silent)
{
    if(m_impl->inProgress)
    {
        if(silent)
            return;
    }

    if (!silent && !m_impl->updaterSettings.value(KEY_AUTO_CHECK_FOR_UPDATES).toBool())
    {
        return;
    }

    m_impl->updateTimer.start();
    m_impl->inProgress = true;
    m_impl->silent = silent;
    m_impl->checker->checkForUpdates();
    m_impl->updaterSettings.setValue(KEY_LAST_CHECK_TIMESTAMP, QDateTime::currentDateTime().toString(Qt::ISODate));
}

void UpdateManager::checkForUpdates()
{
    checkForUpdates(false);
}

void UpdateManager::forceCheckForUpdates()
{
    checkForUpdates(true);
}

void UpdateManager::onUpdateNotFound(const ReleaseInfo &currentRelease)
{
    m_impl->updateTimer.stop();
    m_impl->inProgress = false;
    if (!m_initAutoCheckFlag) {
        //emit updateNotFound();
    }
    else {
        m_initAutoCheckFlag = false;
        return;
    }
    displayCurrentVersionInfo();
}

void UpdateManager::displayCurrentVersionInfo()
{
    QString fname = getUpdateLogFileName();
    fname = QCoreApplication::applicationDirPath() + "/doc/" + fname;
    if (!QFile::exists(fname))
    {
        fname = QCoreApplication::applicationDirPath() + "/doc/" + "en.md";
    }
    QString default_md = MDEditor::instance().readMarkdownFile(fname);
    QString html_md = MDEditor::instance().md2html(default_md);
    //emit updateFound(newReleases[0].tagName, newReleases[0].url, newReleases[0].body);
    emit updateFound("null", QUrl("127.0.0.1"), html_md);//"null" is keyword for qml function
}

void UpdateManager::onUpdateFound(const ReleaseInfo& currentRelease, const QList<ReleaseInfo>& newReleases)
{
    const Version skippedVersion = Version(m_impl->updaterSettings.value(KEY_SKIPPED_VERSION).toString());
    m_impl->inProgress = false;
    m_impl->updateTimer.stop();
    if (skippedVersion == newReleases.first().version)
    {
        if (!m_impl->silent)
        {
            return;
        }
        //onUpdateNotFound(currentRelease);
        displayCurrentVersionInfo();
        return;
    }
    if (currentRelease.version >= newReleases.first().version)
    {
        if (!m_impl->silent)
        {
            return;
        }
        //onUpdateNotFound(currentRelease);
        displayCurrentVersionInfo();
        return;
    }

    if (newReleases.length() > 0)
    {
        m_releaseVersion = newReleases[0].tagName;
        m_releaseUrl = newReleases[0].url;
        m_releaseNote = newReleases[0].body;

        //QString default_md = newReleases[0].body;
        //QString html_md = MDEditor::instance().md2html(default_md);
        emit updateFound(newReleases[0].tagName, newReleases[0].url, newReleases[0].body);
        //emit updateFound(newReleases[0].tagName, newReleases[0].url, html_md);
    }
    else
    {
        displayCurrentVersionInfo();
    }


    m_impl->inProgress = false;
}

QString UpdateManager::getUpdateLogFileName()
{
    QSettings setting;
    setting.beginGroup("language_perfer_config");
    QString fname = setting.value("language_type","en.ts").toString();
    fname.replace(".ts", ".md");
    return fname;
}

void UpdateManager::onUpdateError(const QString &errorString)
{
    m_impl->updateTimer.stop();
    m_impl->inProgress = false;
    if (!m_initAutoCheckFlag) {
        //emit updateNotFound();
    }
    else {
        m_initAutoCheckFlag = false;
    }
    displayCurrentVersionInfo();
}

QString UpdateManager::getCurrentVersion()
{
    return m_currentVersion;
}

void UpdateManager::initAutoCheckVersionFlag()
{
    if(m_impl->updaterSettings.value(KEY_AUTO_CHECK_FOR_UPDATES).toBool())
    {
        m_initAutoCheckFlag = true;
    }
    else
    {
        m_initAutoCheckFlag = false;
    }
}

QString UpdateManager::version()
{
    return m_releaseVersion;
}

QUrl UpdateManager::url()
{
    return m_releaseUrl;
}

QString UpdateManager::releaseNote()
{
    return m_releaseNote;
}

QString UpdateManager::getProjectName()
{
    return m_impl->ownerstr;//qApp->applicationName();
}
