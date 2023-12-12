#ifndef SETTINGCACHE_H
#define SETTINGCACHE_H

#include <QObject>
#include "cxgcode/us/usettings.h"
namespace us
{
    class SettingCache : public cxgcode::USettings
    {
        Q_OBJECT
    public:
        explicit SettingCache(QObject *parent = nullptr,QString machineName="");
        void save();
        void load();
    protected:
        QString getProfilePath();
    signals:

    public slots:
    private:
        QString m_machineName;
    };
}
#endif // SETTINGCACHE_H
