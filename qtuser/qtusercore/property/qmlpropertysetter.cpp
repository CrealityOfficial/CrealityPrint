#include "qmlpropertysetter.h"
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlProperty>

#include <QtCore/QDebug>
namespace qtuser_qml
{
	void writeProperty(QObject* object, const QString& name, const QVariant& value)
	{
		QQmlProperty::write(object, name, value);
	}

	void writeObjectProperty(QObject* object, const QString& name, QObject* value)
	{
		QQmlProperty::write(object, name, QVariant::fromValue<QObject*>(value));
	}

    void invokeFunc(QObject* obj, const char* member, QObject* receiver,
        QGenericArgument val0,
        QGenericArgument val1,
        QGenericArgument val2,
        QGenericArgument val3,
        QGenericArgument val4,
        QGenericArgument val5,
        QGenericArgument val6,
        QGenericArgument val7,
        QGenericArgument val8)
    {
        QMetaObject::invokeMethod(obj, member, Q_ARG(QVariant, QVariant::fromValue(receiver))
            , val0, val1, val2, val3, val4, val5, val6, val7, val8);
    }

    void invokeFunc(QObject* object, const QString& func,
        QGenericArgument val0,
        QGenericArgument val1,
        QGenericArgument val2,
        QGenericArgument val3,
        QGenericArgument val4,
        QGenericArgument val5,
        QGenericArgument val6,
        QGenericArgument val7,
        QGenericArgument val8)
    {
        QMetaObject::invokeMethod(object, func.toStdString().c_str(), val0, val1, val2, val3, val4, val5, val6, val7, val8);
    }

    void invokeQmlObjectFunc(QObject* object, const QString& func, const QVariant& variant1,
        const QVariant& variant2, const QVariant& variant3)
    {
        if (!variant2.isValid())
            invokeFunc(object, func, Q_ARG(QVariant, variant1));
        else if (!variant3.isValid())
            invokeFunc(object, func, Q_ARG(QVariant, variant1), Q_ARG(QVariant, variant2));
        else
            invokeFunc(object, func, Q_ARG(QVariant, variant1), Q_ARG(QVariant, variant2), Q_ARG(QVariant, variant3));
    }

    void writeObjectNestProperty(QObject* object, const QString& childNest, const QString& name, QObject* value)
    {
        QVariant objectV = QVariant::fromValue<QObject*>(value);
        writeObjectNestProperty(object, childNest, name, objectV);
    }

    void writeObjectNestProperty(QObject* object, const QString& childNest, const QString& name, const QVariant& value)
    {
        if (!object)
            return;

        QStringList nests = childNest.split(".");
        QObject* target = object;
        for (const QString& nest : nests)
        {
            QObject* child = object->findChild<QObject*>(nest);
            if (!child)
                qDebug() << "can't find child " << nest << " in " << target;

            target = child;
            if (!target)
                break;
        }
        if (target)
            QQmlProperty::write(target, name, value);
    }
}