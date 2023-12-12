#ifndef CREATIVE_KERNEL_PARAMETERUTIL_1676022156634_H
#define CREATIVE_KERNEL_PARAMETERUTIL_1676022156634_H
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace cxkernel
{
    // object has name func
	template<class T>
	QStringList objectNames(const QList<T*>& objects)
	{
        QStringList list;
        for (T* object : objects)
        {
            if (!list.contains(object->name()))
                list.append(object->name());
            else
            {
                qWarning() << QString("Dumplicate object name [%1]").arg(object->name());
            }
        }
        return list;
	}

    template<class T>
    QStringList objectUniqueNames(const QList<T*>& objects)
    {
        QStringList list;
        for (T* object : objects)
        {
            if (!list.contains(object->uniqueName()))
                list.append(object->uniqueName());
            else
            {
                qWarning() << QString("Dumplicate object name [%1]").arg(object->uniqueName());
            }
        }
        return list;
    }

    // object has name func
    template<class T>
    T* findObject(const QString& uniqueName, const QList<T*>& objects)
    {
        T* result = nullptr;
        for (T* object : objects)
        {
            if (object->uniqueName() == uniqueName)
            {
                result = object;
                break;
            }
        }
        return result;
    }
}

#endif // CREATIVE_KERNEL_PARAMETERUTIL_1676022156634_H