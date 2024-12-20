#include "qmlui.h"
#include <QtQml/QQmlComponent>
#include <QtCore/QDebug>
#include <QtQuick/QQuickImageProvider>

namespace cxkernel
{
	QmlUI::QmlUI(QObject* parent)
		: QObject(parent)
		, m_rootWindow(nullptr)
		, m_engine(nullptr)
		, m_context(nullptr)
		, m_expression(nullptr)
		, m_jsEngine(new QJSEngine(this))
	{

	}

	QmlUI::~QmlUI()
	{

	}

	QObject* QmlUI::createQmlObjectFromQrc(const QString& fileName)
	{
		QQmlComponent component(m_engine, QUrl::fromLocalFile(fileName));
		return component.create(m_context);
	}

	void QmlUI::registerContextObject(const QString& name, QObject* object)
	{
		if (!m_engine)
		{
			qWarning() << "QmlUI::registerContextObject: [Null Engine]";
			return;
		}

		if (!object)
			return;

		if (!object->parent())
		{
			qWarning() << "QmlUI::registerContextObject: [object without parent!!!]";
			m_engine->setObjectOwnership(object, QQmlEngine::ObjectOwnership::CppOwnership);
		}

		m_engine->rootContext()->setContextProperty(name, object);
	}

	void QmlUI::registerImageProvider(const QString& name, QQuickImageProvider* provider)
	{
		if (!m_engine)
		{
			qWarning() << "QmlUI::registerImageProvider: [Null Engine]";
			return;
		}

		m_engine->addImageProvider(name, provider);
	}

	void QmlUI::removeImageProvider(const QString& name)
	{
		if (!m_engine)
		{
			qWarning() << "QmlUI::removeImageProvider: [Null Engine]";
			return;
		}

		m_engine->removeImageProvider(name);
	}

	void QmlUI::setObjectOwnership(QObject* object)
	{
		if (!m_engine)
		{
			qWarning() << "QmlUI::setObjectOwnerShip: [Null Engine]";
			return;
		}

		m_engine->setObjectOwnership(object, QQmlEngine::CppOwnership);
	}

	void QmlUI::setEngine(QQmlEngine* engine, QQmlContext* context)
	{
		m_engine = engine;
		m_context = context;
	}

	void QmlUI::registerRootWindow(QObject* object)
	{
		m_rootWindow = object;

		m_expression = new QQmlExpression(m_context, m_rootWindow, QString(""), this);
	}

	QObject* QmlUI::itemByName(const QString& name)
	{
		if (!m_rootWindow)
			return nullptr;

		if (name.isEmpty())
			return m_rootWindow;

		return m_rootWindow->findChild<QObject*>(name);
	}

	void QmlUI::invokeItemFunc(const QString& name, const QString& func, const QVariant& variant1, const QVariant& variant2)
	{
		QObject* object = itemByName(name);

		if (!object)
			return;

		if (variant1.isNull())
			QMetaObject::invokeMethod(object, func.toStdString().c_str());
		else if (variant2.isNull())
			QMetaObject::invokeMethod(object, func.toStdString().c_str(), Q_ARG(QVariant, variant1));
		else
			QMetaObject::invokeMethod(object, func.toStdString().c_str(), Q_ARG(QVariant, variant1), Q_ARG(QVariant, variant2));
	}

	QVariant QmlUI::invokeQmlJs(const QString& script)
	{
		qDebug() << QString("QmlUI::invokeQmlJs : [%1]").arg(script);

		m_expression->setExpression(script);
		QVariant ret = m_expression->evaluate();

		if (m_expression->hasError())
		{
			qDebug() << QString("Qml JavaSript Error [%1].").arg(m_expression->error().toString());
			m_expression->clearError();
		}
		return ret;
	}

	bool QmlUI::invokeQmlJsRt(const QString& script)
	{
		qDebug() << QString("QmlUI::invokeQmlJs : [%1]").arg(script);

		m_expression->setExpression(script);
		QVariant ret = m_expression->evaluate();

		if (m_expression->hasError())
		{
			qDebug() << QString("Qml JavaSript Error [%1].").arg(m_expression->error().toString());
			m_expression->clearError();
			return false;
		}
		return true;
	}

	QJSValue QmlUI::invokeJS(const QString& script, const QString& name, QObject* context)
	{
		if (!context || name.isEmpty()) 
		{
			qDebug() << QString("QmlUI::invokeJS . name ,context error.");
			return QJSValue();
		}

		qDebug() << QString("QmlUI::invokeJS : [%1]").arg(script);
		m_jsEngine->globalObject().deleteProperty(name);
		m_jsEngine->globalObject().setProperty(name, m_engine->newQObject(context));

		QJSValue ret = m_jsEngine->evaluate(script);

		if (ret.isUndefined() || ret.isError())
		{
			qDebug() << QString("JavaSript Error.");
			qDebug() << QString("[%1] : [%2]").arg(ret.property("name").toString())
				.arg(ret.property("message").toString());
			//qDebug() << QString("[line] : [%1]").arg(ret.property("lineNumber").toInt());
		}

		return ret;
	}
}