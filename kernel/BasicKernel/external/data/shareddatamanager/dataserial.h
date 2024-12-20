#ifndef CREATIVE_KERNEL_DATASERIAL_1681019989200_H
#define CREATIVE_KERNEL_DATASERIAL_1681019989200_H
#include "data/header.h"
#include "data/kernelenum.h"
#include "ccglobal/serial.h"
#include "dataidentifier.h"
#include <QObject>
#include <QTimer>
#include <QMutex>

namespace creative_kernel
{
	class DataIdentifier;
	class BASIC_KERNEL_API DataSerial : public QObject
	{
	public:
		DataSerial(int id, const QString& name);
		virtual ~DataSerial();

		virtual bool load() = 0;
		virtual bool store() = 0;
		virtual void generateIdentifier() = 0;
		virtual void clear() = 0;

		void storeLater(int laterMs = -1);
		void clearLater(int laterMs = -1);
		bool serialized();
		int ID();
		
	    bool operator==(const DataSerial& other) const;
		bool operator!=(const DataSerial& other) const;

	protected:
		QString serialFileName();

	protected:
		int m_id;
		QString m_serialName;
		bool m_serialized;
		DataIdentifier* m_identifier;
		QTimer m_clearLaterTimer;
		QTimer m_storeLaterTimer;
		bool m_isCleared;
		int m_delay;
		QMutex m_mutex;

	};


}

#endif // CREATIVE_KERNEL_DATASERIAL_1681019989200_H