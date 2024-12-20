#include "reliefoperatemode.h"
#include <QtCore/QSharedPointer>

#include <cxkernel/interface/jobsinterface.h>
//#include "cxkernel/data/meshdata.h"

#include "data/shareddatamanager/renderdata.h"
#include "data/modeln.h"

#include "interface/eventinterface.h"
#include "interface/selectorinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/camerainterface.h"
#include "interface/renderinterface.h"
#include "interface/printerinterface.h"

#include "reliefjob.h"

#include "interface/reuseableinterface.h"
#include "interface/shareddatainterface.h"
#include "interface/spaceinterface.h"
#include "interface/printerinterface.h"
#include "interface/appsettinginterface.h"

#include "qtuser3d/camera/cameracontroller.h"
#include "qtuser3d/camera/screencamera.h"
#include "qtuser3d/trimesh2/conv.h"
#include "qtuser3d/refactor/xeffect.h"
#include "qtuser3d/trimesh2/conv.h"

#include <QElapsedTimer>
#include "entity/alonepointentity.h"
#include "entity/rotate3dhelperentity.h"

#include "kernel/kernelui.h"
#include "kernel/kernel.h"
#include "data/modelspaceundo.h"

#include "reliefrotateundocommand.h"
#include "reliefmoveundocommand.h"
#include "renderpass/zprojectrenderpass.h"

#include "kernel/reuseablecache.h"

#include <QtCore/qmath.h>
#include <QQmlProperty>
#include "qtusercore/property/qmlpropertysetter.h"

using namespace creative_kernel;

ReliefOperateMode::ReliefOperateMode(QObject* parent)
    : MoveOperateMode{ parent }
{
 	m_type = qtuser_3d::SceneOperateMode::OperateModeType::DefaultMode;

	// m_pointEntity = new qtuser_3d::AlonePointEntity();
	// m_pointEntity->setPointSize(12);
	// m_pointEntity->setFilterType("overlay");

	m_reliefEffect = new qtuser_3d::XEffect(getKernel()->reuseableCache());
	{
		qtuser_3d::XRenderPass* viewPass = new qtuser_3d::XRenderPass("modelnormal", m_reliefEffect);
		viewPass->addFilterKeyMask("view", 0);
		viewPass->setPassCullFace();
		viewPass->setPassDepthTest();
		viewPass->setParameter("transparency", 1.0);
		m_reliefEffect->addRenderPass(viewPass);
	}		

	m_reliefPartEffect = new qtuser_3d::XEffect(getKernel()->reuseableCache());
	{
		qtuser_3d::XRenderPass* viewPass = new qtuser_3d::XRenderPass("modeltransparent", m_reliefPartEffect);
		viewPass->addFilterKeyMask("alpha", 0);
		viewPass->setPassBlend(Qt3DRender::QBlendEquationArguments::SourceAlpha, Qt3DRender::QBlendEquationArguments::OneMinusSourceAlpha);
		viewPass->setPassCullFace(Qt3DRender::QCullFace::Back);
		viewPass->setPassNoDepthMask();
		m_reliefPartEffect->addRenderPass(viewPass);
	}		
	
	QVariantList values = CONFIG_GLOBAL_VARLIST(modeleffect_statecolors, model_group);
	m_reliefEffect->setParameter("stateGain[0]", values);
	m_reliefEffect->setParameter("outPlatformGain", QVector4D(1.0, 0.50, 0.50, 1.0));
	m_reliefEffect->setParameter("insideGain", QVector4D(1.0, 1.0, 0.50, 1.0));
	m_reliefEffect->setParameter("insideGain", QVector4D(1.0, 1.0, 0.50, 1.0));
	m_reliefPartEffect->setParameter("stateGain[0]", values);
	m_reliefPartEffect->setParameter("outPlatformGain", QVector4D(1.0, 0.50, 0.50, 1.0));
	m_reliefPartEffect->setParameter("insideGain", QVector4D(1.0, 1.0, 0.50, 1.0));
	m_reliefPartEffect->setParameter("insideGain", QVector4D(1.0, 1.0, 0.50, 1.0));

	QVariantList weights;
	for (size_t i = 0; i < 16; i++)
	{
		weights << QVector4D(1.0, 1.0, 1.0, 1.0);
	}
	m_reliefEffect->setParameter("nozzleGain[0]", weights);
	m_reliefPartEffect->setParameter("nozzleGain[0]", weights);

	m_rotateEntity = new qtuser_3d::Rotate3DHelperEntity(NULL, qtuser_3d::RotateHelperType::RH_CUSTOM);
	m_rotateEntity->setCameraController(cameraController());
	m_rotateEntity->setScreenCamera(visCamera());
	m_rotateEntity->setPickSource(visPickSource());
	m_rotateEntity->setXVisibility(false);
	m_rotateEntity->setYVisibility(false);
	m_rotateEntity->setZVisibility(false);
	m_rotateEntity->setRotateCallback(this);

	m_delayTimer.setSingleShot(true);


	auto* ui_parent = getKernelUI()->glMainView();

	tip_component_ = new QQmlComponent{ qmlEngine(ui_parent),
		QUrl{ QStringLiteral("qrc:/CrealityUI/qml/RotateTip.qml") }, this };

	tip_object_ = tip_component_->create(qmlContext(ui_parent));
	if (tip_object_ != nullptr) {
		tip_object_->setParent(ui_parent);
		qtuser_qml::writeObjectProperty(tip_object_, QStringLiteral("parent"), ui_parent);
	}

}

