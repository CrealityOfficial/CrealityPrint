#include "dataserial.h"
#include "qtusercore/module/systemutil.h"
#include <QDir>
#include <QStandardPaths>
#include "qtusercore/string/resourcesfinder.h"
#include "dataidentifier.h"
#include <external/interface/projectinterface.h>
namespace creative_kernel
{
    DataSerial::DataSerial(int id, const QString& serialName)
    {
        m_id = id;
        m_serialName = serialName;
        m_serialized = false;
        m_identifier = new DataIdentifier();
        m_delay = 600000;

        m_storeLaterTimer.setSingleShot(true);
        connect(&m_storeLaterTimer, &QTimer::timeout, this, [=] ()
        {
            store();
        });

        m_clearLaterTimer.setSingleShot(true);
        connect(&m_clearLaterTimer, &QTimer::timeout, this, [=] ()
        {
            clear();
            m_isCleared = true;
        });
        
    }

    DataSerial::~DataSerial() 
    {
        m_storeLaterTimer.stop();
        m_clearLaterTimer.stop();
    }
	
	bool DataSerial::serialized()
    {
        return m_serialized || m_storeLaterTimer.isActive();
    }

    int DataSerial::ID() 
    { 
        return m_id; 
    }

    bool DataSerial::operator==(const DataSerial& other) const
    {
        return *m_identifier == *other.m_identifier;
    }

    bool DataSerial::operator!=(const DataSerial& other) const
    {
        return *m_identifier != *other.m_identifier;
    }

	QString DataSerial::serialFileName()
    {
        QString strPath = creative_kernel::projectUI()->getAutoProjectDirPath()+"/3D/Objects/RecentSerializeModel/";
        QDir d(strPath);
        if (!d.exists())
        {
            d.mkdir(strPath);
        }

        return strPath + "/" + QString("%1%2.serial").arg(m_serialName).arg(m_id);
    }
    
	void DataSerial::storeLater(int laterMs)
    {
        if (laterMs >= 0)
            m_storeLaterTimer.start(laterMs);
        else
            m_storeLaterTimer.start(m_delay);
    }
    
	void DataSerial::clearLater(int laterMs)
    {
        if (laterMs >= 0)
            m_clearLaterTimer.start(laterMs);
        else
            m_clearLaterTimer.start(m_delay);
    }

}