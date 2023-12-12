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

#include "UpdateChecker.h"

#include <algorithm>
#include <cassert>

#include <QApplication>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include "ReleaseInfo.h"
#include <QUrlQuery>

#include <QSettings>
#include "qtusercore/module/systemutil.h"
#include "buildinfo.h"

static bool releaseInfoGreater(const ReleaseInfo &lhs, const ReleaseInfo &rhs)
{
    return lhs.version > rhs.version;
}

static bool isPrerelase(const ReleaseInfo &info)
{
    return info.prerelease;
}

struct UpdateChecker::Impl
{
    const QString owner;
    const QString repo;
    const Version currentVersion;
    const int appType;
    int osType;
    QString osVersion;
    QNetworkAccessManager *networkManager;
    QNetworkReply *activeReply;

    Impl(const QString &owner, const QString &repo, const QString &currentVersion, const int app_type, const QString& build_os)
        : owner(owner)
        , repo(repo)
        , currentVersion(currentVersion)
        , appType(app_type)
        , networkManager(Q_NULLPTR)
        , activeReply(Q_NULLPTR)
    {
        QString q_os_type = build_os;
        q_os_type = q_os_type.toLower();
        osType = 0;
        if (q_os_type == "win64")
        {
            osType = 1;
            osVersion = "Win";
        }
        else if (q_os_type == "win32")
        {
            osType = 1;
            osVersion = "Win";
        }
        else if (q_os_type == "linux")
        {
            osType = 2;
            osVersion = "Linux";
        }
        else if (q_os_type == "macx")
        {
            osType = 3;
            osVersion = "Mac";
        }
    }
};

UpdateChecker::UpdateChecker(RemoteType remoteType, const QString &owner, const QString &repo, const QString &currentVersion, const int app_type, const QString& build_os, QObject *parent)
    : QObject(parent)
    , m_impl(new Impl(owner, repo, currentVersion, app_type, build_os))
{

}

UpdateChecker::~UpdateChecker()
{}