ReliefOperateMode::~ReliefOperateMode()
{

}

ReliefOperateMode::ReliefType ReliefOperateMode::getType() const
{
  	return m_reliefType;
}

void ReliefOperateMode::setType(ReliefType type)
{
	m_reliefType = type;
	qDebug() << "setType:" << (int)type;
    emit typeChanged();
}

ReliefOperateMode::Shape ReliefOperateMode::getShape() const
{
  	return m_shape;
}

void ReliefOperateMode::setShape(Shape shape)
{
	m_shape = shape;
	qDebug() << "setShape:" << (int)shape;
    emit shapeChanged();
}

QString ReliefOperateMode::getText() const
{
	if (m_fontConfig)
	{
		QString text(m_fontConfig->text);
		return text;
	}
	else
	{
		QString text(m_defaultFontConfig.text);
		return text;
	}
}

void ReliefOperateMode::setText(const QString& text)
{
	if (text.isEmpty())
		return;

	if (!m_applyingConfig)
	{
		ReliefUndoConfig oldUndoConfig = generateUndoConfig();
		ReliefUndoConfig newUndoConfig = oldUndoConfig;
		newUndoConfig.config.text = text;
		oldUndoConfig.changedType = ReliefUndoConfigType::UndoText;
		newUndoConfig.changedType = ReliefUndoConfigType::UndoText;

		recordStep(oldUndoConfig, newUndoConfig);
	}
	else 
	{
		if (m_fontConfig)
			m_fontConfig->text = text;
			
		// emit textChanged();
    	emit fontConfigChanged();
	}

}

QString ReliefOperateMode::getFont() const  
{
	if (m_fontConfig)
	{
		QString font(m_fontConfig->font);
		return font;
	}
	else
	{
		QString font(m_defaultFontConfig.font);
		return font;
	}
}

void ReliefOperateMode::setFont(const QString& font)
{
	if (!m_applyingConfig)
	{
		ReliefUndoConfig oldUndoConfig = generateUndoConfig();
		ReliefUndoConfig newUndoConfig = oldUndoConfig;
		newUndoConfig.config.font = font;
		oldUndoConfig.changedType = ReliefUndoConfigType::UndoFont;
		newUndoConfig.changedType = ReliefUndoConfigType::UndoFont;

		recordStep(oldUndoConfig, newUndoConfig);
	}
	else 
	{
		if (m_fontConfig)
			m_fontConfig->font = font;

		// emit fontChanged();
    	emit fontConfigChanged();
	}

}

int ReliefOperateMode::fontSize() const
{
	if (m_fontConfig)
		return m_fontConfig->fontSize;
	else
		return m_defaultFontConfig.fontSize;
}

void ReliefOperateMode::setFontSize(int size)
{
	 if (!m_applyingConfig)
	 {
	 	ReliefUndoConfig oldUndoConfig = generateUndoConfig();
	 	ReliefUndoConfig newUndoConfig = oldUndoConfig;
	 	newUndoConfig.config.fontSize = size;
		oldUndoConfig.changedType = ReliefUndoConfigType::UndoFontSize;
		newUndoConfig.changedType = ReliefUndoConfigType::UndoFontSize;

	 	recordStep(oldUndoConfig, newUndoConfig);
	 }
	 else 
	 {
		if (m_fontConfig)
			m_fontConfig->fontSize = size;

    	// emit fontSizeChanged();
    	emit fontConfigChanged();
	 }

}

