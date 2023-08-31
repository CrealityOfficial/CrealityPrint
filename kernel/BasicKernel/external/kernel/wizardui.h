#pragma once
#include <QObject>
#include <QVariant>
#include <QRect>
#include <QCursor>
#include "basickernelexport.h"

class BASIC_KERNEL_API WizardUI : public QObject
{
    Q_OBJECT
public:
    explicit WizardUI(QObject *parent = nullptr);
    explicit WizardUI(QObject *rootObject, QObject *parent = nullptr);
    virtual ~WizardUI() override;
    Q_INVOKABLE void setRootObjet(QObject *rootObj); //{ pRootObject = rootObj; }
    Q_INVOKABLE float getScreenScaleFactor();
    Q_INVOKABLE QPoint cursorGlobalPos() const;
    QObject *rootObject() const { return pRootObject; }
    void findRootByNode(QObject *nodeObject);

private:
    QObject *pRootObject = nullptr;

public slots:
    QObject *getObject(const QString &targetObjName) const;

    QVariant getObjectProperty(QObject *targetObj, const QString &propertyName) const;
    void setObjectProperty(QObject *targetObj, const QString &propertyName, const QVariant &value) const;

    QVariant getObjectProperty(const QString &targetObjName, const QString &propertyName) const;
    void setObjectProperty(const QString &targetObjName, const QString &propertyName, const QVariant &value) const;

    QRect getItemGeometryToScene(const QString &targetObjName) const;

    void setAppOverrideCursor(QCursor cursor);
    void restoreAppOverrideCursor();
};
