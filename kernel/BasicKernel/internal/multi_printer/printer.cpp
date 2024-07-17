#include "printer.h"
#include "printersettings.h"
#include "internal/render/printerentity.h"
#include "internal/render/plateentity.h"
#include "internal/render/modelnentity.h"
#include "interface/reuseableinterface.h"
#include "interface/machineinterface.h"
#include "interface/layoutinterface.h"
#include "interface/selectorinterface.h"
#include "external/kernel/kernel.h"
#include "external/kernel/reuseablecache.h"
#include "external/slice/sliceattain.h"

#include "internal/tool/layouttoolcommand.h"
#include "external/interface/modelinterface.h"
#include "external/kernel/kernelui.h"

#include "interface/printerinterface.h"
#include "interface/machineinterface.h"
#include "interface/spaceinterface.h"

#include "internal/render/wipetowerentity.h"
#include "cxkernel/data/modelndata.h"

#include "msbase/mesh/get.h"
#include <QtCore/QList>
#include "qtuser3d/trimesh2/conv.h"
#include "kernel/kernelui.h"

//#include "external/message/stayonbordermessage.h"
#include "external/message/wipetowercollisionmessage.h"
#include "internal/utils/byobjecteventhandler.h"
#include "external/message/adaptivelayermessage.h"

#include "external/message/sliceenginefailmessage.h"

const char* PlateTypeKey = "curr_bed_type";
const char* PrintSequenceKey = "print_sequence";

namespace creative_kernel {
	Printer::Printer(QObject* parent)
		:QObject(parent)
		, m_tracer(nullptr)
		, m_pos(QVector3D(0.0f, 0.0f, 0.0f))
		, m_index(-1)
		, m_settings(new PrinterSettings(this))
	{
		Qt3DCore::QNode* root = getKernel()->reuseableCache()->getCachedPrinterEntitiesRoot();
		m_entity = new PrinterEntity(nullptr);
		m_entity->setParent(root);
		m_entity->setTheme(0);

		auto f = [this](IconTypes type) {
			handleClickEvents(type);
		};

		m_plate = m_entity->plateEntity();
		m_plate->setName("");
		m_plate->setClickCallback(f);

		connect(m_entity->wipeTowerEntity(), SIGNAL(signalPositionChange(QVector3D)), this, SLOT(slotWipeTowerPositionChange(QVector3D)));

	}

	Printer::Printer(PrinterEntity* entity, QObject* parent)
		:QObject(parent)
		, m_tracer(nullptr)
		, m_pos(QVector3D(0.0f, 0.0f, 0.0f))
		, m_index(-1)
		, m_settings(new PrinterSettings(this))
	{
		m_entity = entity;
		m_entity->setTheme(0);

		auto f = [this](IconTypes type) {
			handleClickEvents(type);
		};

		m_plate = m_entity->plateEntity();
		m_plate->setName("");
		m_plate->setClickCallback(f);

		connect(m_entity->wipeTowerEntity(), SIGNAL(signalPositionChange(QVector3D)), this, SLOT(slotWipeTowerPositionChange(QVector3D)));
		connect(m_settings, &PrinterSettings::dirtyChanged, this, &Printer::dirty);

	}

	Printer::Printer(const Printer& other)
		:QObject(other.parent())
		, m_tracer(nullptr)
		, m_pos(QVector3D(0.0f, 0.0f, 0.0f))
		, m_index(-1)
		, m_settings(new PrinterSettings(this))
	{
		Qt3DCore::QNode* root = getKernel()->reuseableCache()->getCachedPrinterEntitiesRoot();
		m_entity = new PrinterEntity(nullptr);
		m_entity->setParent(root);
		m_plate = m_entity->plateEntity();

		updateBox(other.m_box);
		m_entity->setTheme(other.m_plate->m_theme);
		//m_plate->setName(other.m_plate->m_name);
		//m_plate->setModelNumber(other.m_plate->m_modelNumber);

		auto f = [this](IconTypes type) {
			handleClickEvents(type);
		};
		m_plate->setClickCallback(f);

		connect(m_entity->wipeTowerEntity(), SIGNAL(signalPositionChange(QVector3D)), this, SLOT(slotWipeTowerPositionChange(QVector3D)));
		connect(m_settings, &PrinterSettings::dirtyChanged, this, &Printer::dirty);
	}