int ReliefOperateMode::wordSpace() const
{
	if (m_fontConfig)
		return m_fontConfig->wordSpace;
	else
		return m_defaultFontConfig.wordSpace;
}

void ReliefOperateMode::setWordSpace(int space)
{
	if (!m_applyingConfig)
	{
		ReliefUndoConfig oldUndoConfig = generateUndoConfig();
		ReliefUndoConfig newUndoConfig = oldUndoConfig;
		newUndoConfig.config.wordSpace = space;
		oldUndoConfig.changedType = ReliefUndoConfigType::UndoWordSpace;
		newUndoConfig.changedType = ReliefUndoConfigType::UndoWordSpace;

		recordStep(oldUndoConfig, newUndoConfig);
	}
	else
	{
		if (m_fontConfig)
			m_fontConfig->wordSpace = space;

		// emit wordSpaceChanged();
    	emit fontConfigChanged();
	}

}

int ReliefOperateMode::lineSpace() const
{
	if (m_fontConfig)
		return m_fontConfig->lineSpace;
	else
		return m_defaultFontConfig.lineSpace;
}

void ReliefOperateMode::setLineSpace(int space)
{
	if (!m_applyingConfig)
	{
		ReliefUndoConfig oldUndoConfig = generateUndoConfig();
		ReliefUndoConfig newUndoConfig = oldUndoConfig;
		newUndoConfig.config.lineSpace = space;
		oldUndoConfig.changedType = ReliefUndoConfigType::UndoLineSpace;
		newUndoConfig.changedType = ReliefUndoConfigType::UndoLineSpace;

		recordStep(oldUndoConfig, newUndoConfig);
	}
	else
	{
		if (m_fontConfig)
			m_fontConfig->lineSpace = space;
		// emit lineSpaceChanged();
    	emit fontConfigChanged();
	}
}

float ReliefOperateMode::height() const
{
	if (m_fontConfig)
	{
		return m_fontConfig->height;
	}
	else
		return m_defaultFontConfig.height;
}

void ReliefOperateMode::setHeight(float height)
{
	if (!m_applyingConfig)
	{
		ReliefUndoConfig oldUndoConfig = generateUndoConfig();
		ReliefUndoConfig newUndoConfig = oldUndoConfig;
		newUndoConfig.config.height = height;
		oldUndoConfig.changedType = ReliefUndoConfigType::UndoHeight;
		newUndoConfig.changedType = ReliefUndoConfigType::UndoHeight;

		recordStep(oldUndoConfig, newUndoConfig);
	}
	else
	{
		if (m_fontConfig)
		{
    		emit fontConfigChanged();
		}
	}
}

float ReliefOperateMode::distance() const
{
	if (m_fontConfig)
		return m_fontConfig->distance;
	else
		return m_defaultFontConfig.distance;
}

void ReliefOperateMode::setDistance(float distance)
{
	if (!m_applyingConfig)
	{
		ReliefUndoConfig oldUndoConfig = generateUndoConfig();
		ReliefUndoConfig newUndoConfig = oldUndoConfig;
		newUndoConfig.config.distance = distance;
		oldUndoConfig.changedType = ReliefUndoConfigType::UndoDistance;
		newUndoConfig.changedType = ReliefUndoConfigType::UndoDistance;

		recordStep(oldUndoConfig, newUndoConfig);
	}
	else
	{
		if (m_fontConfig)
		{
    		emit fontConfigChanged();
		}
	}
}

int ReliefOperateMode::embossType() const
{
	if (m_fontConfig)
		return m_fontConfig->embossType;
	else
		return m_defaultFontConfig.embossType;
}

void ReliefOperateMode::setEmbossType(int type)
{
	if (m_wrapper)
	{
		beginEmboss();
		m_wrapper->setEmbossType(type >= 2);
    	endEmboss();
	}

	emit embossTypeChanged();
}

int ReliefOperateMode::reliefModelType() 
{
	if (!holdWrapper())
		return -1;

	ModelN* reilef = m_wrapper->model();
	return (int)reilef->modelNType();
}

void ReliefOperateMode::setReliefModelType(int type)
{
	if (!holdWrapper())
		return;

	ModelN* reilef = m_wrapper->model();
	QList<ModelNPropertyMeshUndo> changes;
	int64_t model_id;

	QString start_name;
	QString end_name;
	SharedDataID start_data_id;
	SharedDataID end_data_id;

	ModelNPropertyMeshUndo undo;
	undo.model_id = reilef->getObjectId();
	undo.start_name = reilef->name();
	undo.end_name = reilef->name();

	SharedDataID id = reilef->sharedDataID();
	undo.start_data_id = id;

	id(ModelType) = type;
	undo.end_data_id = id;

	changes << undo;
	replaceModelsProperty(changes, true);
	
	// notifySpaceChange();
	emit reliefModelTypeChanged();
}