void UpdateChecker::checkForUpdates()
{
    if(!m_impl->networkManager)
    {
        m_impl->networkManager = new QNetworkAccessManager(this);
        connect(m_impl->networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    }

    QString duid = "12345678";

//    QString os_version = "Win";
//    int osType = 1;
//#ifdef __APPLE__
//    os_version = "Mac";
//    osType = 3;
//#endif

    QString strContent = QString::fromLatin1("{\"page\": %1, \"pageSize\": %2, \"type\" : %3, \"palform\" : %4}").arg(1).arg(999).arg(m_impl->appType).arg(m_impl->osType);
    auto body = strContent.toLatin1();

    QString cloudUrl;
    QSettings setting;
    setting.beginGroup("cloud_service");
    QString strStartType = setting.value("server_type", "-1").toString();
    if (strStartType == "-1")
    {
        strStartType = "0";
        setting.setValue("server_type", "0");
    }
    setting.endGroup();
    QString versionExtra(PROJECT_VERSION_EXTRA);
    if (versionExtra == "Release")
    {
        if (strStartType == "0")
        {
            cloudUrl = "https://www.crealitycloud.cn";
        }
        else
        {
            cloudUrl = "https://www.crealitycloud.com";
        }
    }
    else
    {
        cloudUrl = "http://model-dev.crealitygroup.com";
    }

    cloudUrl = cloudUrl + "/api/cxy/search/softwareSearch";

    QSettings settingLang;
    settingLang.beginGroup("language_perfer_config");
    QString lang = settingLang.value("language_type", "zh_cn").toString();
    settingLang.endGroup();
    QString currentLanguage;
    //please refer http://yapi.crealitygroup.com/project/8/wiki
    if (lang == "en.ts")
    {
        currentLanguage = "0";
    }
    else if (lang == "zh_CN.ts")
    {
        currentLanguage = "1";
    }
    else if (lang == "zh_TW.ts")
    {
        currentLanguage = "2";
    }
    else
    {
        currentLanguage = "0";
    }

    QNetworkRequest request;
    request.setUrl(QUrl(cloudUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    request.setRawHeader("__CXY_APP_ID_", "creality_model");
    request.setRawHeader("__CXY_OS_LANG_", currentLanguage.toStdString().c_str());
    request.setRawHeader("__CXY_DUID_", duid.toStdString().c_str());
    request.setRawHeader("__CXY_OS_VER_", m_impl->osVersion.toStdString().c_str());
    request.setRawHeader("__CXY_PLATFORM_", "6");
    request.setRawHeader("__CXY_APP_VER_", "3.5.0");

    QSslConfiguration m_sslConfig = QSslConfiguration::defaultConfiguration();
    m_sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    m_sslConfig.setProtocol(QSsl::TlsV1_2);
    request.setSslConfiguration(m_sslConfig);

    m_impl->activeReply = m_impl->networkManager->post(request, body);

}

void UpdateChecker::cancel()
{
    if(!m_impl->activeReply)
        return;
     m_impl->activeReply->abort();
     m_impl->activeReply = Q_NULLPTR;
}

void UpdateChecker::replyFinished(QNetworkReply *reply)
{
    QSettings setting;
    m_impl->activeReply = Q_NULLPTR;

    int osType = 1;
#ifdef __APPLE__
    osType = 3;
#endif

    int state = -1;
    QJsonParseError error;
    ReleaseInfo currentRelease;
    QList<ReleaseInfo> newReleases;

    QNetworkReply::NetworkError aa = reply->error();

    if (reply->error() == QNetworkReply::NoError)
    {
        QByteArray arrayJson = reply->readAll();
        const QJsonDocument document = QJsonDocument::fromJson(arrayJson, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qWarning() << "[getQrinfoFromCloud QJsonDocument]" << error.errorString() << "\n";
        }
        QString strJson(document.toJson(QJsonDocument::Compact));

        QJsonObject object = document.object();
        const QJsonObject result = object.value(QString::fromLatin1("result")).toObject();
        state = object.value(QString::fromLatin1("code")).toInt();

        if (state == 0)
        {
            QJsonArray verList = result.value(QString::fromLatin1("list")).toArray();
            for (int i = 0; i < verList.count(); i++)
            {
                int platform = verList[i].toObject().value(QString::fromLatin1("platform")).toInt();
                if (platform != osType)
                {
                    continue;
                }

                ReleaseInfo info;
                info.url = QUrl(verList[i].toObject().value(QString::fromLatin1("fileUrl")).toString());
                info.tagName = verList[i].toObject().value(QString::fromLatin1("versionNumber")).toString();
                info.createdAt = QDateTime::fromString(verList[i].toObject().value(QString::fromLatin1("createTime")).toString(), Qt::ISODate);
                info.version = Version(info.tagName);
                QJsonArray descriptionlist = verList[i].toObject().value(QString::fromLatin1("description")).toArray();
                QString html = info.tagName + "\n";
                for (int j = 0; j < descriptionlist.count(); j++)
                {
                    html += descriptionlist[j].toString();
                }

                info.body = html + "\n";
                
                if (info.version == m_impl->currentVersion)
                    currentRelease = info;
                else if (info.version > m_impl->currentVersion)
                    newReleases.append(info);
            }
        }
        else
        {
            qWarning() << "[UpdateChecker]" << error.errorString() << "\n";
            Q_EMIT updateError(error.errorString());
            return;
        }
        
    }
    else
    {
        qWarning() << "[UpdateChecker]" << error.errorString() << "\n";
        Q_EMIT updateError(error.errorString());
        return;
    }

    if(currentRelease.version.isValid() && !currentRelease.prerelease)
        newReleases.erase(std::remove_if(newReleases.begin(), newReleases.end(), isPrerelase), newReleases.end());

    if(!currentRelease.version.isValid())
        currentRelease.version = m_impl->currentVersion;

    if(newReleases.empty())
    {
        Q_EMIT updateNotFound(currentRelease);
        return;
    }

    std::sort(newReleases.begin(), newReleases.end(), releaseInfoGreater);
    Q_EMIT updateFound(currentRelease, newReleases);
}