	Printer::~Printer()
	{
		disconnectModels();

		m_tracer = nullptr;
		m_plate->setClickCallback(nullptr);
		m_entity->setParent((Qt3DCore::QNode *)nullptr);
		delete m_entity;
		m_entity = nullptr;
		m_plate = nullptr;

		if (m_sliceInfo.attain)
			delete m_sliceInfo.attain;

		if (m_sliceInfo.sliceCache)
			delete m_sliceInfo.sliceCache;
	}
	
	void Printer::applyLayersColor(const QVariantList& layersColor)
	{
		for(ModelN* model : m_modelsInside)
		{
			model->setLayersColor(layersColor);
		}
	}

	void Printer::reloadLayersConfig()
	{
		m_settings->reload();
	}

	void Printer::clearLayersConfig()
	{
		m_settings->clearLayerConfig();
	}	

	crslice2::plateInfo Printer::getPlateInfo()
	{
		return m_settings->getPlateInfo();
	}

	PrinterEntity* Printer::entity()
	{
		return m_entity;
	}

	void Printer::addTracer(PrinterTracer* tracer)
	{
		m_tracer = tracer;
	}

	bool Printer::selected()
	{
		return m_plate->selected();
	}

	void Printer::setSelected(bool select)
	{
		if (selected() != select)
		{
			m_plate->setSelected(select);
			updateAssociateModelsState();
		}

		if (select)
		{
			emit signalSelectPrinter(this);
			if (m_tracer)
			{
				m_tracer->didSelectPrinter(this);
			}
		}
	}

	int Printer::index()
	{
		return m_index;
	}

	void Printer::setIndex(int idx)
	{
		if (m_index != idx)
		{
			m_index = idx;
			m_entity->setIndex(idx);
			emit signalIndexChanged();
		}
	}

	const qtuser_3d::Box3D& Printer::box()
	{
		return m_box;
	}

	qtuser_3d::Box3D Printer::globalBox()
	{
		QVector3D min = m_box.min + m_pos;
		QVector3D max = m_box.max + m_pos;
		qtuser_3d::Box3D box(min, max);
		return box;
	}

	void Printer::updateBox(const qtuser_3d::Box3D& box)
	{
		bool change = (m_box != box);
		m_box = box;
		m_entity->updateBox(box);

		updateAssociateModelsState();
		/*if (change && m_tracer)
		{
			m_tracer->didBoundingBoxChange(this, box);
		}*/
	}

	QVector3D Printer::position()
	{
		return m_pos;
	}

	void Printer::setPosition(const QVector3D& pos)
	{
		if (pos == m_pos)
			return;

		m_pos = pos;

		QMatrix4x4 p;
		p.setToIdentity();
		p.translate(pos);
		m_entity->setPose(p);

		updateAssociateModelsState();
	}

	const QString& Printer::name()
	{
		return m_plate->name();
	}

	void Printer::setName(const QString& str)
	{
		if (m_plate->name() == str)
			return;

		m_plate->setName(str);
		m_plate->resetGroundGeometry();
		emit nameChanged();
	}

	void Printer::setModelNumber(const QString& str)
	{
		m_plate->setModelNumber(str);
	}

	bool Printer::lock()
	{
		return m_plate->lock();
	}

	void Printer::setLock(bool lock)
	{
		m_plate->setLock(lock);
	}

	void Printer::setTheme(int theme)
	{
		m_entity->setTheme(theme);
	}

	void Printer::handleClickEvents(IconTypes type)
	{
		qDebug() << "on click" << type;

		if (m_tracer == nullptr) return;

		switch (type)
		{

		case IconType::Close:
		{
			if (m_tracer->shouldDeletePrinter(this))
			{
				m_tracer->willDeletePrinter(this);
				removePrinter(index(), true);
			}
		}
		break;

		case IconType::Autolayout:
		{
			if (!lock())
			{
				m_tracer->willAutolayoutForPrinter(this);

				layoutModels(modelsInside() + modelsOnBorder(), m_index, false, false);
			}
		}
		break;

		case IconType::PickBottom:
		{
			if (!lock())
			{
				m_tracer->willPickBottomForPrinter(this);
			}
		}
		break;

		case IconType::Platform:
		{
			m_tracer->willSelectPrinter(this);
		}
		break;

		case IconType::Lock:
		{
			bool l = !lock();
			setLock(l);
			m_tracer->didLockPrinterOrNot(this, l);
		}
		break;

		case IconType::Setting:
		{
			m_tracer->willSettingPrinter(this);
			getKernelUI()->requestPrinterSettingsDialog(this);
		}
		break;

		case IconType::EditName:
		{
			m_tracer->willEditNameForPrinter(this, name());
			getKernelUI()->requestPrinterNameEditorDialog(this);
		}
		break;

		case IconType::ModifyMachine:
		{
			m_tracer->willModifyPrinter(this);
		}
		break;

		case IconType::Order:
		{

		}
		break;



		default:
			break;
		}
	}