bool ReliefOperateMode::bold() const
{
	if (m_fontConfig)
		return m_fontConfig->bold;
	else
		return m_defaultFontConfig.bold;
}

void ReliefOperateMode::setBold(bool bold)
{
	if (!m_applyingConfig)
	{
		ReliefUndoConfig oldUndoConfig = generateUndoConfig();
		ReliefUndoConfig newUndoConfig = oldUndoConfig;
		newUndoConfig.config.bold = bold;
		oldUndoConfig.changedType = ReliefUndoConfigType::UndoBold;
		newUndoConfig.changedType = ReliefUndoConfigType::UndoBold;

		recordStep(oldUndoConfig, newUndoConfig);
	}
	else
	{
		if (m_fontConfig)
			m_fontConfig->bold = bold;

    	emit fontConfigChanged();
	}
}

bool ReliefOperateMode::italic() const
{
	if (m_fontConfig)
		return m_fontConfig->italic;
	else
		return m_defaultFontConfig.italic;
}

void ReliefOperateMode::setItalic(bool italic)
{
	if (!m_applyingConfig)
	{
		ReliefUndoConfig oldUndoConfig = generateUndoConfig();
		ReliefUndoConfig newUndoConfig = oldUndoConfig;
		newUndoConfig.config.italic = italic;
		oldUndoConfig.changedType = ReliefUndoConfigType::UndoItalic;
		newUndoConfig.changedType = ReliefUndoConfigType::UndoItalic;

		recordStep(oldUndoConfig, newUndoConfig);
	}
	else
	{
		if (m_fontConfig)
			m_fontConfig->italic = italic;

    	emit fontConfigChanged();
	}
}


QPoint ReliefOperateMode::pos() const
{
	return m_pos;
}

bool ReliefOperateMode::needIndicate() const
{
	return m_needIndicate;
}

bool ReliefOperateMode::hoverOnModel() const
{
	return m_hoverOnModel;
}

bool ReliefOperateMode::canEmboss() const
{
	return m_canEmboss;
}

bool ReliefOperateMode::distanceEnabled() const
{
	if (!m_fontConfig)
		return false;

	return m_fontConfig->reliefTargetID != -1;
}

void ReliefOperateMode::applyFontConfigToRelief(creative_kernel::ModelN* relief, ReliefUndoConfig config)
{
	joinReliefOperate(relief);

	m_applyingConfig = true;

	FontConfig fontConfig = config.config;

	switch (config.changedType)
	{
		case ReliefUndoConfigType::UndoText:
			setText(QString(fontConfig.text.data()));
			m_wrapper->updateOutline();
		break;
		case ReliefUndoConfigType::UndoFont:
			setFont(QString(fontConfig.font.data()));
			m_wrapper->updateOutline();
			break;
		case ReliefUndoConfigType::UndoFontSize:
			setFontSize(fontConfig.fontSize);
			m_wrapper->updateOutline();
			break;
		case ReliefUndoConfigType::UndoWordSpace:
			setWordSpace(fontConfig.wordSpace);
			m_wrapper->updateOutline();
			break;
		case ReliefUndoConfigType::UndoLineSpace:
			setLineSpace(fontConfig.lineSpace);
			m_wrapper->updateOutline();
			break;
		case ReliefUndoConfigType::UndoHeight:
			m_wrapper->setHeight(fontConfig.height);
			setHeight(fontConfig.height);
			break;
		case ReliefUndoConfigType::UndoDistance:
			m_wrapper->setDistance(fontConfig.distance);
			setDistance(fontConfig.distance);
			break;
		case ReliefUndoConfigType::UndoEmbossType:
			setEmbossType(fontConfig.embossType);
			break;
		case ReliefUndoConfigType::UndoBold:
			setBold(fontConfig.bold);
			m_wrapper->updateOutline();
			break;
		case ReliefUndoConfigType::UndoItalic:
			setItalic(fontConfig.italic);
			m_wrapper->updateOutline();
			break;
	}
	m_applyingConfig = false;

	m_wrapper->updateModel();
	showRotateEntity();
	creative_kernel::setNeedCheckScope(0);
}		

