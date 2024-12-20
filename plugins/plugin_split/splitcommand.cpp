#include "splitcommand.h"
#include "splitop.h"

#include "interface/visualsceneinterface.h"
#include "kernel/translator.h"
#include "interface/selectorinterface.h"

#include "kernel/kernel.h"
#include "kernel/kernelui.h"

#include "cxkernel/interface/undointerface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

using namespace creative_kernel;
SplitCommand::SplitCommand(QObject* parent)
	:ToolCommand(parent)
	, m_op(nullptr)
{
    m_name = tr("Split") + ": C";
    orderindex = 7;
	m_source = "qrc:/split/split/split.qml";
    addUIVisualTracer(this,this);
}

SplitCommand::~SplitCommand()
{
}

void SplitCommand::onThemeChanged(creative_kernel::ThemeCategory category)
{
    setDisabledIcon("qrc:/UI/photo/cToolBar/cut_dark_disable.svg");
    setEnabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/cToolBar/cut_dark_default.svg" : "qrc:/UI/photo/cToolBar/cut_light_default.svg");
    setHoveredIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/cToolBar/cut_dark_default.svg" : "qrc:/UI/photo/cToolBar/cut_light_default.svg");
    setPressedIcon("qrc:/UI/photo/cToolBar/cut_dark_press.svg");
}

void SplitCommand::onLanguageChanged(creative_kernel::MultiLanguage language)
{
    m_name = tr("Split") + ": C";
}

void SplitCommand::slotMouseLeftClicked()
{
    message();
}

bool SplitCommand::message()
{
    if (m_op->getMessage())
    {
        requestQmlDialog(this, "deleteSupportDlg");
    }

    return true;
}

void SplitCommand::setMessage(bool isRemove)
{
    m_op->setMessage(isRemove);
}

void SplitCommand::execute()
{
	if (!m_op)
	{
		m_op = new SplitOp(this);
       connect(m_op, SIGNAL(posionChanged()), this, SIGNAL(onPositionChanged()));
       connect(m_op, SIGNAL(offsetChanged()), this, SIGNAL(onOffsetChanged()));
       connect(m_op, SIGNAL(rotateAngleChanged()), this, SLOT(slotRotateAngleChanged()));
       connect(m_op, SIGNAL(mouseLeftClicked()), this, SLOT(slotMouseLeftClicked()));
           //rotateAngleChanged
	}
    
    if (!m_op->getShowPop())
    {
        //没有选择模型，不弹出操作界面
        /*QList<ModelN*> selections = selectionms();
        if (selections.size() < 1)
        {           
            return;
        }*/

        setVisOperationMode(m_op);
        //setDefauletDir(this->dir());
        setDefauletDir(QVector3D(0, 0, 0));
        setDefauletPos(this->position());
    }
    else
    {
        setVisOperationMode(nullptr);
    }
    sensorAnlyticsTrace("Model Editing & Layout", "Cut");
}

void SplitCommand::slotRotateAngleChanged()
{
    qDebug()<<"slotRotateAngleChanged!";
    emit onRotateAngleChanged();
}

void SplitCommand::split()
{
	if (m_op) m_op->split();
}
void SplitCommand::changeOffset(float off)
{
    if (m_op)
    {
        m_op->setOffset(m_op->toOffsetInner(off));
        //emit onOffsetChanged();
    }
}

void SplitCommand::changeAxisType(int type)
{
    m_op->setAcitiveAxis(type);
}
void SplitCommand::indicateEnabled(bool bRet)
{
    if (m_op)
    {
        m_op->enabledIndicate(bRet);
    }
}
void SplitCommand::setCutToParts(bool bRet)
{
    if (m_op)
    {
        m_op->setCutToParts(bRet);
    }
}

bool SplitCommand::getCutToParts() const
{
    if (m_op)
    {
        return m_op->getCutToParts();
    }
    return false;
}
void SplitCommand::modelGap(float gap)
{
    if (m_op)
    {
        m_op->setModelGap(gap);
    }
}


float SplitCommand::getOffset()
{
    qDebug() << "m_op->plane().center =" << m_op->plane().center;
    return m_op->getOffset();
}

float SplitCommand::getGap()
{
    return m_op->getGap();
}

QVector3D SplitCommand::position()
{
    if (m_op)
        return m_op->plane().center;
    else
        return QVector3D(0,0,0);
}

QVector3D SplitCommand::defauletPos() {
    this->setPositon(m_DefauletPos);
    return m_DefauletPos;
}

QVector3D SplitCommand::defauletDir() {
    this->setDir(m_DefauletDir);
    return m_DefauletDir;
}

QVector3D SplitCommand::dir()
{
    if (m_op)
    {
        return m_op->planeRotateAngle();
        //return m_op->plane().dir;
    }
    else {
        return QVector3D(0,0,0);
    }
}

void SplitCommand::setPositon(QVector3D pos)
{
    if (m_op) m_op->setPlanePosition(pos);
}

void SplitCommand::setDir(QVector3D d)
{
    if (m_op) m_op->setPlaneDir(d);
}

void SplitCommand::setDefauletPos(QVector3D pos)
{
    m_DefauletPos = pos;
}

void SplitCommand::setDefauletDir(QVector3D pos)
{
    m_DefauletDir = pos;
}

bool SplitCommand::checkSelect()
{
    QList<ModelN*> selections = selectionms();
    if(selections.size()>0)
    {
        return true;
    }
    return false;
}

void SplitCommand::blockSignalSplitChanged(bool block)
{
    if (m_op)
    {
        m_op->blockSignals(block);
    }
}

void SplitCommand::undo()
{
    cxkernel::undo();
}

void SplitCommand::accept()
{
    setMessage(true);
}

void SplitCommand::cancel()
{
    setMessage(false);
    getKernelUI()->switchPickMode();
}
//
// void SplitCommand::resetCmd()
//{
//     m_op->setPlanePosition(m_DefauletPos);
//     m_op->setPlaneDir(m_DefauletDir);
//}
//
// void SplitCommand::setQmlPosition(float val, int nXYZFlag)
//{
//    if (nXYZFlag > 2)return;
//    QVector3D oldPos = position();
//    switch (nXYZFlag)
//    {
//    case 0:
//        m_op->setPlanePosition(QVector3D(val, oldPos.y(), oldPos.z()));
//        break;
//    case 1:
//        m_op->setPlanePosition(QVector3D(oldPos.x(), val, oldPos.z()));
//        break;
//    case 2:
//        m_op->setPlanePosition(QVector3D(oldPos.x(), oldPos.y(), val));
//        break;
//    default:
//        break;
//    }
//}
//
//void SplitCommand::setQmlRotate(float val, int nXYZFlag)
//{
//    if (nXYZFlag > 2)return;
//    QVector3D oldRotate = dir();
//    switch (nXYZFlag)
//    {
//    case 0:
//        m_op->setPlaneDir(QVector3D(val, oldRotate.y(), oldRotate.z()));
//        break;
//    case 1:
//        m_op->setPlaneDir(QVector3D(oldRotate.x(), val, oldRotate.z()));
//
//        break;
//    case 2:
//        m_op->setPlaneDir(QVector3D(oldRotate.x(), oldRotate.y(), val));
//        break;
//    default:
//        break;
//    }
//}