	QList<ModelN*> Printer::currentModelsInsideVisible()
	{
		QList<ModelN*> ms;
		for (auto m : m_modelsInside)
		{
			if (m->isVisible())
			{
				ms.push_back(m);
			}
		}
		return ms;
	}

	QList<ModelN*> Printer::modelsInsideVisible()
	{
		return currentModelsInsideVisible();
	}

	const QList<ModelN*>& Printer::modelsInside()
	{
		return m_modelsInside;
	}

	void Printer::replaceModelsInside(QList<ModelN*>& list)
	{
		bool equal = checkTwoListEqual(m_modelsInside, list);
		if (!equal)
		{
			//for (auto m : m_modelsInside)
			disconnectModels();

			m_modelsInside = list;
			m_insideModelSerialNames.clear();
			for (auto m : m_modelsInside)
			{
				m_insideModelSerialNames.append(m->getSerialName());
				connect(m, &ModelN::dirtyChanged, this, &Printer::slotModelsDirtyChanged, Qt::UniqueConnection);
			}
			updateAssociateModelsState();

			emit signalModelsInsideChange(list);
			setDirty(true);
		}
	}

	int Printer::modelsInsideCount()
	{
		return m_modelsInside.length();
	}

	const QList<ModelN*>& Printer::modelsOnBorder()
	{
		return m_modelsOnBorder;
	}

	void Printer::replaceModelsOnBorder(QList<ModelN*>& list)
	{
		bool equal = checkTwoListEqual(m_modelsOnBorder, list);
		if (!equal)
		{
			m_modelsOnBorder = list;
			updateAssociateModelsState();
		}
	}

	bool Printer::hasModelsOnBorder()
	{
		return m_modelsOnBorder.size() > 0;
	}

	void Printer::updateAssociateModelsState()
	{
		if (selected())
		{
			for (ModelN* model : m_modelsInside) {

				Qt3DCore::QEntity* entity = model->getModelEntity();
				ModelNEntity* modelE = qobject_cast<ModelNEntity*>(entity);
				if (modelE)
				{
					modelE->setAssociatePrinterBox(globalBox());
				}

			}

			for (ModelN* model : m_modelsOnBorder) {

				Qt3DCore::QEntity* entity = model->getModelEntity();
				ModelNEntity* modelE = qobject_cast<ModelNEntity*>(entity);
				if (modelE)
				{
					modelE->setAssociatePrinterBox(globalBox());
				}
			}
		}
		else {

			qtuser_3d::Box3D box(QVector3D(-100000.0f, -100000.0f, -100000.0f), QVector3D(100000.0f, 100000.0f, 100000.0f));

			for (ModelN* model : m_modelsInside) {

				Qt3DCore::QEntity* entity = model->getModelEntity();
				ModelNEntity* modelE = qobject_cast<ModelNEntity*>(entity);
				if (modelE)
				{
					modelE->setAssociatePrinterBox(box);
				}

			}

			for (ModelN* model : m_modelsOnBorder) {

				Qt3DCore::QEntity* entity = model->getModelEntity();
				ModelNEntity* modelE = qobject_cast<ModelNEntity*>(entity);
				if (modelE)
				{
					modelE->setAssociatePrinterBox(box);
				}
			}
		}
	}

	bool Printer::checkTwoListEqual(const QList<ModelN*>& a, const QList<ModelN*>& b)
	{
		bool equal = true;
		if (a.size() == b.size())
		{
			for (ModelN* m : a)
			{
				if (!b.contains(m))
				{
					equal = false;
					break;
				}
			}
		}
		else {
			equal = false;
		}

		return equal;
	}

