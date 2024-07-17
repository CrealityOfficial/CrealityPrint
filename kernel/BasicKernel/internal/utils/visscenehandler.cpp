#include "visscenehandler.h"

#include "interface/selectorinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/modelinterface.h"
#include "interface/machineinterface.h"
#include "interface/camerainterface.h"
#include "interface/spaceinterface.h"
#include "qtuser3d/camera/cameracontroller.h"
#include "qtuser3d/camera/screencamera.h"
#include "interface/commandinterface.h"
#include "interface/printerinterface.h"
#include "internal/multi_printer/printermanager.h"
#include "interface/eventinterface.h"

#include "interface/selectorinterface.h"
#include "external/data/modeln.h"
#include "qtuser3d/module/manipulatepickable.h"

#include <QApplication>
#include <QTimer>
#include "entity/alonepointentity.h"
#include "kernel/kernel.h"
#include "internal/rclick_menu/mergemodellocation.h"
#include "kernel/kernelui.h"

namespace creative_kernel
{
	VisSceneHandler::VisSceneHandler(QObject* parent)
		:QObject(parent)
	{
		m_rotateCenterEntity = new qtuser_3d::AlonePointEntity();
		m_rotateCenterEntity->setPointSize(6);
		m_rotateCenterEntity->setFilterType("overlay");
		addUIVisualTracer(this,this);

		connect(cameraController(), &qtuser_3d::CameraController::signalRightMousePressed, this, [=](bool pressed)
		{
			if (pressed) 
			{
				QMatrix4x4 pose;
				QVector3D center = cameraController()->rotateCenter();
				m_rotateCenterEntity->updateGlobal(center);
				if (isUsingDefaultScene())
					visShow(m_rotateCenterEntity);
				else 
					visShowCustom(m_rotateCenterEntity);		

				m_rotating = true;

				int phase = getKernel()->currentPhase();
				if (phase == 1)
				{
					getKernelUI()->setScene3DCursor(QCursor(m_rotatePixmap));
				}
			}
			else
			{
				if (isUsingDefaultScene())
					visHide(m_rotateCenterEntity);
				else 
					visHideCustom(m_rotateCenterEntity);
						
				m_rotating = false;
				getKernelUI()->setScene3DCursor(QCursor(Qt::ArrowCursor));
			}
		});
		connect(cameraController(), &qtuser_3d::CameraController::signalMidMousePressed, this, [=](bool pressed)
		{
			if (pressed)
			{
				getKernelUI()->setScene3DCursor(QCursor(m_movePixmap));
			}
			else
			{
				getKernelUI()->setScene3DCursor(QCursor(Qt::ArrowCursor));
			}
		});

		m_updateCameraCenterTimer.setSingleShot(true);
		connect(&m_updateCameraCenterTimer, &QTimer::timeout, this, [=] ()
		{
			updateCameraCenter();
		});
	}	

	VisSceneHandler::~VisSceneHandler()
	{
	}

	void VisSceneHandler::setEnabled(bool enabled)
	{
		if (enabled) 
		{
			connect(getKernel(), &Kernel::currentPhaseChanged, this, &VisSceneHandler::activeCameraCenterChange, Qt::UniqueConnection);
			// connect(getPrinterManager(), &PrinterMangager::signalPrintersCountChanged, this, &VisSceneHandler::activeCameraCenterChange, Qt::UniqueConnection);
			connect(getPrinterManager(), &PrinterMangager::signalDidSelectPrinter, this, &VisSceneHandler::activeCameraCenterChange, Qt::UniqueConnection);
			prependLeftMouseEventHandler(this);
			prependRightMouseEventHandler(this);
			// addSelectTracer(this);
			addKeyEventHandler(this);
			addHoverEventHandler(this);
		}
		else 
		{
			//disconnect(getPrinterManager(), &PrinterMangager::signalPrintersCountChanged, this, &VisSceneHandler::activeCameraCenterChange);

			// removeSelectorTracer(this);
			removeKeyEventHandler(this);
			removeHoverEventHandler(this);
			removeLeftMouseEventHandler(this);
			removeRightMouseEventHandler(this);
		}
	}

	void VisSceneHandler::updateCameraCenter()
	{
		int phase = getKernel()->currentPhase();
		QList<ModelN*> models = creative_kernel::selectionms();
		if (phase != 0 || models.empty())
		{
			Printer* p = getPrinterManager()->getSelectedPrinter();
			auto center = p->globalBox().center();
			center.setZ(0);
			cameraController()->setRotateCenter(center);
		}
		else
		{
			auto models = selectionms();
			if (models.isEmpty())
				return;
			QVector3D allCenter;
			for (auto m : models)
			{
				allCenter += m->boxWithSup().center();
			}
			allCenter /= models.count();
			cameraController()->setRotateCenter(allCenter);
		}

	}

