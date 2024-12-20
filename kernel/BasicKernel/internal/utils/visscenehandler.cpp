#include "visscenehandler.h"

#include "interface/selectorinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/machineinterface.h"
#include "interface/camerainterface.h"
#include "interface/spaceinterface.h"
#include "qtuser3d/camera/cameracontroller.h"
#include "qtuser3d/camera/screencamera.h"
#include "interface/commandinterface.h"
#include "interface/printerinterface.h"
#include "internal/multi_printer/printermanager.h"
#include "internal/rclick_menu/rclickmenulist.h"
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
#include "external/kernel/modelnselector.h"
#include "internal/render/platepickable.h"

#include "../menu/saveascommand.h"
#include "internal/project_cx3d/cx3dimportercommand.h"

#include "internal/tool/translatemode.h"
#include "external/operation/translateop.h"
#include "external/operation/rotateop.h"

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
			// addModelNSelectorTracer(this);
			addKeyEventHandler(this);
			addHoverEventHandler(this);
		}
		else 
		{
			//disconnect(getPrinterManager(), &PrinterMangager::signalPrintersCountChanged, this, &VisSceneHandler::activeCameraCenterChange);

			// removeModelNSelectorTracer(this);
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
			if (models.isEmpty())
				return;
			
			qtuser_3d::Box3D box;
			for (auto m : models)
			{
				box += m->globalSpaceBox();
			}
			cameraController()->setRotateCenter(box.center());
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
		cancelRectangleSelect();
	}

	void VisSceneHandler::onHoverMove(QHoverEvent* event)
	{
		if (pointHover(event))
			requestVisUpdate(false);
	}

	void VisSceneHandler::onKeyPress(QKeyEvent* event)
	{
		if (event->key() == Qt::Key_Delete&&!wipeTowerSelected()&&(getKernel()->currentPhase()==0))//�û��ڼ����ϰ���Delete��ʱ
		{
			qDebug() << "VisSceneKeyHandler::onKeyPress: delete";
			removeSelections();
			qDebug() << "VisSceneKeyHandler::onKeyPress: delete end";
		}

		if (event->modifiers() == Qt::ControlModifier && (event->key() == Qt::Key_Delete))//Ctrl+Delete ɾ������
		{
			qDebug() << "VisSceneKeyHandler::onKeyPress: ctrl + delete";
			clearSpace();
		}



		if (event->modifiers() == Qt::ControlModifier && (event->key() == Qt::Key_V))
		{
			qDebug() << "VisSceneKeyHandler::onKeyPress: ctrl + V";
		}
		if (event->modifiers() == Qt::AltModifier && (event->key() == Qt::Key_S))
		{
			qDebug() << "VisSceneKeyHandler::onKeyPress: Alt + S";
		}

		if (event->modifiers() == Qt::ControlModifier && (event->key() == Qt::Key_I))
		{
			getKernel()->openFile();
			
		}

		if (event->modifiers() == Qt::ControlModifier && (event->key() == Qt::Key_O))
		{
			CX3DImporterCommand* projectFile = new CX3DImporterCommand();
			projectFile->execute();
			delete projectFile;
			projectFile = nullptr;
		}

		if (event->key() == Qt::Key_F1)
		{
			TranslateMode* translate = new TranslateMode();
			translate->center();
			delete translate;
			translate = nullptr;
		}
		if (event->key() == Qt::Key_F2)
		{
			TranslateMode* translate = new TranslateMode();
			translate->bottom();
			delete translate;
			translate = nullptr;
		}

		if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) && event->key() == Qt::Key_S)
		{
			SaveAsCommand* modelfile = new SaveAsCommand();
			modelfile->execute();
			delete modelfile;
			modelfile = nullptr;
		}

		if (!wipeTowerSelected())
		{	/* 切换耗材 */
		    int extruderCount = currentMachineExtruderCount(); 
			int value = event->key() - Qt::Key_1;	
			if (value >= 0 && value < extruderCount) 
			{
				getKernel()->changeMaterial(value);
			}
		}

		if(getKernel()->isDefaultVisScene() && getKernel()->currentPhase()==0){
			
	

			if (event->modifiers() == Qt::ControlModifier && (event->key() == Qt::Key_A))
			{
				qDebug() << "VisSceneKeyHandler::onKeyPress: ctrl + A";
				ctrlASelectAll();
			}
			if(!wipeTowerSelected()){
				if (!(event->modifiers() & Qt::ControlModifier|event->modifiers() & Qt::ShiftModifier) && event->key() == Qt::Key_X)
				{
					rotateXModels(1.0);
					notifyRotationChanged();
				}
				if (event->modifiers() == Qt::ShiftModifier && (event->key() == Qt::Key_X))
				{
					rotateXModels(- 1.0);
					notifyRotationChanged();
				}

				if (!(event->modifiers() & Qt::ControlModifier|event->modifiers() & Qt::ShiftModifier) && event->key() == Qt::Key_Y)
				{
					rotateYModels(1.0);
					notifyRotationChanged();
				}
				if (event->modifiers() == Qt::ShiftModifier && (event->key() == Qt::Key_Y))
				{
					rotateYModels(- 1.0);
					notifyRotationChanged();
				}
				if ( event->key() == Qt::Key_Q)
				{
					rotateZModels(1.0);
					notifyRotationChanged();
				}
				if (event->key() == Qt::Key_E)
				{
					rotateZModels(- 1.0);
					notifyRotationChanged();
				}
			}
			if(!(event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_A )
			{
				moveXModels(-1.0);
				notifyPositionChanged();
			}
			if(!(event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_D)
			{
				moveXModels(1.0);
				notifyPositionChanged();
			}
			if(!(event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_W)
			{
				moveYModels(1.0);
				notifyPositionChanged();
			}
			if(!(event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_S)
			{
				moveYModels(- 1.0);
				notifyPositionChanged();
			}
			if(event->modifiers() == Qt::ControlModifier && (event->key() == Qt::Key_M))
			{
				creative_kernel::setVisOperationMode(NULL);
				mergePosition();
			}	
		}

		
		
		cancelRectangleSelect();
	}

	void VisSceneHandler::onKeyRelease(QKeyEvent* event)
	{

	}

	void VisSceneHandler::onLeftMouseButtonPress(QMouseEvent* event)
	{
		m_selectedPickable = checkPickable(event->pos(), nullptr);
		if (m_selectedPickable == NULL || 
			!m_selectedPickable->enableSelect() || 
			(qobject_cast<creative_kernel::PlatePickable*>(m_selectedPickable) != nullptr))
		{
			if (shouldMultipleSelect())
			{
				QPoint p = event->pos();
				m_posOfLeftMousePress = p;
				m_didSelectModelAtPress = (m_selectedPickable != nullptr);
				if (m_selectedPickable)
				{
					qtuser_3d::ManipulatePickable* maniPick = qobject_cast<qtuser_3d::ManipulatePickable*>(m_selectedPickable);
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
			pointSelect(event);
		}
	}

	void VisSceneHandler::onLeftMouseButtonRelease(QMouseEvent* event)
	{
		if (shouldMultipleSelect() && m_didSelectModelAtPress == false)
		{
			getKernelUI()->setScene3DCursor(QCursor(Qt::ArrowCursor));
			dismissRectangleSelector();

			if (m_posOfLeftMousePress.x() < 0 || m_posOfLeftMousePress.y() < 0) return;
				
			QRect rect = QRect(m_posOfLeftMousePress, event->pos());
			if (rect.size().isNull()) return;
			int width = abs(rect.width()), height = abs(rect.height());
			if (width <= 1 || height <= 1) return;

			areaSelect(rect, event);
		}
		m_didSelectModelAtPress = true;

		// activeCameraCenterChange();
	}

	void VisSceneHandler::onLeftMouseButtonMove(QMouseEvent* event)
	{
		//渲染框选
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
		// qtuser_3d::Pickable* pickable = checkPickable(event->pos(), nullptr);
		// if (pickable != m_selectedPickable)
		// {

		// }

		ModelN* model = qobject_cast<ModelN*>(m_selectedPickable);
		if ((m_selectedPickable == nullptr) || (model == nullptr))
			pointSelect(event);

	}

	void VisSceneHandler::onRightMouseButtonPress(QMouseEvent* event) 
	{
		updateCameraCenter();

		ModelN* model = checkPickModel(event->pos());
		if (model)
		{
			auto models = selectionms();
			if (models.isEmpty())
				selectGroup(model->getModelGroup(), false);

			models = selectedParts();
			if (!models.isEmpty())
			{
				if (models.size() == 1)
				{
					ModelN* sModel = models.first();
					ModelNType type = sModel->modelNType();

					if (type == ModelNType::normal_part)
					{
						if (!sModel->isRelief())
							getKernel()->setRightClickType(Kernel::NormalPartMenu);
						else
							getKernel()->setRightClickType(Kernel::NormalReliefMenu);
					}
					else if (type == ModelNType::modifier)
					{
						if (!sModel->isRelief())
							getKernel()->setRightClickType(Kernel::ModifierPartMenu);
						else
							getKernel()->setRightClickType(Kernel::ModifierReliefMenu);
					}
					else 
					{
						if (!sModel->isRelief())
							getKernel()->setRightClickType(Kernel::SpecialPartMenu);
						else
							getKernel()->setRightClickType(Kernel::SpecialReliefMenu);
					}
				} 
				else 
				{
					/* 取交集菜单，normal_part的菜单功能最多，其次modifier，最后到其它特殊零件 */
					int rootLevel = 3;
					bool allRelief = true;
					for (ModelN* m : models)
					{
						if (m->isRelief())
							allRelief = false;

						ModelNType type = m->modelNType();
						if (type == ModelNType::normal_part)
						{
							rootLevel = rootLevel < 3 ? rootLevel : 3;
						}
						else if (type == ModelNType::modifier) 
						{
							rootLevel = rootLevel < 2 ? rootLevel : 2;
						}
						else 
						{
							rootLevel = rootLevel < 1 ? rootLevel : 1;
						}
					}

					if (!allRelief)
					{
						if (rootLevel == 1)
						{
							getKernel()->setRightClickType(Kernel::SpecialPartMenu);
						}
						else if (rootLevel == 2)
						{
							getKernel()->setRightClickType(Kernel::ModifierPartMenu);
						} 
						else if (rootLevel == 3)
						{
							getKernel()->setRightClickType(Kernel::NormalPartMenu);
						}
					}
					else 
					{
						if (rootLevel == 1)
						{
							getKernel()->setRightClickType(Kernel::SpecialReliefMenu);
						}
						else if (rootLevel == 2)
						{
							getKernel()->setRightClickType(Kernel::ModifierReliefMenu);
						} 
						else if (rootLevel == 3)
						{
							getKernel()->setRightClickType(Kernel::NormalReliefMenu);
						}
					}
				}
			}
			else
			{
				QList<ModelGroup*> groups = selectedGroups();
				int count = groups.count();
				if (count == 1)
				{
					QList<ModelN*> models = groups.first()->models();
					if (models.count() == 1 && models.first()->isRelief())
					{
						getKernel()->setRightClickType(Kernel::SingleReliefMenu);
					}
					else 
					{
						getKernel()->setRightClickType(Kernel::SingleModelMenu);
					}
				}
				else
					getKernel()->setRightClickType(Kernel::MultiModelsMenu);
			}
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

	void VisSceneHandler::cancelRectangleSelect()
	{
		if (shouldMultipleSelect() && m_didSelectModelAtPress == false)
		{
			m_posOfLeftMousePress = QPoint(-1, -1);
			dismissRectangleSelector();
		}
	}

	void VisSceneHandler::notifyPositionChanged()
	{
		qtuser_3d::SceneOperateMode* mode = creative_kernel::visOperationMode();
		creative_kernel::TranslateOp* transop = qobject_cast<creative_kernel::TranslateOp*>(mode);
		if (transop)
		{
			transop->notifyPositionChanged();
		}
	}

	void VisSceneHandler::notifyRotationChanged()
	{
		qtuser_3d::SceneOperateMode* mode = creative_kernel::visOperationMode();
		creative_kernel::RotateOp* rotop = qobject_cast<creative_kernel::RotateOp*>(mode);
		if (rotop)
		{
			rotop->rotateChanged();
		}
	}
}