	bool Printer::isDirty()
	{
		return m_isDirty || m_settings->isDirty();
	}

	void Printer::resetDirty()
	{
		m_settings->resetDirty();
		setDirty(false);
	}

	void Printer::dirty()
	{
		setDirty(true);
	}

	void Printer::setDirty(bool dirty)
	{
		if (m_isDirty != dirty)
		{
			m_isDirty = dirty;
			setIsSliced(!m_isDirty);
			if (m_isDirty)
			{
				setAttain(NULL);
			}
		} 
		else 
		{
			if (m_isDirty) 
			{
				if (!m_reserveSliceCacheFlag)
				{
					m_reserveSliceCacheFlag = false;
					clearSliceCache();
					emit signalAttainChanged();
				}
			}
		}
		clearSliceError();
		emit signalDirtyChanged();
	}

	void Printer::setVisibility(bool visibility)
	{
		m_entity->showPrinterEntity(visibility);
	}

	bool Printer::isVisible()
	{
		return m_entity->isEnabled();
	}

	void Printer::applySettings()
	{
		m_settings->applySettings();
		settingsApplyed();
	}

	void Printer::setParameter(const QString& key, const QString& value)
	{
		m_settings->setValue(key, value);

		if (key == PlateTypeKey)
		{
			onLocalPlateTypeChanged(value);
		}
		else if (key == PrintSequenceKey)
		{
			if (m_tracer)
			{
				m_tracer->localPrintSequenceChanged(this, value);
			}
		}
		parameterChanged(key, value);
	}

	QString Printer::parameter(const QString& key)
	{
		return m_settings->value(key);
	}
	
	QObject* Printer::settingsObject()
	{
		return (QObject*)m_settings;
	}

	std::map<std::string, std::string> Printer::stdSettings()
	{
		return m_settings->stdSettings();
	}

	void Printer::setSliceError(const QString& error)
	{
		m_sliceInfo.sliceError = error;
		checkSliceError();
	}

	void Printer::clearSliceError()
	{
		m_sliceInfo.sliceError.clear();
		checkSliceError();
	}

	void Printer::checkSliceError()
	{
		auto kernelUi = getKernelUI();
		if (m_sliceInfo.sliceError.isEmpty())
		{
			kernelUi->destroyMessage(MessageType::SliceEngineFail);
		}
		else 
		{
			SliceEngineFailMessage* msg = new SliceEngineFailMessage(m_sliceInfo.sliceError);
			kernelUi->requestMessage(msg);
		}
	}

	qtuser_3d::Box3D Printer::modelsInsideBoundingBox()
	{
		return modelsBox(m_modelsInside);
	}

	void Printer::slotWipeTowerPositionChange(QVector3D newPosition)
	{
		if (selected())
		{
			checkWipeTowerCollide();
		}
		setDirty(true);
	}

	void Printer::slotModelsDirtyChanged()
	{
		ModelN* model = dynamic_cast<ModelN*>(sender());
		if (!model)
		{
			return;
		}

		bool modelsDirty = model->isDirty();
		//if (modelsDirty && modelsDirty != m_isDirty)
		if (modelsDirty)
		{
			setDirty(modelsDirty);
		}

		if (hasModelInsideUseColors())
		{
			m_settings->clearLayerConfig(crslice2::Plate_Type::ToolChange);
		}
	}

	bool Printer::checkAndUpdateWipeTowerState()
	{
		//bool lastState = m_entity->wipeTowerEntity()->isVisible();

		bool show = (m_plate->viewModel() == PlateViewMode::EDIT) && checkWipeTowerShouldShow();

		m_entity->wipeTowerEntity()->setVisibility(show);

		if (show)
		{
			updateWipeTowerState();
		}

		if (selected()/*&& lastState != show*/)
		{
			checkModelInsideVisibleLayers(true);
		}

		return show;
	}

	bool Printer::checkWipeTowerShouldShow()
	{
		if (m_tracer == nullptr) return false;

		const WipeTowerGlobalSetting& setting = m_tracer->wipeTowerCurrentSetting();
		if (setting.enable == false)
			return false;

		if (m_modelsInside.isEmpty())
			return false;

		//逐个打印不显示擦拭塔
		QString seq = getPrintSequence();
		if (seq.toLower() == "by object")
		{
			return false;
		}

		int colorSize = getModelInsideUseColors().size();
		if (colorSize >= 2)
			return true;

		return m_settings->specificExtruderHeights().size() > 0;
	}