	void VisSceneHandler::activeCameraCenterChange()
	{
		m_updateCameraCenterTimer.start(100);
	}
	
	void VisSceneHandler::onPhaseChanged()
	{

	}

	void VisSceneHandler::onThemeChanged(ThemeCategory category)
	{
		if (category == ThemeCategory::tc_dark)
		{
			m_rotateCenterEntity->setColor(QVector4D(1.0, 0.5, 0.0, 1.0));
			m_selectPixmap = QPixmap(":/UI/photo/select_dark.png"); 
			m_movePixmap = QPixmap(":/UI/photo/move_dark.png"); 
			m_rotatePixmap = QPixmap(":/UI/photo/rotate_dark.png"); 
		}
		else if (category == ThemeCategory::tc_light)
		{
			m_rotateCenterEntity->setColor(QVector4D(0.0, 0.5, 1.0, 1.0));
			m_selectPixmap = QPixmap(":/UI/photo/select_light.png");
			m_movePixmap = QPixmap(":/UI/photo/move_light.png"); 
			m_rotatePixmap = QPixmap(":/UI/photo/rotate_light.png"); 
		}
	}

	void VisSceneHandler::onLanguageChanged(MultiLanguage language)
	{

	}

	// void VisSceneHandler::onSelectionsChanged()
	// {
	// 	activeCameraCenterChange();
	// }

	// void VisSceneHandler::selectChanged(qtuser_3d::Pickable* pickable)
	// {

	// }

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
		if (event->key() == Qt::Key_Delete&&!wipeTowerSelected())//�û��ڼ����ϰ���Delete��ʱ
		{
			qDebug() << "VisSceneKeyHandler::onKeyPress: delete";
			removeSelectionModels(true);
			qDebug() << "VisSceneKeyHandler::onKeyPress: delete end";
		}

		if (event->modifiers() == Qt::ControlModifier && (event->key() == Qt::Key_Delete))//Ctrl+Delete ɾ������
		{
			qDebug() << "VisSceneKeyHandler::onKeyPress: ctrl + delete";
			removeAllModel(true);
		}


		if (event->modifiers() == Qt::ControlModifier && (event->key() == Qt::Key_V))
		{
			qDebug() << "VisSceneKeyHandler::onKeyPress: ctrl + V";
		}
		if (event->modifiers() == Qt::AltModifier && (event->key() == Qt::Key_S))
		{
			qDebug() << "VisSceneKeyHandler::onKeyPress: Alt + S";
		}

		if (event->modifiers() == Qt::ControlModifier&&!wipeTowerSelected())
		{	/* 切换耗材 */
		    int extruderCount = currentMachineExtruderCount(); 
			int value = event->key() - Qt::Key_1;	
			if (value >= 0 && value < extruderCount) 
			{
				getKernel()->changeMaterial(value);
			}
		}

