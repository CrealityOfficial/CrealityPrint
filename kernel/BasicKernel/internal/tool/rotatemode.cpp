#include "rotatemode.h"
#include "operation/rotateop.h"
#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"
#include "kernel/kernelui.h"

namespace creative_kernel
{
	RotateMode::RotateMode(QObject* parent)
		:ToolCommand(parent)
		, m_op(nullptr)
	{
		orderindex = 3;
		m_name = tr("Rotate") + ": R";

		m_source = "qrc:/kernel/qml/RotatePanel.qml";
		addUIVisualTracer(this,this);
	}

	RotateMode::~RotateMode()
	{
		if (m_op != nullptr)
		{
			delete m_op;
			m_op = nullptr;
		}
	}

	void RotateMode::onThemeChanged(ThemeCategory category)
	{
		setDisabledIcon("qrc:/UI/photo/cToolBar/rotate_dark_disable.svg");
		setEnabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/cToolBar/rotate_dark_default.svg": "qrc:/UI/photo/cToolBar/rotate_light_default.svg");
		setHoveredIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/rotate_pressed.svg" : "qrc:/UI/photo/leftBar/rotate_lite.svg");
		setPressedIcon("qrc:/UI/photo/cToolBar/rotate_dark_press.svg");
	}

	void RotateMode::onLanguageChanged(MultiLanguage language)
	{
		m_name = tr("Rotate") + ": R";
	}

	void RotateMode::slotMouseLeftClicked()
	{
		//message();
	}

	void RotateMode::execute()
	{
		if (!m_op)
		{
			m_op = new RotateOp(this);
			connect(m_op, SIGNAL(rotateChanged()), this, SIGNAL(rotateChanged()), Qt::UniqueConnection);
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
		emit rotateChanged();
	}

	void RotateMode::reset()
	{
		m_op->reset();
	}

	QVector3D RotateMode::rotate()
	{
		return m_op->rotate();
	}

	/*void RotateMode::setRotate(QVector3D position)
	{
		m_op->setRotate(position);
	}*/

	void RotateMode::setQmlRotate(float val, int nXYZFlag)
	{
		if (nXYZFlag > 2)return;
		QVector3D oldRotate = m_op->rotate();
		switch (nXYZFlag)
		{
		case 0:
			m_op->setRotate(QVector3D(val, oldRotate.y(), oldRotate.z()));
			break;
		case 1:
			m_op->setRotate(QVector3D(oldRotate.x(), val, oldRotate.z()));

			break;
		case 2:
			m_op->setRotate(QVector3D(oldRotate.x(), oldRotate.y(), val));
			break;
		default:
			break;
		}
	}

	/*Q_INVOKABLE void RotateMode::startRotate()
	{
		m_op->startRotate();
	}*/

	bool RotateMode::checkSelect()
	{
		QList<ModelN*> selections = selectionms();
		if (selections.size() > 0)
		{
			return true;
		}
		return false;
	}

	void RotateMode::blockSignalScaleChanged(bool block)
	{
		if (m_op)
		{
			m_op->blockSignals(block);
		}
	}
}
