#include "visscenehandler.h"

#include "interface/selectorinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/modelinterface.h"
#include "interface/camerainterface.h"
#include "interface/spaceinterface.h"
#include "qtuser3d/camera/cameracontroller.h"
#include "qtuser3d/camera/screencamera.h"
#include "interface/commandinterface.h"

#include "external/data/modeln.h"

#include <QApplication>
#include <QTimer>

namespace creative_kernel
{
	VisSceneHandler::VisSceneHandler(QObject* parent)
		:QObject(parent)
	{
		addUIVisualTracer(this);

		connect(cameraController(), &qtuser_3d::CameraController::signalRightMousePressed, this, [=](bool pressed)
		{
			if (pressed) 
				QApplication::setOverrideCursor(QCursor(m_rotatePixmap));
			else
				QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));

		});
		connect(cameraController(), &qtuser_3d::CameraController::signalMidMousePressed, this, [=](bool pressed)
		{
			if (pressed)
				QApplication::setOverrideCursor(QCursor(m_movePixmap));
			else
				QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
		});
	}	

	VisSceneHandler::~VisSceneHandler()
	{
	}

	void VisSceneHandler::updateCameraCenter()
	{
		QList<ModelN*> models = creative_kernel::selectionms();
		if (models.empty())
		{
			cameraController()->setRotateCenter(visCamera()->orignCenter());
		}
		else
		{
			QTimer::singleShot(100, [=]()
				{
					QVector3D allCenter;
					for (auto m : models)
					{
						allCenter += m->boxWithSup().center();
					}
					allCenter /= models.count();
					cameraController()->setRotateCenter(allCenter);
				});
		}
	}

	void VisSceneHandler::onThemeChanged(ThemeCategory category)
	{
		if (category == ThemeCategory::tc_dark)
		{
			m_selectPixmap = QPixmap(":/UI/photo/select_dark.png"); 
			m_movePixmap = QPixmap(":/UI/photo/move_dark.png"); 
			m_rotatePixmap = QPixmap(":/UI/photo/rotate_dark.png"); 
		}
		else if (category == ThemeCategory::tc_light)
		{
			m_selectPixmap = QPixmap(":/UI/photo/select_light.png");
			m_movePixmap = QPixmap(":/UI/photo/move_light.png"); 
			m_rotatePixmap = QPixmap(":/UI/photo/rotate_light.png"); 
		}
	}

	void VisSceneHandler::onLanguageChanged(MultiLanguage language)
	{

	}

	void VisSceneHandler::onSelectionsChanged()
	{
		updateCameraCenter();
	}

	void VisSceneHandler::selectChanged(qtuser_3d::Pickable* pickable)
	{

	}

	void VisSceneHandler::onHoverEnter(QHoverEvent* event)
	{

	}

	void VisSceneHandler::onHoverLeave(QHoverEvent* event)
	{
		if (shouldMultipleSelect() && m_didSelectModelAtPress == false)
		{
			dismissRectangleSelector();
			requestVisUpdate(false);
		}
	}

	void VisSceneHandler::onHoverMove(QHoverEvent* event)
	{
		if (hoverVis(event->pos()))
			requestVisUpdate(false);
	}

	void VisSceneHandler::onKeyPress(QKeyEvent* event)
	{
		if (event->key() == Qt::Key_Delete)//�û��ڼ����ϰ���Delete��ʱ
		{
			qDebug() << "VisSceneKeyHandler::onKeyPress: delete";
			removeSelectionModels(true);
		}

		if (event->modifiers() == Qt::ControlModifier && (event->key() == Qt::Key_Delete))//Ctrl+Delete ɾ������
		{
			qDebug() << "VisSceneKeyHandler::onKeyPress: ctrl + delete";
			removeAllModel(true);
		}

		if (event->modifiers() == Qt::ControlModifier && (event->key() == Qt::Key_A))//���û�����Ctrl+A��ʱ
		{
			qDebug() << "VisSceneKeyHandler::onKeyPress: ctrl + A";
			selectAll();
		}

		if (event->modifiers() == Qt::ControlModifier && (event->key() == Qt::Key_V))
		{
			qDebug() << "VisSceneKeyHandler::onKeyPress: ctrl + V";
		}
	}

	void VisSceneHandler::onKeyRelease(QKeyEvent* event)
	{
	}

	void VisSceneHandler::onLeftMouseButtonPress(QMouseEvent* event)
	{
		qtuser_3d::Pickable* pickable = checkPickable(event->pos(), nullptr);
		if (pickable == NULL)
		{
			if (shouldMultipleSelect())
			{
				QPoint p = event->pos();
				m_posOfLeftMousePress = p;
				m_didSelectModelAtPress = didSelectAnyEntity(p);
			} 

			requestVisUpdate();
		}
		else 
		{
			m_didSelectModelAtPress = true;
			if (!selectionms().contains((ModelN*)pickable))
				selectVis(event->pos(), event->modifiers() == Qt::KeyboardModifier::ControlModifier);
		}
	}

	void VisSceneHandler::onLeftMouseButtonRelease(QMouseEvent* event)
	{
		if (shouldMultipleSelect() && m_didSelectModelAtPress == false)
		{
			QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
			dismissRectangleSelector();

			QRect rect = QRect(m_posOfLeftMousePress, event->pos());
			if (rect.size().isNull()) return;
			int width = abs(rect.width()), height = abs(rect.height());
			if (width <= 1 || height <= 1) return;

			selectArea(rect);
			requestVisUpdate();
			// updateCameraCenter();
		}

	}

	void VisSceneHandler::onLeftMouseButtonMove(QMouseEvent* event)
	{
		if (shouldMultipleSelect() && m_didSelectModelAtPress == false)
		{
			if (!QApplication::overrideCursor() || QApplication::overrideCursor()->shape() == Qt::ArrowCursor)
				QApplication::setOverrideCursor(QCursor(m_selectPixmap));

			QRect rect = QRect(m_posOfLeftMousePress, event->pos());
			if (rect.size().isNull()) return;
			int width = abs(rect.width()), height = abs(rect.height());
			if (width <= 1 || height <= 1) return;

			showRectangleSelector(rect);
			requestVisUpdate();
		}
	}

	void VisSceneHandler::onLeftMouseButtonClick(QMouseEvent* event)
	{
		QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));

		/* cancel select by click empty space */
		qtuser_3d::Pickable* pickable = checkPickable(event->pos(), nullptr);
		if (pickable == NULL)	
			selectVis(event->pos(), event->modifiers() == Qt::KeyboardModifier::ControlModifier);
	}

}