		if(getKernel()->isDefaultVisScene()){
			
			if (event->modifiers() == Qt::ControlModifier && (event->key() == Qt::Key_A))
			{
				qDebug() << "VisSceneKeyHandler::onKeyPress: ctrl + A";
				selectAll();
			}
			if(!wipeTowerSelected()){
				if (!(event->modifiers() & Qt::ControlModifier|event->modifiers() & Qt::ShiftModifier) && event->key() == Qt::Key_X)
				{
					rotateXModels(1);
				}
				if (event->modifiers() == Qt::ShiftModifier && (event->key() == Qt::Key_X))
				{
					rotateXModels(1,true);
				}

				if (!(event->modifiers() & Qt::ControlModifier|event->modifiers() & Qt::ShiftModifier) && event->key() == Qt::Key_Y)
				{
					rotateYModels(1);
				}
				if (event->modifiers() == Qt::ShiftModifier && (event->key() == Qt::Key_Y))
				{
					rotateYModels(1,true);
				}
				if ( event->key() == Qt::Key_Q)
				{
					rotateZModels(1);
				}
				if (event->key() == Qt::Key_E)
				{
					rotateZModels(1,true);
				}
			}	
		}
		if(!(event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_A )
		{
			creative_kernel::setVisOperationMode(NULL);
			moveXModels(1.0,true);
		}
		if(!(event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_D)
		{
			creative_kernel::setVisOperationMode(NULL);
			moveXModels(1.0);
		}
		if(!(event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_W)
		{
			creative_kernel::setVisOperationMode(NULL);
			moveYModels(1.0);
		}
		if(!(event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_S)
		{
			creative_kernel::setVisOperationMode(NULL);
			moveYModels(1.0,true);
		}
		if(event->modifiers() == Qt::ControlModifier && (event->key() == Qt::Key_M))
		{
			creative_kernel::setVisOperationMode(NULL);
			mergePosition(true);
		}
			

	}

	void VisSceneHandler::onKeyRelease(QKeyEvent* event)
	{

	}

	void VisSceneHandler::onLeftMouseButtonPress(QMouseEvent* event)
	{
		qtuser_3d::Pickable* pickable = checkPickable(event->pos(), nullptr);
		if (pickable == NULL || !pickable->enableSelect())
		{
			if (shouldMultipleSelect())
			{
				QPoint p = event->pos();
				m_posOfLeftMousePress = p;
				qtuser_3d::Pickable* pickable = checkPickable(p, nullptr);
				m_didSelectModelAtPress = (pickable != nullptr);
				if (pickable)
				{
					qtuser_3d::ManipulatePickable* maniPick = qobject_cast<qtuser_3d::ManipulatePickable*>(pickable);
					if (!maniPick)
					{
						m_didSelectModelAtPress = false;
					}
				}
			} 
			requestVisUpdate();
		}
		else 
		{
			m_didSelectModelAtPress = true;
			if (!selectionms().contains((ModelN*)pickable))
			{	/* 单选/多选 */
				selectVis(event->pos(), event->modifiers() == Qt::KeyboardModifier::ControlModifier);
			}
			else 
			{	
				if (event->modifiers() == Qt::KeyboardModifier::ControlModifier) 
				{	/* 取消选中 */
					unselectOne(pickable);
				}
			}
		}
	}

	void VisSceneHandler::onLeftMouseButtonRelease(QMouseEvent* event)
	{
		if (shouldMultipleSelect() && m_didSelectModelAtPress == false)
		{
			getKernelUI()->setScene3DCursor(QCursor(Qt::ArrowCursor));
			dismissRectangleSelector();

			QRect rect = QRect(m_posOfLeftMousePress, event->pos());
			if (rect.size().isNull()) return;
			int width = abs(rect.width()), height = abs(rect.height());
			if (width <= 1 || height <= 1) return;

			selectArea(rect);
		}
		m_didSelectModelAtPress = true;

		// activeCameraCenterChange();
	}

	void VisSceneHandler::onLeftMouseButtonMove(QMouseEvent* event)
	{
		if (shouldMultipleSelect() && m_didSelectModelAtPress == false)
		{
			if (getKernelUI()->getScene3DCursor().shape() == Qt::ArrowCursor)
			getKernelUI()->setScene3DCursor(QCursor(m_selectPixmap));

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
		getKernelUI()->setScene3DCursor(QCursor(Qt::ArrowCursor));

		/* cancel select by click empty space */
		qtuser_3d::Pickable* pickable = checkPickable(event->pos(), nullptr);
		if (pickable == NULL || !modelns().contains((ModelN*)pickable))
			selectVis(event->pos(), event->modifiers() == Qt::KeyboardModifier::ControlModifier);
	}

	void VisSceneHandler::onRightMouseButtonPress(QMouseEvent* event) 
	{
		updateCameraCenter();

		qtuser_3d::Pickable* pickable = checkPickable(event->pos(), nullptr);
		if (pickable != NULL && pickable->enableSelect())
		{
			if (!selectionms().contains((ModelN*)pickable))
			{
				selectVis(event->pos(), false);
			}

			int count = selectionms().count();
			if (count == 1)
				getKernel()->setRightClickType(Kernel::SingleModelMenu);
			else 
				getKernel()->setRightClickType(Kernel::MultiModelsMenu);
		}
		else 
		{
			getKernel()->setRightClickType(Kernel::NoModelMenu);
		}
	}

	void VisSceneHandler::onRightMouseButtonRelease(QMouseEvent* event)
	{

	}

	void VisSceneHandler::onRightMouseButtonMove(QMouseEvent* event)
	{
		if (m_rotating)
		{
			if (getKernelUI()->getScene3DCursor().shape() == Qt::ArrowCursor)
				getKernelUI()->setScene3DCursor(QCursor(m_rotatePixmap));
		}
	}

	void VisSceneHandler::onRightMouseButtonClick(QMouseEvent* event)
	{

	}

}