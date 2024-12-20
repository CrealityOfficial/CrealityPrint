#include "scalemode.h"
#include "operation/scaleop.h"
#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"
#include "kernel/kernelui.h"

namespace creative_kernel
{
    ScaleMode::ScaleMode(QObject* parent)
        :ToolCommand(parent)
        , m_op(nullptr)
    {
        orderindex = 1;
        m_name = tr("Scale") + ": Ctrl+R";

        m_source = "qrc:/kernel/qml/ScalePanel.qml";
        addUIVisualTracer(this,this);
    }

    ScaleMode::~ScaleMode()
    {
        if (m_op != nullptr)
        {
            delete m_op;
            m_op = nullptr;
        }
    }

    /*void ScaleMode::setMessage(bool isRemove)
    {
        if (!m_op) return;
        m_op->setMessage(isRemove);
    }

    bool ScaleMode::message()
    {
        if (!m_op) return false;
        if (m_op->getMessage())
        {
            requestQmlDialog(this, "deleteSupportDlg");
        }

        return true;
    }

    void ScaleMode::accept()
    {
        setMessage(true);
    }

    void ScaleMode::cancel()
    {
        setMessage(false);
        getKernelUI()->switchPickMode();
    }*/

    void ScaleMode::onThemeChanged(ThemeCategory category)
    {
        setDisabledIcon("qrc:/UI/photo/cToolBar/scale_dark_disable.svg");
        setEnabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/cToolBar/scale_dark_default.svg" : "qrc:/UI/photo/cToolBar/scale_light_default.svg");
        setHoveredIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/cToolBar/scale_dark_default.svg" : "qrc:/UI/photo/cToolBar/scale_light_default.svg");
        setPressedIcon("qrc:/UI/photo/cToolBar/scale_dark_press.svg");
    }

    void ScaleMode::onLanguageChanged(MultiLanguage language)
    {
        m_name = tr("Scale") + ": Ctrl+R";
    }

    void ScaleMode::slotMouseLeftClicked()
    {
        //message();
    }

    void ScaleMode::execute()
    {
        if (!m_op)
        {
            m_op = new ScaleOp(this);
            connect(m_op, SIGNAL(scaleChanged()), this, SIGNAL(scaleChanged()), Qt::UniqueConnection);
            connect(m_op, SIGNAL(sizeChanged()), this, SIGNAL(sizeChanged()), Qt::UniqueConnection);
            connect(m_op, SIGNAL(mouseLeftClicked()), this, SLOT(slotMouseLeftClicked()), Qt::UniqueConnection);
        }


        if (!m_op->getShowPop())
        {
            setVisOperationMode(m_op);
        }
        else
        {
            setVisOperationMode(nullptr);
        }
        //	emit scaleChanged();
    }

    void ScaleMode::reset()
    {
        if (!m_op) return;
        m_op->reset();
    }

    QVector3D ScaleMode::size()
    {
        //    qDebug()<<"m_op->box() =" << m_op->box();
        //return m_op->box() * m_op->scale();
        if (!m_op) return QVector3D();
        return m_op->globalbox();
    }

    QVector3D ScaleMode::scale()
    {
        if (!m_op) return QVector3D();
        return m_op->scale();
    }

    void ScaleMode::setScale(QVector3D scale)
    {
        qDebug() << "setScale = " << scale;
        if (!m_op) return;
        m_op->setScale(scale);
    }

    QVector3D ScaleMode::orgSize() const
    {
        if (!m_op) return QVector3D();
        ScaleOp* op = (ScaleOp*)(m_op);
        //return op->box()/*/op->scale()*/;
        return op->box()/*/op->scale()*/;

    }

    void ScaleMode::setQmlSize(float sizeValue, int xyzFlag)
    {
        if (xyzFlag > 5)return;
        if (!m_op) return;

        QVector3D size = this->size();
        QVector3D scale = this->scale();

        switch (xyzFlag)
        {
        case 0:
            // m_op->setScale(QVector3D(scaleValue, oldScale.y(), oldScale.z()));
            m_op->setScale(QVector3D(sizeValue / size.x() * scale.x(), m_op->scale().y(), m_op->scale().z()));
            break;
        case 1:
            //   m_op->setScale(QVector3D(oldScale.x(), scaleValue, oldScale.z()));
            m_op->setScale(QVector3D(m_op->scale().x(), sizeValue / size.y() * scale.y(), m_op->scale().z()));
            break;
        case 2:
            m_op->setScale(QVector3D(m_op->scale().x(), m_op->scale().y(), sizeValue / size.z() * scale.z()));
            break;
        case 3:
        {
            float newScale = sizeValue / size.x() * scale.x();
            m_op->setScale(QVector3D(newScale, newScale, newScale));
        }
            break;
        case 4:
        {
            float newScale = sizeValue / size.y() * scale.y();
            m_op->setScale(QVector3D(newScale, newScale, newScale));
        }
            break;
        case 5:
        {
            float newScale = sizeValue / size.z() * scale.z();
            m_op->setScale(QVector3D(newScale, newScale, newScale));
        }
            break;
        default:
            break;
        }
        emit scaleChanged();
        emit sizeChanged();
    }

    void ScaleMode::setQmlScale(float scaleValue, int xyzFlag)
    {
        if (!m_op) return;
        QVector3D oldScale = m_op->scale();
        if (xyzFlag > 3)return;
        switch (xyzFlag)
        {
        case 0:
            m_op->setScale(QVector3D(scaleValue, oldScale.y(), oldScale.z()));
            break;
        case 1:
            m_op->setScale(QVector3D(oldScale.x(), scaleValue, oldScale.z()));
            break;
        case 2:
            m_op->setScale(QVector3D(oldScale.x(), oldScale.y(), scaleValue));
            break;
        case 3:
            m_op->setScale(QVector3D(scaleValue, scaleValue, scaleValue));
        default:
            break;
        }
        emit scaleChanged();
        emit sizeChanged();
    }

    QVector3D ScaleMode::bindScale() const
    {
        if (!m_op) return QVector3D();
        ScaleOp* op = (ScaleOp*)(m_op);
        return op->scale();
    }

    QVector3D ScaleMode::bindSize() const
    {
        if (!m_op) return QVector3D();
        return m_op->box();
    }

    bool ScaleMode::uniformCheck()const
    {
        if (!m_op) return false;
        return m_op->uniformCheck();
    }

    void ScaleMode::setUniformCheck(const bool check)
    {
        if (!m_op) return;
        m_op->setUniformCheck(check);
        emit checkChanged();
    }

    bool ScaleMode::checkSelect()
    {
        QList<ModelN*> selections = selectionms();
        if (selections.size() > 0)
        {
            return true;
        }
        return false;
    }

    void ScaleMode::blockSignalScaleChanged(bool block)
    {
        if (m_op)
        {
            m_op->blockSignals(block);
        }
    }
}
