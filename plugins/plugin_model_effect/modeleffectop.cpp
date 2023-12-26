#include "modeleffectop.h"

#include <QtCore/qnamespace.h>
#include <QtGui/qvector4d.h>
#include <QtCore/qstring.h>
#include "interface/spaceinterface.h"
#include "data/modeln.h"

ModelEffectOp::ModelEffectOp(QObject* parent)
	:SceneOperateMode(parent)
	, m_dialog(nullptr)
	, m_currentItem(nullptr)
{
	m_setter = new ModelEffectSettor();
}

ModelEffectOp::~ModelEffectOp()
{
	disconnect(m_dialog, SIGNAL(currentColorChanged(const QColor&)), this, SLOT(onCurrentColorChanged(const QColor&)));
	delete m_dialog;
}

void ModelEffectOp::onAttach()
{
	EffectItem* item = m_setter->getNextItem();
	while (item)
	{
		m_currentItem = item;
		if (!m_dialog)
		{
			m_dialog = new QColorDialog(nullptr);
			connect(m_dialog, SIGNAL(currentColorChanged(const QColor&)), this, SLOT(onCurrentColorChanged(const QColor&)));
		}
		
		//m_dialog->setModal(true);
		m_dialog->setWindowTitle(item->title);
		QVector4D gain = item->value.value<QVector4D>();
		m_dialog->setCurrentColor(QColor(gain.x() * 255.0, gain.y() * 255.0, gain.z() * 255.0, 255));
		
		if (item->index != -1)
		{
			QList<creative_kernel::ModelN*> modelns = creative_kernel::getModelSpace()->modelns();
			for (creative_kernel::ModelN* model : modelns)
			{
				model->setState(item->index);
			}
		}

		m_dialog->exec();
		m_setter->printParameter();
		m_currentItem = nullptr;

		item = m_setter->getNextItem();
	}
}

void ModelEffectOp::onDettach()
{
}


void ModelEffectOp::onCurrentColorChanged(const QColor& color)
{
	onColorSelected(color);
}

void ModelEffectOp::onColorSelected(const QColor& color)
{
	if (m_currentItem)
	{
		QVector4D v = QVector4D(color.red() / 255.0f, color.green() / 255.0f, color.blue() / 255.0f, 1.0f);
		m_currentItem->value = v;
		m_setter->update(m_currentItem);
	}
}

void ModelEffectOp::onKeyPress(QKeyEvent* event)
{
}