void ReliefOperateMode::rotateRelief(creative_kernel::ModelN* relief, float angle)
{
	joinReliefOperate(relief);
	rotate(angle);
	showRotateEntity();
}

void ReliefOperateMode::moveRelief(creative_kernel::ModelN* relief, creative_kernel::ModelN* target, int faceId, QVector3D location, int embossType)
{
	joinReliefOperate(relief);

	if (m_wrapper->config()->embossType != embossType)
	{
		m_wrapper->config()->embossType = embossType;
		emit fontConfigChanged();
	}
	m_wrapper->setEmbossTarget(target);
	m_wrapper->emboss(faceId, trimesh::vec3(location[0], location[1], location[2]));
	m_wrapper->updateModel(true);
	//emboss(target, moveConfig.faceId, moveConfig.cross, moveConfig.normal, false, false);
	showRotateEntity();
}

void ReliefOperateMode::initRelief(creative_kernel::ModelN* relief)
{
	joinReliefOperate(relief);
	
	if (m_wrapper->config()->embossType != 0)
	{
		m_wrapper->config()->embossType = 0;
		emit fontConfigChanged();
	}
	
	m_wrapper->initEmboss();
	m_wrapper->updateInitModel();

	showRotateEntity();
}

void ReliefOperateMode::onModelTypeChanged(ModelN* model)
{
	if (!holdWrapper())
		return;

	if (model == m_wrapper->model())
		emit fontConfigChanged();
}
    
void ReliefOperateMode::joinReliefOperate(ModelN* relief)
{
	QList<ModelN*> selections = selectionms();
	int count = selections.size();
	if (count == 1)
	{
		if (selections.first() != relief)
			selectModelN(relief);
	}
	else if (count == 0 || count > 1)
		selectModelN(relief);

	if (visOperationMode() != this)
	{
		getKernelUI()->switchMode(34);
	}
}

void ReliefOperateMode::onAttach()
{
	m_initing = true;

	requestRender(this);

	emit reliefModelTypeChanged();
	creative_kernel::addModelNSelectorTracer(this);
	creative_kernel::addHoverEventHandler(this);
	creative_kernel::prependLeftMouseEventHandler(this);
	traceSpace(this);

	m_rotateEntity->attach();

	QList<qtuser_3d::Pickable*> pickableList = m_rotateEntity->getPickables();
	for (qtuser_3d::Pickable* pickable : pickableList)
	{
		tracePickable(pickable);
	}

	QList<qtuser_3d::LeftMouseEventHandler*> handlers = m_rotateEntity->getLeftMouseEventHandlers();
	for (qtuser_3d::LeftMouseEventHandler* handler : handlers)
	{
		prependLeftMouseEventHandler(handler);
	}

	ModelGroup* group = getValidSelectedGroup();
	if (!m_wrapper)
	{
		FontMeshWrapper* wrapper = new FontMeshWrapper();
		wrapper->createModel(group);
		setFontMeshWrapper(wrapper);
		group = m_wrapper->group();
		m_isReliefObject = (group && group->models().count() == 1);
		m_canEmboss = (holdWrapper() && !m_isReliefObject);
		emit canEmbossChanged();
	}
	else
	{
		group = m_wrapper->group();
		m_isReliefObject = (group && group->models().count() == 1);
		m_canEmboss = (holdWrapper() && !m_isReliefObject);
		emit canEmbossChanged();
	}

	showRotateEntity();

	m_initing = false;
	requestVisUpdate(true);
}

void ReliefOperateMode::onDettach() 
{
	creative_kernel::removeHoverEventHandler(this);
	creative_kernel::removeLeftMouseEventHandler(this);
	creative_kernel::removeModelNSelectorTracer(this);
	creative_kernel::requestVisUpdate();
	unTraceSpace(this);

	m_rotateEntity->detach();
	
	QList<qtuser_3d::Pickable*> pickableList = m_rotateEntity->getPickables();
	for (qtuser_3d::Pickable* pickable : pickableList)
	{
		unTracePickable(pickable);
	}

	QList<qtuser_3d::LeftMouseEventHandler*> handlers = m_rotateEntity->getLeftMouseEventHandlers();
	for (qtuser_3d::LeftMouseEventHandler* handler : handlers)
	{
		removeLeftMouseEventHandler(handler);
	}

	hideRotateEntity();

	setFontMeshWrapper(NULL);
	endRequestRender(this);
	//setCommandRender();
}

bool ReliefOperateMode::shouldMultipleSelect() 
{
	return false;
}