	bool Printer::hasModelInsideUseColors(){
		return  getModelInsideUseColors().size() > 1;
	}

	QList<QVector4D> Printer::getModelInsideUseColors()
	{
		if (m_tracer == nullptr) return QList<QVector4D>();

		//��������ɫ˳����currentColors�е�˳��Ϊ��
		std::vector<trimesh::vec> allColors = creative_kernel::currentColors();
		QSet<int> allIndexs;

		const WipeTowerGlobalSetting& setting = m_tracer->wipeTowerCurrentSetting();
		if (setting.enableSupport)
		{
			//支撑耗材"缺省"为0，用户选择的耗材索引值从1开始
			if (setting.supportFilamentColorIndex >= 1)
			{
				allIndexs.insert(setting.supportFilamentColorIndex-1);
			}
			if (setting.supportInterfaceFilamentColorIndex >= 1)
			{
				allIndexs.insert(setting.supportInterfaceFilamentColorIndex - 1);
			}
		}

		QList<ModelN*> visibleList = currentModelsInsideVisible();
		for (auto m : visibleList)
		{
			allIndexs.unite(m->data()->colorIndexs);
		}

		QList<int> indexsList = allIndexs.toList();
		std::sort(indexsList.begin(), indexsList.end());

		QList<QVector4D> list;

		QList<int>::const_iterator it = indexsList.constBegin();
		while (it != indexsList.constEnd())
		{
			int index = *it;
			if (0 <= index && index < allColors.size())
			{
				const trimesh::vec& cls = allColors.at(index);
				list.push_back(QVector4D(cls.x, cls.y, cls.z, 0.65f));
			}

			++it;
		}

		return list;
	}

	qtuser_3d::Box3D Printer::getWipeTowerBox(QList<QVector4D>& colors)
	{
		colors = getModelInsideUseColors();

		qtuser_3d::Box3D bbox = modelsInsideBoundingBox();
		float maxH = bbox.size().z();

		std::vector<int> plate_extruders;
		plate_extruders.resize(colors.size());
		if ((plate_extruders.size() == 1) && (m_settings->specificExtruderHeights().size() > 0))
			plate_extruders.push_back(1);

		const WipeTowerGlobalSetting& setting = m_tracer->wipeTowerCurrentSetting();
		float volume = setting.volume;
		float width = setting.width;
		float layerHeight = setting.layerHeight;

		trimesh::vec3 size = msbase::estimate_wipe_tower_size(width, volume, plate_extruders, layerHeight, maxH);
		QVector3D min = QVector3D(size.x * -0.50, size.y * -0.5, 0.0);
		QVector3D max = QVector3D(size.x * 0.50, size.y * 0.5, size.z);
		qtuser_3d::Box3D towerBox(min, max);

		return towerBox;
	}

	void Printer::updateWipeTowerState()
	{
		if (m_tracer == nullptr) return;

		QList<QVector4D> colors;
		qtuser_3d::Box3D towerBox = getWipeTowerBox(colors);
		WipeTowerEntity* tower = m_entity->wipeTowerEntity();
		tower->setLocalBox(towerBox);
		tower->setColors(colors);
	}

	bool Printer::checkWipeTowerCollide(bool showAlert)
	{
		qtuser_3d::Box3D box = m_entity->wipeTowerEntity()->localBox();
		if (!box.valid)
			return false;

		bool intersect = false;

		WipeTowerEntity *tower = m_entity->wipeTowerEntity();
		if (tower->isVisible())
		{
			box.translate(tower->position() + position());
			QList<ModelN*> models = currentModelsInsideVisible();
			for (auto m : models)
			{
				if (box.intersects(m->globalSpaceBox()))
				{
					intersect = true;
					break;
				}
			}
		}

		if (showAlert)
		{
			if (intersect)
			{
				qDebug() << "wipe tower intersects";
				WipeTowerCollisionMessage* message = new WipeTowerCollisionMessage(m_entity->wipeTowerEntity());
				getKernelUI()->requestMessage(message);
			}
			else
			{
				getKernelUI()->destroyMessage(MessageType::WipeTowerCollision);
			}
		}
		
		return intersect;
	}

