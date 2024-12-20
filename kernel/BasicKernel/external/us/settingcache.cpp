#include "settingcache.h"
#include <QDir>
#include <QStandardPaths>
#include "qtusercore/string/resourcesfinder.h"
namespace us
{
    SettingCache::SettingCache(QObject *parent,QString machineName) : USettings(parent),m_machineName(machineName)
    {

    }
    void SettingCache::save()
    {

    }
    void SettingCache::load()
    {
        this->loadDefault(getProfilePath());
    }
    QString SettingCache::getProfilePath()
    {
        QString Directory = qtuser_core::getOrCreateAppDataLocation("") + "/Profiles/";
        QDir dir(Directory);
        if(!dir.exists())
            dir.mkdir(Directory);

        return dir.absolutePath()+m_machineName;
    }
}