void ReliefOperateMode::onHoverMove(QHoverEvent* event) 
{

	if (m_needIndicate)
	{
		m_needIndicate = false;
		emit needIndicateChanged();
	}

	if (m_rotatingRelief)
	{
		setTipObjectPos(event->pos());
	}
}

void ReliefOperateMode::onLeftMouseButtonMove(QMouseEvent* event)
{
	if (m_isReliefObject && !m_movingRelief && !m_rotatingRelief)
	{
		MoveOperateMode::onLeftMouseButtonMove(event);
		m_leftMoveStatus = false;
		return;
	}

	if (m_rotatingRelief)
	{
		setTipObjectPos(event->pos());
		return;
	}

	if (m_delayTimer.isActive())
		return;

	m_delayTimer.start(40);

	emboss(event->pos());
}

void ReliefOperateMode::onLeftMouseButtonClick(QMouseEvent* event) 
{
	if (m_movingRelief)
	{
		m_leftClickStatus = false;
	}
}

void ReliefOperateMode::onLeftMouseButtonPress(QMouseEvent* event)
{
	m_hoverOnModel = false;
	int faceID = -1;
	qtuser_3d::Pickable* pickable = nullptr;
	pickable = checkPickable(event->pos(), &faceID);

	ModelN* model = dynamic_cast<ModelN*>(pickable);
	bool isReliefPiackable = model && model->isRelief();

	if (isReliefPiackable)
	{
		ModelN* relief = m_wrapper->model();
		if (m_isReliefObject && model == relief)
		{
			hideRotateEntity();
			MoveOperateMode::onLeftMouseButtonPress(event);
			m_leftPressStatus = false;
			return;
		}

		if (model == relief)
		{
			beginEmboss();
			if (relief->modelNType() == ModelNType::normal_part)
				relief->useCustomEffect(m_reliefEffect);
			else 
				relief->useCustomEffect(m_reliefPartEffect);
			m_movingRelief = true;
			hideRotateEntity();
		}
		else 
		{
			selectModelN(model, false, true);
			m_leftClickStatus = false;
			relief = m_wrapper->model();
			if (m_isReliefObject)
			{	/* switch to relief obj */
				selectGroup(model->getModelGroup());
				relief = m_wrapper->model();

				hideRotateEntity();
				MoveOperateMode::onLeftMouseButtonPress(event);
				m_leftPressStatus = false;
				return;
			}
			else 
			{	/* switch to relief part */
				beginEmboss();
				if (relief->modelNType() == ModelNType::normal_part)
					relief->useCustomEffect(m_reliefEffect);
				else 
					relief->useCustomEffect(m_reliefPartEffect);
				m_movingRelief = true;
				hideRotateEntity();
			}
		}

		m_leftPressStatus = false;

	}
	else
	{
		m_movingRelief = false;
	
	}
	
}

void ReliefOperateMode::onLeftMouseButtonRelease(QMouseEvent* event) 
{
	if (m_isReliefObject)
	{
		showRotateEntity();
		MoveOperateMode::onLeftMouseButtonRelease(event);
		m_leftReleaseStatus = false;
		return;
	}

	if (m_movingRelief)
	{
		emboss(event->pos(), true);
		m_wrapper->syncTarget();
		m_movingRelief = false;
		m_wrapper->model()->useCustomEffect(NULL);
		emit distanceEnabledChanged();
		showRotateEntity();
    	endEmboss();
	}
}

void ReliefOperateMode::onStartRotate() 
{
	if (!holdWrapper())
		return;

	m_angle = m_wrapper->angle();
	m_rotatingRelief = true;
    setTipObjectVisible(true);

}

void ReliefOperateMode::onRotate(QQuaternion q) 
{
	if (m_delayTimer.isActive())
		return;

	m_delayTimer.start(50);
	rotate(q);
}

void ReliefOperateMode::onEndRotate(QQuaternion q) 
{
	rotate(q, true);
	m_rotatingRelief = false;
    setTipObjectVisible(false);

	// qDebug() << "**************** onEndRotate ****************";
	// for (QString str : m_angleLogs)
	// 	qDebug() << str;
	requestVisUpdate(true);
}

void ReliefOperateMode::setRotateAngle(QVector3D axis, float angle) 
{
}

void ReliefOperateMode::onSceneChanged(const trimesh::dbox3& box) 
{
	showRotateEntity();
}