	QImage Printer::picture() const
	{
		return m_sliceInfo.picture;
	}

	void Printer::setPicture(const QImage& p)
	{
		m_sliceInfo.picture = p.copy();
	}

	bool Printer::isSliced()
	{
		return m_sliceInfo.isSliced;
	}

	void Printer::setIsSliced(bool isSliced)
	{
		if (m_sliceInfo.isSliced == isSliced)
			return;

		m_sliceInfo.isSliced = isSliced;
		emit signalSlicedStateChanged();
	}
	
	void Printer::reserveSliceCache()
	{
		m_reserveSliceCacheFlag = true;
	}

	void Printer::clearSliceCache()
	{
		if (m_sliceInfo.sliceCache)
		{
			delete m_sliceInfo.sliceCache;
			m_sliceInfo.sliceCache = NULL;
			// emit signalAttainChanged();
		}
	}

	QObject* Printer::sliceCache()
	{
		if (!m_sliceInfo.sliceCache)
			return m_sliceInfo.attain;
		else 
			return m_sliceInfo.sliceCache;
	}

	QObject* Printer::attain()
	{
		return m_sliceInfo.attain;
	}

	void Printer::setAttain(SliceAttain* attain)
	{
		if (attain == m_sliceInfo.attain)
			return;

		if (!m_reserveSliceCacheFlag)
			clearSliceCache();

		if (m_sliceInfo.attain && m_reserveSliceCacheFlag)
		{
			clearSliceCache();
			m_sliceInfo.sliceCache = m_sliceInfo.attain;
			m_sliceInfo.attain = NULL;
		}

		m_reserveSliceCacheFlag = false;
        m_sliceInfo.attain = (QObject*)attain;
		emit signalAttainChanged();
	}

	std::vector<trimesh::vec3> Printer::wipeTowerOutlinePath(bool global)
	{
		QVector3D pos = m_entity->wipeTowerEntity()-> positionOfLeftBottom();
		QVector3D size = m_entity->wipeTowerEntity()->localBox().size();

		std::vector<trimesh::vec3> outline;

		outline.push_back(qtuser_3d::qVector3D2Vec3(pos));
		outline.push_back(qtuser_3d::qVector3D2Vec3(pos + QVector3D(size.x(), 0.0, 0.0)));
		outline.push_back(qtuser_3d::qVector3D2Vec3(pos + QVector3D(size.x(), size.y(), 0.0)));
		outline.push_back(qtuser_3d::qVector3D2Vec3(pos + QVector3D(0.0, size.y(), 0.0)));
		outline.push_back(qtuser_3d::qVector3D2Vec3(pos));

		if (global)
		{
			QVector3D pos = position();
			trimesh::fxform xf = trimesh::fxform::trans(trimesh::vec3(pos.x(), pos.y(), 0.0f));

			for (trimesh::vec3& point : outline)
				point = xf * point;
		}

		return outline;
	}

	void Printer::setPrinterEditMode()
	{
		m_plate->setViewMode(PlateViewMode::EDIT);
		checkAndUpdateWipeTowerState();
	}

	void Printer::setPrinterPreviewMode()
	{
		m_plate->setViewMode(PlateViewMode::PREVIEW);
		m_entity->wipeTowerEntity()->setVisibility(false);
	}

	void Printer::setCloseEnable(bool enable)
	{
		m_plate->setCloseEntityEnable(enable);
	}

	QString Printer::getLocalPrintSequence()
	{
		return parameter(PrintSequenceKey);
	}

	QString Printer::getPrintSequence()
	{
		QString seq = getLocalPrintSequence();
		if (seq.isEmpty())
		{
			seq = m_tracer->globalPrintSequence();
		}
		return seq;
	}

	QString Printer::getLocalPlateType()
	{
		return parameter(PlateTypeKey);
	}

	void Printer::onLocalPlateTypeChanged(QString str)
	{
		if (str.isEmpty())
		{
			if (m_tracer)
			{
				str = m_tracer->globalPlateType();
			}
		}

		setPlateStyle(str);
	}

	void Printer::onGlobalPlateTypeChanged(QString str)
	{
		QString type = getLocalPlateType();
		if (type.isEmpty())
		{
			setPlateStyle(str);
		}
	}

