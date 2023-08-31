#include "supportui.h"
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlProperty>
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QQmlContext>
#include "interface/selectorinterface.h"
#include "supportcommand.h"
#include "simplesupportop.h"
#include "interface/visualsceneinterface.h"
#include "interface/commandinterface.h"
#include "kernel/kernelui.h"

using namespace creative_kernel;
SupportUI::SupportUI(QObject *parent)
    : QObject(parent)
    , m_qmlUI(nullptr)
    , m_pOp(nullptr)
    , m_bFDMType(true)
{
}

void SupportUI::slotSupportModelChange()
{
}

void SupportUI::slotSupportExecute()
{
    getKernelUI()->switchPickMode();
    if(m_bFDMType)
    {
        QQmlProperty::write(m_qmlUI, "visible", QVariant::fromValue(true));
        execute();        
    }
    else
    {
        QQmlProperty::write(m_qmlUI, "visible", QVariant::fromValue(false));
    }
    sensorAnlyticsTrace("Support", "Support (Icon)");
}

void SupportUI::setFDMVisible(bool bFDMType)
{
    m_bFDMType = bFDMType;
}

void SupportUI::showOnKernelUI(bool show)
{
    QQmlProperty::write(m_qmlUI, "parent", QVariant::fromValue<QObject*>(show ? m_root : nullptr));
}

SupportUI::~SupportUI()
{
}

bool SupportUI::hasSupport()
{
    if (m_pOp)
    {
        return m_pOp ->hasSupport();
    }
    return false;
}

void SupportUI::execute()
{
    if(!m_pOp)
    {
        m_pOp = new SimpleSupportOp(this);
    }

    setVisOperationMode(m_pOp);
}

void SupportUI::addMode()
{
    if (m_pOp)
    {
        m_pOp->setAddMode();
    }
}

void SupportUI::deleteMode()
{
    if (m_pOp)
    {
        m_pOp->setDeleteMode();
    }
}

void SupportUI::removeAll()
{
    if (m_pOp)
    {
        m_pOp->deleteAllSupport();
    }
}

void SupportUI::autoAddSupport(qreal size, qreal angle, bool platform)
{
    qDebug() << "support autoAddSupport";
    if (m_pOp)
    {
        m_pOp->setSupportAngle(angle);
        m_pOp->setSupportSize(size);
        m_pOp->autoSupport(platform);
    }
}

void SupportUI::changeSize(qreal size)
{    if (m_pOp)
    {
        m_pOp->setSupportSize(size);
    }
}

void SupportUI::changeAngle(qreal angle)
{
    if (m_pOp)
    {
        m_pOp->setSupportAngle(angle);
    }
}

void SupportUI::slotSupportSelState(bool state)
{
    m_state = state;
}

void SupportUI::onSliceClicked(bool doSlice)
{
    if (doSlice)
    {
        QObject* pmainWinbj = getKernelUI()->getUI("UIAppWindow");
        QMetaObject::invokeMethod(pmainWinbj, "printerConfigBtnClicked");

        QObject* pSlicePanel = getKernelUI()->getUI("rightpanel");
        QObject* pRifhtPanel = pSlicePanel->findChild<QObject*>("RightInfoPanel");
        QMetaObject::invokeMethod(pRifhtPanel, "sliceClickFromSupport");
    }
    else
    {
        sensorAnlyticsTrace("Configuration Profile", "Back to Printing Profile");
    }
}

void SupportUI::backToPrinterConfig()
{
    QObject* pmainWinbj = getKernelUI()->getUI("UIAppWindow");
    QMetaObject::invokeMethod(pmainWinbj, "printerConfigBtnClicked");
}
