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

#include "internal/parameter/parametermanager.h"
#include "internal/parameter/printextruder.h"
#include "internal/parameter/printmachine.h"
#include "internal/parameter/printmaterial.h"
#include "internal/tool/layouttoolcommand.h"
#include "external/kernel/kernelui.h"

#include "interface/printerinterface.h"
#include "interface/machineinterface.h"
#include "interface/spaceinterface.h"

#include "internal/render/wipetowerentity.h"

#include "msbase/mesh/get.h"
#include <QtCore/QList>

#include "qtuser3d/trimesh2/conv.h"

#include "data/spaceutils.h"

#include "kernel/kernel.h"

//#include "external/message/stayonbordermessage.h"
#include "external/message/wipetowercollisionmessage.h"
#include "internal/utils/byobjecteventhandler.h"
#include "internal/utils/operationutil.h"
#include "external/message/adaptivelayermessage.h"

#include <Qt3DRender/QCamera>
#include "topomesh/interface/utils.h"
#include <QFileInfo>
#include "interface/projectinterface.h"
#include "internal/project_cx3d/autosavejob.h"
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
		m_entity->setPrinter(this);

		auto f = [this](IconTypes type) {
			handleClickEvents(type);
		};

		m_plate = m_entity->plateEntity();
		m_plate->setName("");
		m_plate->setClickCallback(f);

		traceSpace(this);

		connect(m_entity->wipeTowerEntity(), SIGNAL(signalPositionChange(QVector3D)), this, SLOT(slotWipeTowerPositionChange(QVector3D)));

        // 信号绑定
        auto parameterManager = getKernel()->parameterManager();
        connect(parameterManager, &ParameterManager::currentColorsChanged, this, &Printer::requestPicture, Qt::UniqueConnection);

		m_requestPictureTimer.setSingleShot(true);
		m_requestPictureTimer.callOnTimeout(this, &Printer::triggerCapture);

		connect(this, &Printer::signalDirtyChanged, this, &Printer::signalCanSliceChanged);
		connect(this, &Printer::signalGCodeSettingsDirtyChanged, this, &Printer::signalCanSliceChanged);

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
		m_entity->setPrinter(this);

		auto f = [this](IconTypes type) {
			handleClickEvents(type);
		};

		m_plate = m_entity->plateEntity();
		m_plate->setName("");
		m_plate->setClickCallback(f);
		traceSpace(this);

		connect(m_entity->wipeTowerEntity(), SIGNAL(signalPositionChange(QVector3D)), this, SLOT(slotWipeTowerPositionChange(QVector3D)));
		connect(m_settings, &PrinterSettings::dirtyChanged, this, &Printer::dirty);

        // 信号绑定
        auto parameterManager = getKernel()->parameterManager();
        connect(parameterManager, &ParameterManager::currentColorsChanged, this, &Printer::requestPicture, Qt::UniqueConnection);
		m_requestPictureTimer.setSingleShot(true);
		m_requestPictureTimer.callOnTimeout(this, &Printer::triggerCapture);

		connect(this, &Printer::signalDirtyChanged, this, &Printer::signalCanSliceChanged);
		connect(this, &Printer::signalGCodeSettingsDirtyChanged, this, &Printer::signalCanSliceChanged);
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
		m_entity->setPrinter(this);

		updateBox(other.m_box);
		m_entity->setTheme(other.m_plate->m_theme);
		//m_plate->setName(other.m_plate->m_name);
		//m_plate->setModelNumber(other.m_plate->m_modelNumber);

		auto f = [this](IconTypes type) {
			handleClickEvents(type);
		};
		m_plate->setClickCallback(f);
		traceSpace(this);

        // 信号绑定
        auto parameterManager = getKernel()->parameterManager();
        connect(parameterManager, &ParameterManager::currentColorsChanged, this, &Printer::requestPicture, Qt::UniqueConnection);
		connect(m_entity->wipeTowerEntity(), SIGNAL(signalPositionChange(QVector3D)), this, SLOT(slotWipeTowerPositionChange(QVector3D)));
		connect(m_settings, &PrinterSettings::dirtyChanged, this, &Printer::dirty);

		m_requestPictureTimer.setSingleShot(true);
		m_requestPictureTimer.callOnTimeout(this, &Printer::triggerCapture);
		
		connect(this, &Printer::signalDirtyChanged, this, &Printer::signalCanSliceChanged);
		connect(this, &Printer::signalGCodeSettingsDirtyChanged, this, &Printer::signalCanSliceChanged);
	}

	Printer::~Printer()
	{
		unTraceSpace(this);
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

			if (select)
			{
				emit signalSelectPrinter(this);
				if (m_tracer)
				{
					m_tracer->didSelectPrinter(this);
				}
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

			if (m_tracer)
			{
				m_tracer->printerIndexChanged(this, idx);
			}
			emit signalIndexChanged();
		}
	}

	trimesh::dbox3 Printer::printerBox()
	{
		trimesh::dbox3 b;
		b += trimesh::dvec3(m_box.min.x() + m_pos.x(),
			m_box.min.y() + m_pos.y(),
			m_box.min.z() + m_pos.z());
		b += trimesh::dvec3(m_box.max.x() + m_pos.x(),
			m_box.max.y() + m_pos.y(),
			m_box.max.z() + m_pos.z());
		return b;
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

		if (m_tracer)
		{
			m_tracer->nameChanged(this, str);
		}
		emit nameChanged();
		triggerAutoSave(QList<ModelGroup*>(),AutoSaveJob::PLATE);
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

	void Printer::setWipeTowerPosition_LeftBottom(const QVector3D& pos)
	{
		m_entity->wipeTowerEntity()->setLeftBottomPosition(pos.x(), pos.y());
	}

	QVector3D Printer::getWipeTowerPosition_LeftBottom()
	{
		return m_entity->wipeTowerEntity()->positionOfLeftBottom();
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

				layoutModelGroups(getPrinterModelGroupIds(m_index), m_index);
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
			disconnectModels();

			m_modelsInside = list;
			m_insideModelSerialNames.clear();
			for (auto m : m_modelsInside)
			{
				m_insideModelSerialNames.append(m->getSerialName());
				// connect(m, &ModelN::dirtyChanged, this, &Printer::slotModelsDirtyChanged, Qt::UniqueConnection);
				connect(m, &ModelN::formModified, this, &Printer::requestPicture, Qt::UniqueConnection);
				connect(m, &ModelN::supportEnabledChanged, this, &Printer::slotUpdateWipeTowerState, Qt::UniqueConnection);
			}

			emit signalModelsInsideChange(list);
			setDirty(true);
			requestPicture();
		}
	}

	const QList<ModelGroup*>& Printer::modelGroupsInside()
	{
		return m_groupsInside;
	}

	void Printer::replaceModelGroupsInside(QList<ModelGroup*>& list)
	{
		bool equal = checkTwoGroupListEqual(m_groupsInside, list);
		if (!equal)
		{
			for (ModelGroup* g : m_groupsInside)
			{
				disconnect(g, &ModelGroup::dirtyChanged, this, &Printer::slotModelGroupsDirtyChanged);
				disconnect(g, &ModelGroup::formModified, this, &Printer::requestPicture);
				disconnect(g, &ModelGroup::supportEnabledChanged, this, &Printer::slotUpdateWipeTowerState);
				disconnect(g, &ModelGroup::supportSupportFilamentChanged, this, &Printer::slotUpdateWipeTowerState);
				disconnect(g, &ModelGroup::supportInterfaceFilamentChanged, this, &Printer::slotUpdateWipeTowerState);
			}

			m_groupsInside = list;

			QList<ModelN*> modelList;
			for (ModelGroup* g : list)
			{
				modelList << g->models();
				connect(g, &ModelGroup::dirtyChanged, this, &Printer::slotModelGroupsDirtyChanged, Qt::UniqueConnection);
				connect(g, &ModelGroup::formModified, this, &Printer::requestPicture, Qt::UniqueConnection);
				connect(g, &ModelGroup::supportEnabledChanged, this, &Printer::slotUpdateWipeTowerState);
				connect(g, &ModelGroup::supportSupportFilamentChanged, this, &Printer::slotUpdateWipeTowerState);
				connect(g, &ModelGroup::supportInterfaceFilamentChanged, this, &Printer::slotUpdateWipeTowerState);
			}

			replaceModelsInside(modelList);

			if (m_tracer)
			{
				m_tracer->modelGroupsInsideChange(this, list);
			}
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

	const QList<ModelGroup*>& Printer::modelGroupsOnBorder()
	{
		return m_groupsOnBorder;
	}

	void Printer::replaceModelsOnBorder(QList<ModelN*>& list)
	{
		bool equal = checkTwoListEqual(m_modelsOnBorder, list);
		if (!equal)
		{
			m_modelsOnBorder = list;
		}
	}

	bool Printer::hasModelsOnBorder()
	{
		return m_modelsOnBorder.size() > 0;
	}


	void Printer::replaceModelGroupsOnBorder(QList<ModelGroup*>& list)
	{
		bool equal = checkTwoGroupListEqual(m_groupsOnBorder, list);
		if (!equal)
		{
			m_groupsOnBorder = list;

			QList<ModelN*> modelList;
			for (ModelGroup* g : list)
			{
				modelList << g->models();
			}
			replaceModelsOnBorder(modelList);
		}
	}

	bool Printer::hasModelGroupsOnBorder()
	{
		return m_groupsOnBorder.size() > 0;
	}

	QString Printer::primeTowerPosition(int axis)
	{
		this->adjustWipeTowerPositionWhenInvalid();
		QVector3D pos = this->entity()->wipeTowerEntity()->positionOfLeftBottom();
		return QString("%1").arg(pos[axis]);
	}


	void Printer::updateAssociateModelsState()
	{
		if (selected())
		{
			for (ModelN* model : m_modelsInside) {

				ModelNEntity* modelE = model->entity();
				if (modelE)
				{
					modelE->setAssociatePrinterBox(globalBox());
				}

			}

			for (ModelN* model : m_modelsOnBorder) {
				ModelNEntity* modelE = model->entity();
				if (modelE)
				{
					modelE->setAssociatePrinterBox(globalBox());
				}
			}
		}
		else {

			qtuser_3d::Box3D box(QVector3D(-100000.0f, -100000.0f, -100000.0f), QVector3D(100000.0f, 100000.0f, 100000.0f));

			for (ModelN* model : m_modelsInside) {

				ModelNEntity* modelE = model->entity();
				if (modelE)
				{
					modelE->setAssociatePrinterBox(box);
				}

			}

			for (ModelN* model : m_modelsOnBorder) {

				ModelNEntity* modelE = model->entity();
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
			for (auto m : a)
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


	bool Printer::checkTwoGroupListEqual(const QList<ModelGroup*>& a, const QList<ModelGroup*>& b)
	{
		bool equal = true;
		if (a.size() == b.size())
		{
			for (auto m : a)
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
		requestPicture();
	}

	void Printer::setDirty(bool dirty)
	{
		if (m_isDirty != dirty)
		{
			m_SlicedPlateName = QString();
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

	bool Printer::isGCodeSettingDirty()
	{
		return m_isGCodeSettingDirty;
	}

	void Printer::resetGCodeSettingDirty()
	{
		m_isGCodeSettingDirty = false;
		emit signalGCodeSettingsDirtyChanged();
	}

	void Printer::gCodeSettingDirty()
	{
		m_isGCodeSettingDirty = true;
		emit signalGCodeSettingsDirtyChanged();
	}

	bool Printer::canSlice()
	{
		return isDirty() || isGCodeSettingDirty();
	}

	void Printer::setVisibility(bool visibility)
	{
		m_entity->showPrinterEntity(visibility);
	}

	bool Printer::isVisible()
	{
		return m_entity->isEnabled();
	}

	trimesh::dbox3 Printer::modelsInsideBoundingBox()
	{
		return modelsBox(m_modelsInside);
	}

	void Printer::slotUpdateWipeTowerState()
	{
		bool show = checkAndUpdateWipeTowerState();
		if (show && selected())
			checkWipeTowerCollide();
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

	void Printer::slotModelGroupsDirtyChanged()
	{
		ModelGroup* modelGroup = dynamic_cast<ModelGroup*>(sender());
		if (!modelGroup)
		{
			return;
		}

		bool modelGroupsDirty = modelGroup->isDirty();
		//if (modelsDirty && modelsDirty != m_isDirty)
		if (modelGroupsDirty)
		{
			setDirty(modelGroupsDirty);
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

		bool supportEnable = false;
		for (ModelGroup* group : m_groupsInside)
		{
			if (group->checkSupportSettingState())
			{
				supportEnable = true;
				break;
			}
		}

		const WipeTowerGlobalSetting& setting = m_tracer->wipeTowerCurrentSetting();
		if (setting.enable == false && supportEnable == false)
			return false;

		if (m_modelsInside.isEmpty())
			return false;

		//逐个打印不显示擦拭塔
		QString seq = getPrintSequence();
		if (seq.toLower() == "by object")
		{
			return false;
		}

		int colorSize = getModelInsideUseColors(true).size();
		return (colorSize >= 2);
	}

	bool Printer::hasModelInsideUseColors(){
		return  getModelInsideUseColors().size() > 1;
	}

	QList<QVector4D> Printer::getModelInsideUseColors(bool includeSpecificExtruder)
	{
		if (m_tracer == nullptr) return QList<QVector4D>();

		//��������ɫ˳����currentColors�е�˳��Ϊ��
		std::vector<trimesh::vec> allColors = creative_kernel::currentColors();

		QList<int> indexsList = getUsedExtruders(includeSpecificExtruder).toList();
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

	QSet<int> Printer::getUsedExtruders(bool includeSpecificExtruder)
	{
		QSet<int> allIndexs;
		 const WipeTowerGlobalSetting& setting = m_tracer->wipeTowerCurrentSetting();

		for (ModelGroup* group : m_groupsInside)
		{
			if (group->checkSupportSettingState())
			{
				int supportFilament = group->cSupportFilament();
				int supportInterfaceFilament = group->cSupportInterfaceFilament();
				if (supportFilament >= 1)
					allIndexs.insert(supportFilament - 1);
				else
					allIndexs.insert(setting.supportFilamentColorIndex - 1);

				if (supportInterfaceFilament >= 1)
					allIndexs.insert(supportInterfaceFilament - 1);
				else
					allIndexs.insert(setting.supportInterfaceFilamentColorIndex - 1);
			}
		}

		QList<ModelN*> visibleList = currentModelsInsideVisible();
		for (auto m : visibleList)
		{
			RenderDataPtr data = m->renderData();
			if (data)
				allIndexs.unite(data->colorIndexs());
		}

		//加上编辑gcode耗材丝产生的颜色索引信息
		if (includeSpecificExtruder && m_settings && m_settings->hasSpecificExtruder())
		{
			allIndexs.unite(m_settings->specificExtruderIndexes());
		}

		return std::move(allIndexs);
	}

	qtuser_3d::Box3D Printer::getWipeTowerBox(QList<QVector4D>& colors)
	{
		colors = getModelInsideUseColors(true);

		trimesh::dbox3 bbox = modelsInsideBoundingBox();
		float maxH = bbox.size().z;

		std::vector<int> plate_extruders;
		plate_extruders.resize(colors.size());
		/*if ((plate_extruders.size() == 1) && (m_settings->specificExtruderHeights().size() > 0))
			plate_extruders.push_back(1);*/

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
		{
			getKernelUI()->destroyMessage(MessageType::WipeTowerCollision);
			return false;
		}

		bool intersect = false;

		WipeTowerEntity *tower = m_entity->wipeTowerEntity();
		if (tower && tower->isVisible())
		{
			std::vector<trimesh::vec3> towerPath = wipeTowerOutlinePath(true);
			topomesh::contour_2d_t towerContour;
			towerContour.resize(towerPath.size());
			for (size_t i = 0; i < towerPath.size(); ++i)
			{
				towerContour[i][0] = towerPath[i][0];
				towerContour[i][1] = towerPath[i][1];
			}

			box.translate(tower->position() + position());
			QList<ModelGroup*> groups = modelGroupsInside();
			for (auto m : groups)
			{
				if (box.intersects(m->globalSpaceBox()))
				{
					//intersect = true;
					std::vector<trimesh::vec3> rsPath = m->rsPath();
					size_t point_num = rsPath.size();
					if (point_num == 0)
					{
						continue;
					}

					topomesh::contour_2d_t contour;
					contour.resize(point_num);
					for (size_t i = 0; i < point_num; ++i)
					{
						contour[i][0] = rsPath[i][0];
						contour[i][1] = rsPath[i][1];
					}

					intersect = topomesh::contour_is_intersect(towerContour, contour);

					if (intersect)
					{
						break;
					}
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

	QString Printer::slicedPlateName()
	{
		const QList<ModelN*>& models = modelsInside();
		QString res;
		if (models.count() > 0 && m_SlicedPlateName.isEmpty())
		{
			res = models[0]->name();
		}
		else {
			res = m_SlicedPlateName;
		}
		return res;
	}

	void Printer::setSlicedPlateName(const QString& name)
	{
		if (m_SlicedPlateName == name)
			return;
		m_SlicedPlateName = name;
		emit sigSlicedPlateNameChanged();
	}

	bool Printer::isModifySlicePlateName()
	{
		return !m_SlicedPlateName.isEmpty();
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
		// The logic for checking adaptive layer height's errors has been transferred to
		// EnablePrimeTowerDataItem::updateExceededState and SupportStyleDataItem::updateExceededState
		// in the parameterdataitem.h
		return false;
	}

	bool Printer::checkVisiableModelError()
	{
		const QList<ModelGroup*> list = modelGroupsInside();
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

				QList<ModelGroup*> colList = ByObjectEventHandler::collisionCheck(list);
				error = !colList.isEmpty();

				if (!error)
				{
					QList<ModelGroup*> tallList = ByObjectEventHandler::heightCheck(list, creative_kernel::extruderClearanceHeightToRod());
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

	bool Printer::checkBedTypeInvalid() {
		auto* kernel = getKernelSafely();
		if (!kernel) {
			return false;
		}

		auto* parameter_manager = kernel->parameterManager();
		if (!parameter_manager) {
			return false;
		}

		auto bed_type = parameter(QStringLiteral("curr_bed_type"));
		if (bed_type.isEmpty()) {
			bed_type = parameter_manager->currBedType();
		}

		if (bed_type != QStringLiteral("Epoxy Resin Plate")) {
			return false;
		}

		std::set<int> extruder_indexes{};
		for (auto* model : modelsInside()) {
			if (model) {
				auto indexes = model->colorIndexes();
				extruder_indexes.insert(indexes.cbegin(), indexes.cend());
			}
		}

		auto* machine = parameter_manager->currentMachine();
		if (!machine) {
			return false;
		}

		for (auto index : extruder_indexes) {
			auto* extruder = machine->getExtruder(index);
			if (!extruder) {
				continue;
			}

			auto* material = machine->materialAt(extruder->materialIndex());
			if (!material) {
				continue;
			}

			auto* data_model = material->getDataModel();
			if (!data_model) {
				continue;
			}

			for (const auto& key : {
					QStringLiteral("epoxy_resin_plate_temp"),
					QStringLiteral("epoxy_resin_plate_temp_initial_layer") }) {
				if (data_model->findOrMakeDataItem(key)->getValue() == QStringLiteral("0")) {
					return true;
				}
			}
		}

		return false;
	}

	void Printer::adjustWipeTowerPositionWhenInvalid()
	{
		WipeTowerEntity* tower = m_entity->wipeTowerEntity();
		if (tower && tower->isEnabled())
		{
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
	}

	void Printer::onModelGroupModified(ModelGroup* _model_group, const QList<ModelN*>& removes, const QList<ModelN*>& adds)
	{
		if (adds.isEmpty())
			return;

		if (m_groupsInside.contains(_model_group))
		{
			m_modelsInside.clear();
			for (ModelGroup* group : m_groupsInside)
			{
				m_modelsInside += group->models();
			}
		}

		if (m_groupsOnBorder.contains(_model_group))
		{
			 m_modelsOnBorder.clear();
			for (ModelGroup* group : m_groupsInside)
			{
				m_modelsOnBorder += group->models();
			}
		}
	}
	QString Printer::generateSceneName()
	{
		QString name;
		QList<ModelN*> models = modelsInside();
		if (!models.isEmpty()) {
			ModelN* firstModel = models.first();
			name = QFileInfo(firstModel->name()).completeBaseName();
			//name += "-";
		}
		return name;
	}

    void Printer::ResetSettings()
    {
        if (m_settings)
        {
            m_settings->ResetlayersConfig();
        }
    }
}