	void Printer::setPlateStyle(QString typeStr)
	{
		PlateStyle style = PlateStyle::Custom_Plate;
		if (typeStr.compare("Textured PEI plate", Qt::CaseInsensitive) == 0)
		{
			style = PlateStyle::Textured_PEI_Plate;
		}
		else if (typeStr.compare("High Temp Plate", Qt::CaseInsensitive) == 0)
		{
			style = PlateStyle::Smooth_PEI_Plate;
		}
		m_plate->setPlateStyle(style);
	}

	bool Printer::checkModelInsideVisibleLayers(bool showAlert)
	{
		qDebug() << "checkModelInsideVisibleLayers";
		
		bool error = false;
		QList<ModelN*> visiModels = currentModelsInsideVisible();
		for (auto model : visiModels)
		{
			bool support_enabled = model->supportEnabled();
			QString support_type = model->supportStructure();
			bool hasAdaptiveLayer = model->layerHeightProfile().size() > 0;
			if (hasAdaptiveLayer && support_enabled && ("tree(auto)" == support_type || "tree(manual)" == support_type))
			{
				error = true;
				break;
			}
		}
		if (error)
		{
			if (showAlert)
			{
				qDebug() << "supportTree adaptive intersects";
				AdaptiveLayerMessage* message = new AdaptiveLayerMessage(true);
				getKernelUI()->requestMessage(message);
			}
			else
			{
				getKernelUI()->destroyMessage(MessageType::AdaptiveLayer);
			}
		}
		else
		{
			bool show = checkWipeTowerShouldShow();
			if (show)
			{
				auto layers = std::vector<std::vector<double>>();
				for (auto model : visiModels)
				{
					auto modelLayers = model->layerHeightProfile();
					layers.push_back(modelLayers);
				}
				error = !getKernel()->modelSpace()->checkLayerHeightEqual(layers);
			}

			if (showAlert)
			{
				if (error)
				{
					qDebug() << "AdaptiveLayerMessage intersects";
					AdaptiveLayerMessage* message = new AdaptiveLayerMessage();
					getKernelUI()->requestMessage(message);
				}
				else
				{
					getKernelUI()->destroyMessage(MessageType::AdaptiveLayer);
				}
			}
		}
		return error;
	}

	bool Printer::checkVisiableModelError()
	{
		const QList<ModelN*> list = currentModelsInsideVisible();
		bool error = list.isEmpty();
		if (error)
		{
			return error;
		}

		error = hasModelsOnBorder();

		if (!error)
		{
			QString seq = getPrintSequence();
			if (seq.toLower() == "by object")
			{
				//逐个打印
				
				QList<ModelN*> colList = ByObjectEventHandler::collisionCheck(list);
				error = !colList.isEmpty();

				if (!error)
				{
					QList<ModelN*> tallList = ByObjectEventHandler::heightCheck(list, creative_kernel::extruderClearanceHeightToRod());
					error = !tallList.isEmpty();

				}
			}
			else if (seq.toLower() == "by layer") {
				//逐层
				error = checkModelInsideVisibleLayers(false);
			}
		}

		return error;
	}

	void Printer::adjustWipeTowerPositionWhenInvalid()
	{
		bool show = checkWipeTowerShouldShow();
		if (show)
		{
			WipeTowerEntity* tower = m_entity->wipeTowerEntity();
			QList<QVector4D> colors;
			qtuser_3d::Box3D towerBox = getWipeTowerBox(colors);

			QVector3D better;
			QVector3D current = tower->position();
			tower->setLocalBoxOnly(towerBox);
			bool should = tower->shouldTranslateTo(current, better);
			if (!should)
			{
				tower->translateTo(better);
			}
		}
	}

	void Printer::moveWipeTower(const QVector3D& delta)
	{
		WipeTowerEntity* tower = m_entity->wipeTowerEntity();
		QVector3D current = tower->position();
		current += delta;
		tower->translateTo(current);
	}

	void Printer::disconnectModels()
	{
		for (auto mstr : m_insideModelSerialNames)
		{
			ModelN* m = getModelNBySerialName(mstr);
			if (m)
			{
				disconnect(m, &ModelN::dirtyChanged, this, &Printer::slotModelsDirtyChanged);
			}
		}
	}
}
