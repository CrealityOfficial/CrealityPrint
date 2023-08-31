#include "wizardui.h"
#include <QQuickItem>
#include <QGuiApplication>
#include <QApplication>
#include <QFontMetrics>

WizardUI::WizardUI(QObject *parent) : QObject(parent)
{

}

WizardUI::WizardUI(QObject *rootObject, QObject *parent) : QObject(parent), pRootObject(rootObject)
{

}

WizardUI::~WizardUI()
{

}

void WizardUI::setRootObjet(QObject *rootObj)
{
    pRootObject = rootObj;
}

float WizardUI::getScreenScaleFactor()
{  
#ifdef __APPLE__
    QFont font = qApp->font();
    font.setPointSize(12);
    qApp->setFont(font);
    return 1;
#else
    QFont font = qApp->font();
    font.setPointSize(10);
    qApp->setFont(font);
    //int ass = QFontMetrics(font).ascent();
    float fontPixelRatio = QFontMetrics(qApp->font()).ascent() / 11.0;
    fontPixelRatio = int(fontPixelRatio * 4) / 4.0;
    return fontPixelRatio;
#endif
}

QPoint WizardUI::cursorGlobalPos() const
{
    return QCursor::pos();
}

void WizardUI::findRootByNode(QObject *nodeObject)
{
    pRootObject = nodeObject;

    if (pRootObject)
    {
        QObject *pParent = nullptr;
        while ((pParent = pRootObject->parent()))
        {
            pRootObject = pParent;
        }
    }
}

QRect WizardUI::getItemGeometryToScene(const QString &targetObjName) const
{
    if (!pRootObject) return QRect();
    auto pItem = pRootObject->findChild<QQuickItem *>(targetObjName);

    if (pItem)
    {
        if (pItem->parentItem())
        {
            auto pos = pItem->parentItem()->mapToScene(pItem->position());
            return QRectF(pos.x(), pos.y(), pItem->width(), pItem->height()).toRect();
        }
        else {
            return pItem->boundingRect().toRect();
        }
    }

    return QRect();
}

void WizardUI::setAppOverrideCursor(QCursor cursor)
{
    qApp->setOverrideCursor(cursor);
}

void WizardUI::restoreAppOverrideCursor()
{
    qApp->restoreOverrideCursor();
}

QObject *WizardUI::getObject(const QString &targetObjName) const
{
    if (!pRootObject) return nullptr;
    return pRootObject->findChild<QObject *>(targetObjName);
}

QVariant WizardUI::getObjectProperty(QObject *targetObj, const QString &propertyName) const
{
    return targetObj->property(propertyName.toUtf8().constData());
}

void WizardUI::setObjectProperty(QObject *targetObj, const QString &propertyName, const QVariant &value) const
{
    targetObj->setProperty(propertyName.toUtf8().constData(), value);
}

QVariant WizardUI::getObjectProperty(const QString &targetObjName, const QString &propertyName) const
{
    auto pObj = getObject(targetObjName);
    return (pObj ? getObjectProperty(pObj, propertyName) : QVariant());
}

void WizardUI::setObjectProperty(const QString &targetObjName, const QString &propertyName, const QVariant &value) const
{
    auto pObj = getObject(targetObjName);
    if (pObj) setObjectProperty(pObj, propertyName, value);
}