void ReliefOperateMode::onSelectionsChanged()
{
	if (m_initing)
		return;

	setFontMeshWrapper(NULL);
	ModelGroup* group = NULL;

	QList<ModelN*> models = selectionms();
	if (models.count() == 1)
	{
		ModelN* model = models.first();
		if (model->isRelief())
		{
			setFontMeshWrapper(model->fontMesh());
			group = model->getModelGroup();
			onFontConfigChanged();
		}
	}

	if (m_wrapper == NULL)
	{
		getKernelUI()->setCommandIndex(-1);
		return;
	}

	/* select a relief object, not a relief part or other */
	m_isReliefObject = (group && group->models().count() == 1);

	/* qml display update */
	m_canEmboss = (holdWrapper() && !m_isReliefObject);
	emit canEmbossChanged();
}

/*** private ***/
void ReliefOperateMode::setFontMeshWrapper(FontMeshWrapper* wrapper)
{
	if (m_wrapper)
	{
		m_wrapper->model()->useCustomEffect(NULL);
	}

	m_wrapper = wrapper;

	if (m_wrapper)
	{
		m_wrapper->syncFontMesh();
		m_fontConfig = m_wrapper->config();
		
	}
	else 
	{
		m_fontConfig = NULL;
	}
}

void ReliefOperateMode::onFontConfigChanged()
{
	emit fontConfigChanged();
}

bool ReliefOperateMode::holdWrapper()
{
	return m_wrapper && m_wrapper->valid();
}

void ReliefOperateMode::showRotateEntity()
{
	if (holdWrapper())
	{
		QList<ModelN*> ms = modelns();

		ModelN* relief = m_wrapper->model();
		ModelGroup* group = relief->getModelGroup();
		QMatrix4x4 reliefMatrix = relief->globalMatrix();
		reliefMatrix.setColumn(3, QVector4D(0, 0, 0, 1));

		trimesh::vec3 faceTo = m_wrapper->faceTo();
		QVector3D faceDir(faceTo[0], faceTo[1], faceTo[2]);
		faceDir = reliefMatrix.map(faceDir);
		m_rotateEntity->setCustomRotateAxis(faceDir);

		qtuser_3d::Box3D reliefBox = relief->globalSpaceBox();
		m_rotateEntity->onBoxChanged(reliefBox);
		visShow(m_rotateEntity);
		forceHideBox(true);
		requestVisPickUpdate(true);
	}
}

void ReliefOperateMode::hideRotateEntity()
{
	visHide(m_rotateEntity);
	forceHideBox(false);
}

void ReliefOperateMode::emboss(const QPoint& pos, bool reversible)
{
	if (!m_movingRelief)
		return;

	if (!holdWrapper())
		return;

	if (!m_needIndicate)
	{
		m_needIndicate = true;
		emit needIndicateChanged();
	}

	if (m_needIndicate)
	{
		m_pos = pos;
		emit posChanged();
	}

	int faceID;
	QVector3D position, normal;
	ModelN* model = checkPickModel(pos, position, normal, &faceID);
	if (!model)
	{
		if (m_hoverOnModel)
		{
			m_hoverOnModel = false;
			emit hoverOnModelChanged();
		}
		return;
	}

	if (!m_hoverOnModel)
	{
		m_hoverOnModel = true;
		emit hoverOnModelChanged();
	}

	if (model == m_wrapper->model())	// hover on relief use white indicator
		return;

	if (!reversible)
	{
		m_wrapper->setEmbossTarget(model);
		m_wrapper->emboss(faceID, trimesh::vec3(position[0], position[1], position[2]));
		m_wrapper->updateModel(true);
		m_wrapper->syncScale();
	}
	else 
	{

	}
}


void ReliefOperateMode::rotate(const QQuaternion& q, bool reversible)
{	
	if (!holdWrapper())
	{
		return;
	}

	trimesh::vec3 faceTo = m_wrapper->faceTo();

	QVector3D v1(faceTo[0], faceTo[1], faceTo[2]);

	float dot = QVector3D::dotProduct(v1, q.vector());

	float rad = q.scalar();
	if (dot > 0)
		rad = 2 - rad;

	rad = rad + 1;
	int num = rad / 2;
	rad = rad - num * 2;
	float angle = m_angle + rad;

	if (!reversible)
	{
		rotate(angle);
	}
	else 
	{
		recordStep(m_angle, angle);
	}
}

void ReliefOperateMode::rotate(float angle)
{
	if (!holdWrapper())
	{
		return;
	}

	/* correct angle to 0-2 */
	int circleNum = angle / 2;
	angle -= circleNum * 2.0;

	QQmlProperty::write(tip_object_, "rotateValue", QVariant::fromValue<float>((2 - angle) * 180.0));
	m_wrapper->rotate(angle);
	m_wrapper->updateModel();
}

creative_kernel::ModelGroup* ReliefOperateMode::getValidSelectedGroup()
{
	setFontMeshWrapper(NULL);
	ModelGroup* group = NULL;

	QList<ModelN*> models = selectionms();
	if (models.count() == 1)
	{
		ModelN* model = models.first();
		if (model->isRelief())
		{
			setFontMeshWrapper(model->fontMesh());
			group = model->getModelGroup();
		}
	}

	if (m_wrapper == NULL)
	{
		m_fontConfig = NULL;
		QList<ModelGroup*> groups = selectedGroups();
		if (groups.isEmpty())
		{	/* check multi parts */
			if (!models.isEmpty())
			{
				group = models.first()->getModelGroup();
			}
		}
		else if (groups.size() == 1)
		{
			group = groups.first();
		}
	}

	return group;
}

ReliefUndoConfig ReliefOperateMode::generateUndoConfig()
{
	ReliefUndoConfig undoConfig;
	if (m_fontConfig)
		undoConfig.config = *m_fontConfig;
	else 
		undoConfig.config = m_defaultFontConfig;

	if (m_wrapper)
		undoConfig.modelType = (int)m_wrapper->model()->modelNType();

	return undoConfig;
}

void ReliefOperateMode::recordStep(ReliefUndoConfig oldConfig, ReliefUndoConfig newConfig)
{
	creative_kernel::ModelSpaceUndo* stack = creative_kernel::getKernel()->modelSpaceUndo();
	ReliefUndoCommand* command = new ReliefUndoCommand();

	command->setOperater(this);
	command->setTarget(m_wrapper->model());
	command->setUndoConfig(oldConfig, newConfig);

	stack->executeCommand(command);
}

void ReliefOperateMode::recordStep(float oldAngle, float newAngle)      
{
	if (!holdWrapper())
		return;

	creative_kernel::ModelSpaceUndo* stack = creative_kernel::getKernel()->modelSpaceUndo();
	ReliefRotateUndoCommand* command = new ReliefRotateUndoCommand();

	command->setOperater(this);
	command->setRelief(m_wrapper->model());
	command->setAngle(oldAngle, newAngle);

	stack->executeCommand(command);
} 

void ReliefOperateMode::beginEmboss()
{
	if (!holdWrapper())
		return;

	m_embossCommand = new ReliefMoveUndoCommand();
	m_embossCommand->setOperater(this);
	m_embossCommand->setRelief(m_wrapper->model());

	if (m_wrapper->hasEmbossed())
	{
		m_embossCommand->needInit(false);
		m_embossCommand->setOldEmbossInfo(m_wrapper->lastTargetId(), 
										  m_wrapper->lastFaceId(), 
										  m_wrapper->lastCross(), 
										  m_wrapper->lastEmbossType());
	}
	else 
	{
		m_embossCommand->needInit(true);
	}
}

void ReliefOperateMode::endEmboss()
{
	if (!holdWrapper())
		return;
		
	if (m_embossCommand == NULL)
		return;

	m_embossCommand->setNewEmbossInfo(m_wrapper->lastTargetId(), 
									  m_wrapper->lastFaceId(), 
									  m_wrapper->lastCross(), 
									  m_wrapper->lastEmbossType());

	if (m_embossCommand->valid())
	{
		creative_kernel::ModelSpaceUndo* stack = creative_kernel::getKernel()->modelSpaceUndo();
		stack->executeCommand(m_embossCommand);
	}
	else
	{
		delete m_embossCommand;
	}

	m_embossCommand = NULL;

}

void ReliefOperateMode::recordSubStep()
{

}

void ReliefOperateMode::setTipObjectPos(const QPoint& point)
{
	if (tip_object_)
	{
		QQmlProperty::write(tip_object_, "posX", QVariant::fromValue<int>(point.x()));
		QQmlProperty::write(tip_object_, "posY", QVariant::fromValue<int>(point.y()));
	}
}

void ReliefOperateMode::setTipObjectVisible(bool visible)
{
	if (tip_object_)
	{
		QQmlProperty::write(tip_object_, "isVisible", QVariant::fromValue<bool>(visible));
	}
}

bool ReliefOperateMode::getTipObjectVisible()
{
	if (tip_object_)
	{
		QVariant var = QQmlProperty::read(tip_object_, "isVisible");
		return var.toBool();
	}

	return false;
}