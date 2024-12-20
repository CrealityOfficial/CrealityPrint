#include "modelnselector.h"
#include "qtuser3d/module/pickable.h"
#include "qtuser3d/module/facepicker.h"
#include "qtuser3d/module/pickableselecttracer.h"
#include "qtuser3d/trimesh2/conv.h"
#include "qtusercore/plugin/toolcommandcenter.h"
#include "internal/tool/scalemode.h"
#include "data/modeln.h"
#include "kernel/kernelui.h"
#include "data/modelspace.h"
#include "internal/render/wipetowerpickable.h"
#include "internal/render/platepickable.h"
#include "data/modelgroup.h"
#include "interface/spaceinterface.h"
#include "interface/selectorinterface.h"
#include "interface/camerainterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/printerinterface.h"
#include "interface/spaceinterface.h"
#include "internal/multi_printer/printermanager.h"
#include "data/spaceutils.h"

using namespace qtuser_3d;
namespace creative_kernel
{
	ModelNSelector::ModelNSelector(QObject* parent)
		: Selector(parent)
		, m_model_space(nullptr)
		, m_selected_wipe_tower(nullptr)
	{
		addModelNSelectorTracer(this);
		traceSpace(this);
	}
	
	ModelNSelector::~ModelNSelector()
	{
	}
	
	int ModelNSelector::modelTracersCount()
	{
		return m_selector_tracers.size();
	}

	void ModelNSelector::setModelSpace(ModelSpace* space)
	{
		m_model_space = space;
	}

	void ModelNSelector::addModelNSelectorTracer(ModelNSelectorTracer* tracer)
	{
		if (tracer && !m_selector_tracers.contains(tracer))
		{
			m_selector_tracers.append(tracer);
		}
	}

	void ModelNSelector::removeModelNSelectorTracer(ModelNSelectorTracer* tracer)
	{
		if (tracer && m_selector_tracers.contains(tracer))
		{
			m_selector_tracers.removeOne(tracer);
		}
	}

	void ModelNSelector::modifyModels(const QList<ModelN*>& adds, const QList<ModelN*>& removes)
	{
		bool changed = false;
		setEnabled(false);
		for (ModelN* model : removes) {
			unTracePickable(model);

			if (m_selected_models.contains(model))
			{
				changed = true;
				model->setSelected(false);
				m_selected_models.removeOne(model);
			}
		}

		for (ModelN* model : adds) {
			tracePickable(model);
		}
		setEnabled(true);

		if (changed)
		{
			updateFaceBases();
			_notifyTracers();
		}
	}

	void ModelNSelector::modifyModelGroups(const QList<ModelGroup*>& removes)
	{
		bool changed = false;
		setEnabled(false);
		for (ModelGroup* model_group : removes) {
			if (m_selected_groups.contains(model_group))
			{
				changed = true;
				model_group->setSelected(false);
				m_selected_groups.removeOne(model_group);
			}
		}
		setEnabled(true);

		if (changed)
			_notifyTracers();
	}

	void ModelNSelector::changeModels(const QList<ModelN*>& models)
	{
		for (ModelN* model : models)
		{
			if (model->selected() || model->getModelGroup()->selected())
			{
				_notifyBoxChange();
				return;
			}
		}
	}

	void ModelNSelector::changeModelGroups(const QList<ModelGroup*>& groups)
	{
		for (ModelGroup* _model_group : groups)
		{
			if (_model_group->selected())
			{
				_notifyBoxChange();
				return;
			}
		}
	}

	void ModelNSelector::onSelectedPlateChanged()
	{
		_notifyTracers();
		// requestVisUpdate();
	}

	void ModelNSelector::sizeChanged()
	{
		emit selectedNumChanged();
	}

	QList<ModelN*> ModelNSelector::selectionms()
	{
		QList<ModelN*> _models;
		if (m_selected_groups.size() > 0)
		{
			for (ModelGroup* _model_group : m_selected_groups)
				_models << _model_group->models();
		}
		else {
			_models = m_selected_models;
		}
		return _models;
	}
	
	QList<ModelN*> ModelNSelector::selectionms(ModelNType type)
	{
		QList<ModelN*> _models;
		if (m_selected_groups.size() > 0)
		{
			for (ModelGroup* _model_group : m_selected_groups)
			{
				QList<ModelN*> models = _model_group->models();
				for (ModelN* model : models)
				{
					if (model->modelNType() == type)
						_models << model;
				}
			}
		}
		else {
			for (ModelN* model : m_selected_models)
			{
				if (model->modelNType() == type)
					_models << model;
			}
		}
		return _models;
	}

	bool ModelNSelector::canPasteEnabled()
	{
		bool canPaste = true;
		if (m_onCopyObjectIds.size() <= 0)
			return false;

		bool copyGroupFlag = true;

		int64_t objId = m_onCopyObjectIds[0];
		ModelGroup* mg = m_model_space->getModelGroupByObjectId(objId);
		if (!mg)
		{
			copyGroupFlag = false;
		}

		if (copyGroupFlag)
		{
			QList<ModelGroup*> copyGroups;
			for (auto objid : m_onCopyObjectIds)
			{
				ModelGroup* aGroup = m_model_space->getModelGroupByObjectId(objid);
				if (aGroup == nullptr)
					canPaste = false;
			}

		}
		else // paste parts
		{
			QList<ModelN*> copyModels;
			for (auto mid : m_onCopyObjectIds)
			{
				ModelN* aModel = m_model_space->getModelNByObjectId(mid);
				if (aModel == nullptr)
					canPaste = false;
			}
		}
		return canPaste;
	}
	
	QList<ModelN*> ModelNSelector::selectedParts()
	{	
		return m_selected_models;
	}
	
	QList<ModelN*> ModelNSelector::selectedParts(ModelNType type)
	{
		QList<ModelN*> parts;
		for (ModelN* model : m_selected_models)
		{
			if (model->modelNType() == type)
				parts << model;
		}
		return parts;
	}

	QList<ModelN*> ModelNSelector::unselectionms() {
		QList<ModelN*> selected_model_list = selectionms();
		QList<ModelN*> unselect_model_list;

		for (auto* model : m_model_space->modelns()) {
			if (!selected_model_list.contains(model)) {
				unselect_model_list << model;
			}
		}

		return unselect_model_list;
	}

	bool ModelNSelector::wipeTowerSelected()
	{
		return m_selected_wipe_tower != nullptr;
	}

	int ModelNSelector::selectedNum()
	{
		return selectionms().size();
	}

	int ModelNSelector::selectedObjectsNum()
	{
		return selectedGroups().size() + selectedParts(ModelNType::normal_part).size();
	}
	
	int ModelNSelector::selectedPartsNum()
	{
		return m_selected_models.size();
	}

	int ModelNSelector::selectedGroupsNum()
	{
		return m_selected_groups.size();
	}

	qtuser_3d::Box3D ModelNSelector::selectionsBoundingbox()
	{
		QList<ModelN*> models = selectionms();
		qtuser_3d::Box3D box;
		for (ModelN* model : models)
			box += model->globalSpaceBox();
		return box;
	}

	QVector3D ModelNSelector::selectionmsSize()
	{
		return selectionsBoundingbox().size();
	}

	void ModelNSelector::onSelectionsChanged()
	{
		emit selectedNumChanged();
	}

	void ModelNSelector::onModelGroupModified(ModelGroup* _model_group, const QList<ModelN*>& removes, const QList<ModelN*>& adds)
	{
		//if (adds.size() == 1 && removes.isEmpty())
		//{
		//	selectModelN(adds.first());
		//}
	}

	bool ModelNSelector::pointHover(QHoverEvent* event)
	{
		bool result = hover(event->pos());
		if (result)
			requestVisUpdate();

		return result;
	}

	void ModelNSelector::pointSelect(QMouseEvent* event)
	{
		qtuser_3d::Pickable* pickable = checkPickable(event->pos(), nullptr);
		WipeTowerPickable* wipe_tower = qobject_cast<WipeTowerPickable*>(pickable);
		ModelN* model = qobject_cast<ModelN*>(pickable);
		PlatePickable* plate = qobject_cast<PlatePickable*>(pickable);

		bool changed = false;
		if (wipe_tower)
		{
			if (!wipe_tower->selected())
			{
				if (m_selected_wipe_tower)
					m_selected_wipe_tower->setSelected(true);

				m_selected_wipe_tower = wipe_tower;
				m_selected_wipe_tower->setSelected(true);

				_unselectAllModels();
				changed = true;

				emit wipeTowerSelectedChanged();
			}
		}
		else if (model)
		{
			ModelGroup* model_group = model->getModelGroup();
			assert(model_group);

			bool control = event->modifiers() & Qt::KeyboardModifier::ControlModifier;
			bool alt = event->modifiers() & Qt::KeyboardModifier::AltModifier || 
					   model->modelNType() != ModelNType::normal_part;

			if (!alt)
			{	/* select/unselect groups */
				if (!control)
				{
					if (!m_selected_groups.contains(model_group)
						&& !m_selected_models.contains(model))
					{
						_unselectAllModels();
						_selectGroup(model_group);
						changed = true;
					}

					if (!model->selected())
					{
						changed = true;
					}
				}
				else
				{
					if (m_selected_groups.contains(model_group))
						changed = _unselectGroup(model_group);
					else
						changed = _selectGroup(model_group, true);
				}
			}
			else
			{	/* select/unselect models */
				if (!control)
				{
					if (!m_selected_groups.contains(model_group)
						&& !m_selected_models.contains(model))
					{
						_unselectAllModels();
						_selectModelN(model);
						changed = true;
					}
				}
				else
				{
					if (m_selected_groups.size() == 1 &&
						m_selected_groups.contains(model_group))
					{
						changed = _unselectModelN(model);
					}
					else if (m_selected_groups.isEmpty())
					{
						if (m_selected_models.contains(model))
						{
							changed = _unselectModelN(model);
						}
						else
						{
							bool onSameGroup = true;
							for (ModelN* m : m_selected_models)
							{
								if (m->getModelGroup() != model_group)
								{
									onSameGroup = false;
									break;
								}
							}
							if (onSameGroup)
								changed = _selectModelN(model, true);
						}
					}
				}
			}
		}
		else if (pickable)
		{
			bool control = event->modifiers() & Qt::KeyboardModifier::ControlModifier;
			Selector::select(event->pos(), control);

			if (plate)
			{
				if (m_selected_plate == plate)
				{
					changed = m_selected_models.size() != 0 || m_selected_groups.size() != 0;
				}
				else 
				{
					changed = true;
					m_selected_plate = plate;
				}
				_unselectAllModels();
			}
		}
		else  {
			_unselectAllModels();
			changed = true;
		}

		if(changed)
		{
			//test
			for (ModelGroup* model_group : m_selected_groups)
			{
				for (ModelN* _model : model_group->models())
				{
					_model->setSelected(true);
				}
			}
			for (ModelN* _model : m_selected_models)
				_model->setSelected(true);

			_notifyTracers();
			requestVisUpdate();
		}
	}

	void ModelNSelector::areaSelect(const QRect& rect, QMouseEvent* event)
	{
		if (rect.size().isNull()) return;

		int width = abs(rect.width()), height = abs(rect.height());
		if (width <= 1 || height <= 1) return;

		int x = std::min(rect.left(), rect.right()), y = std::min(rect.top(), rect.bottom());

		QList<ModelGroup*> rect_groups;
		for (size_t i = 0; i < height; i++)
		{
			for (size_t j = 0; j < width; j++)
			{
				QPoint p(x + j, y + i);
				ModelN* model = checkPickModel(p);
				if (model && !rect_groups.contains(model->getModelGroup()))
				{
					rect_groups << model->getModelGroup();
				}
			}
		}

		bool changed = false;
		for (ModelGroup* model_group : rect_groups)
		{
			if (!model_group->selected())
			{
				changed = true;
				break;
			}
		}

		_check(m_selected_models, false);
		m_selected_models.clear();
		_check(m_selected_groups, false);
		m_selected_groups.clear();
		m_selected_groups = rect_groups;
		_check(m_selected_groups, true);

		if (changed)
		{
			_notifyTracers();
			requestVisUpdate();
		}
	}

	void ModelNSelector::ctrlASelectAll()
	{
		//if (m_selected_models.size() > 0)
		//{
		//	selectAllModels();
		//	return;
		//}

		selectAllModelGroup();
	}

	void ModelNSelector::selectGroup(ModelGroup* group, bool append)
	{
		if (group->selected())
		{
			return;
		}

		bool changed = _selectGroup(group, append);
		if (changed)
		{
			_notifyTracers();
			requestVisUpdate();
		}
	}

	void ModelNSelector::selectGroups(const QList<ModelGroup*>& groups)
	{
		if (groups.isEmpty())
			return;

		_check(m_selected_models, false);
		m_selected_models.clear();
		_check(m_selected_groups, false);
		m_selected_groups.clear();

		for (ModelGroup* group : groups)
			_selectGroup(group, true);

		for (ModelGroup* model_group : m_selected_groups)
		{
			for (ModelN* _model : model_group->models())
			{
				_model->setSelected(true);
			}
		}

		_notifyTracers();
		requestVisUpdate();
	}

	void ModelNSelector::unselectGroups(const QList<ModelGroup*>& groups)
	{
		bool changed = false;
		for (ModelGroup* group : groups)
		{
			if (!group->selected())
			{
				return;
			}
			if (_unselectGroup(group))
				changed = true;
		}
		if (changed)
		{
			_notifyTracers();
			requestVisUpdate();
		}
	}

	void ModelNSelector::unselectModels(const QList<ModelN*>& models)
	{
		bool changed = false;
		for (ModelN* model : models)
		{
			if (_unselectModelN(model))
				changed = true;
		}
		if (changed)
		{
			_notifyTracers();
			requestVisUpdate();
		}
	}

	void ModelNSelector::selectModelN(ModelN* model, bool append, bool force)
	{
		if (force)
			_unselectAllModels();

		bool changed = _selectModelN(model, append);
		if (changed)
		{
			_notifyTracers();
			requestVisUpdate();
		}
	}

	void ModelNSelector::selectAllModelGroup()
	{
		bool changed = (m_selected_models.size() > 0);
		_check(m_selected_models, false);
		m_selected_models.clear();

		QList<ModelGroup*> u_model_groups = unselectedGroups();
		changed = changed | (u_model_groups.size() > 0);

		if (u_model_groups.size() > 0)
		{
			m_selected_groups << u_model_groups;
			_check(u_model_groups, true);
		}

		if (changed)
		{
			_notifyTracers();
			requestVisUpdate();
		}
	}

	void ModelNSelector::selectAllModels()
	{
		assert(m_selected_groups.size() == 0);
		int size = m_selected_models.size();
		if (size > 0)
		{
			ModelGroup* model_group = m_selected_groups.at(0);
			for (int i = 1; i < size; ++i)
			{
				assert(model_group == m_selected_models.at(i)->getModelGroup());
			}

			QList<ModelN*> unselected_models = model_group->selectedModels();
			if (unselected_models.size() > 0)
			{
				_check(unselected_models, true);
				m_selected_models << unselected_models;

				_notifyTracers();
				requestVisUpdate();
			}
		}
	}

	void ModelNSelector::unselectAll()
	{
		bool change = false;
		if (m_selected_groups.size() > 0
			|| m_selected_models.size() > 0)
			change = true;

		_check(m_selected_models, false);
		m_selected_models.clear();
		_check(m_selected_groups, false);
		m_selected_groups.clear();

		if (change)
		{
			_notifyTracers();
			requestVisUpdate();
		}
	}

	Printer* ModelNSelector::selectedPlate()
	{
		return getSelectedPrinter();
	}

	QList<ModelGroup*> ModelNSelector::selectedGroups()
	{
		return m_selected_groups;
	}

	QList<ModelGroup*> ModelNSelector::unselectedGroups()
	{
		QList<ModelGroup*> model_groups;
		for (ModelGroup* model_group : m_model_space->modelGroups())
		{
			if (!model_group->selected())
				model_groups << model_group;
		}
		return model_groups;
	}

	QList<QList<ModelN*>> ModelNSelector::selectedModelnsList()
	{
		QList<QList<ModelN*>> list;

		if (m_selected_groups.size() > 0)
		{
			for (ModelGroup* group : m_selected_groups)
			{
				list << group->models();
			}
		}
		else {
		
			list << selectionms();
		}

		return list;
	}

	ModelN* ModelNSelector::checkPickModel(const QPoint& point)
	{
		qtuser_3d::Pickable* pickable = checkPickable(point, nullptr);
		ModelN* model = qobject_cast<ModelN*>(pickable);
		return model;
	}

	ModelN* ModelNSelector::checkPickModel(const QPoint& point, QVector3D& position, QVector3D& normal, int* faceID)
	{
		int primitiveID = -1;
		Pickable* pickable = checkPickable(point, &primitiveID);
		ModelN* model = qobject_cast<ModelN*>(pickable);
		if (model)
		{
			primitiveID = model->getFacet2Facets(primitiveID);
			model->rayCheck(primitiveID, visRay(point), position, &normal, false);

			if (model->getFanZhuanState()) 
			{
				normal = -normal;
			}

		}
		if (faceID)
		{
			*faceID = primitiveID;
		}
		return model;
	}

	ModelN* ModelNSelector::checkPickModelEx(const QPoint& point, QVector3D& position, QVector3D& normal, int* faceID)
	{
		int primitiveID = -1;
		Pickable* pickable = checkPickable(point, &primitiveID);
		ModelN* model = qobject_cast<ModelN*>(pickable);
		if (model)
		{
			primitiveID = model->getFacet2Facets(primitiveID);
			model->rayCheckEx(primitiveID, visRay(point), position, normal);
		}
		if (faceID)
		{
			*faceID = primitiveID;
		}
		return model;
	}

	void ModelNSelector::removeSelections()
	{
		if (m_selected_groups.size() > 0)
			m_model_space->deleteModelGroups(m_selected_groups);
		else if (m_selected_models.size() > 0)
			m_model_space->deleteModels(m_selected_models);
	}

	void ModelNSelector::mirrorXSelections(bool reversible)
	{
		QList<ModelGroup*> _model_groups = selectedGroups();
		QList<ModelN*> list = selectedParts();
		
		beginNodeSnap(_model_groups, list);
		
		if (!_model_groups.isEmpty())
		{
			mirrorX(_model_groups, reversible);
			updateSpaceNodeRender(_model_groups, true);
			for (auto &g : _model_groups)
			{
				g->setOutlinePathDirty();
				g->updateSweepAreaPath();
			}
		}
		else if (!list.isEmpty())
		{	
			mirrorX(list, reversible);
			updateSpaceNodeRender(list, true);

			auto groups = creative_kernel::findRelativeGroups(list);
			for (auto& g : groups)
			{
				g->setOutlinePathDirty();
				g->updateSweepAreaPath();
			}
		}

		endNodeSnap();
	}

	void ModelNSelector::mirrorYSelections(bool reversible)
	{
		QList<ModelGroup*> _model_groups = selectedGroups();
		QList<ModelN*> list = selectedParts();

		beginNodeSnap(_model_groups, list);

		if (!_model_groups.isEmpty())
		{
			mirrorY(_model_groups, reversible);
			updateSpaceNodeRender(_model_groups, true);
			for (auto& g : _model_groups)
			{
				g->setOutlinePathDirty();
				g->updateSweepAreaPath();
			}
		}
		else if (!list.isEmpty())
		{	
			mirrorY(list, reversible);
			updateSpaceNodeRender(list, true);
			auto groups = creative_kernel::findRelativeGroups(list);
			for (auto& g : groups)
			{
				g->setOutlinePathDirty();
				g->updateSweepAreaPath();
			}
		}
		endNodeSnap();
	}

	void ModelNSelector::mirrorZSelections(bool reversible)
	{
		QList<ModelGroup*> _model_groups = selectedGroups();
		QList<ModelN*> list = selectedParts();

		beginNodeSnap(_model_groups, list);
		
		if (!_model_groups.isEmpty())
		{
			mirrorZ(_model_groups, reversible);
			updateSpaceNodeRender(_model_groups, true);
			for (auto& g : _model_groups)
			{
				g->setOutlinePathDirty();
				g->updateSweepAreaPath();
			}
		}
		else if (!list.isEmpty())
		{
			mirrorZ(list, reversible);
			updateSpaceNodeRender(list, true);
			auto groups = creative_kernel::findRelativeGroups(list);
			for (auto& g : groups)
			{
				g->setOutlinePathDirty();
				g->updateSweepAreaPath();
			}
		}
		endNodeSnap();
	}

	void ModelNSelector::cloneSelections(int num)
	{
		if (m_selected_groups.size() > 0)
			m_model_space->cloneGroups(m_selected_groups, num);
		else if (m_selected_models.size() > 0)
			m_model_space->cloneModels(m_selected_models, num);
	}

	void ModelNSelector::onCopySelections()
	{
		m_onCopyObjectIds.clear();
		if (m_selected_groups.size() > 0)
		{
			for (auto mg : m_selected_groups)
			{
				m_onCopyObjectIds.append(mg->getObjectId());
			}
		}
		else if(m_selected_models.size() > 0)
		{
			for (auto m : m_selected_models)
			{
				m_onCopyObjectIds.append(m->getObjectId());
			}
		}
	}

	void ModelNSelector::onPasteSelections()
	{
		if (m_onCopyObjectIds.size() <= 0)
			return;

		bool copyGroupFlag = true;

		// can not pick group and part at the same time
		int64_t objId = m_onCopyObjectIds[0];
		ModelGroup* mg = m_model_space->getModelGroupByObjectId(objId);
		if (!mg)
		{
			copyGroupFlag = false;
		}

		if (copyGroupFlag)
		{
			QList<ModelGroup*> copyGroups;
			for (auto objid : m_onCopyObjectIds)
			{
				ModelGroup* aGroup = m_model_space->getModelGroupByObjectId(objid);
				if (aGroup)
					copyGroups.append(aGroup);
			}

			m_model_space->cloneGroups(copyGroups, 1);

		}
		else // paste parts
		{
			QList<ModelN*> copyModels;
			for (auto mid : m_onCopyObjectIds)
			{
				ModelN* aModel = m_model_space->getModelNByObjectId(mid);
				if (aModel)
					copyModels.append(aModel);
			}

			m_model_space->cloneModels(copyModels, 1);
		}

	}

	void ModelNSelector::mergeSelectionsGroup()
	{
		QList<ModelGroup*> groups = selectedGroups();
		m_model_space->mergeGroups(groups);
	}

	void ModelNSelector::splitSelectionsGroup2Objects()
	{
		QList<ModelGroup*> groups = selectedGroups();
		if (groups.size() == 1)
		{
			m_model_space->splitGroup2Objects(groups.first());
		}
	}

	void ModelNSelector::splitSelectionsModels2Parts()
	{
		QList<ModelN*> models = selectionms();
		assert(models.size() == 1);
		m_model_space->splitModel2Parts(models.first());
	}

	QList<QObject*> ModelNSelector::selectionmObjects()
	{
		QList<QObject*> objects;
		for (ModelGroup* group : m_selected_groups)
		{
			objects << group;
		}
		return objects;
	}
	
	QList<QObject*> ModelNSelector::selectedModelObjects()
	{
		QList<QObject*> objects;
		QList<ModelN*> selectedModels = selectionms();
		for (ModelN* model : selectedModels)
			objects << model;

		return objects;
	}

	QList<QObject*> ModelNSelector::selectedModelGroupObjects()
	{
		QList<QObject*> objects;
		QList<ModelGroup*> selectedModelGroups = selectedGroups();
		for (ModelGroup* group : selectedModelGroups)
			objects << group;

		return objects;
	}

	bool ModelNSelector::_selectGroup(ModelGroup* group, bool append)
	{
		if (!group)
			return false;

		if (m_selected_models.size() > 0 && append)
		{
			//message
			return false;
		}

		if (append)
		{
			if (m_selected_groups.contains(group))
			{
				return false;
			}
		}
		else {
			_check(m_selected_groups, false);
			m_selected_groups.clear();
		}

		_unselectWipeTower();
		group->setSelected(true);
		m_selected_groups.append(group);
		return true;
	}

	bool ModelNSelector::_unselectGroup(ModelGroup* group)
	{
		if (!group)
			return false;

		if (m_selected_groups.contains(group))
		{
			QList<ModelGroup*> groups;
			groups << group;
			_check(groups, false);

			m_selected_groups.removeOne(group);
			return true;
		}
		return false;
	}

	bool ModelNSelector::_selectModelN(ModelN* model, bool append)
	{
		if (!model)
			return false;

		if (m_selected_groups.size() > 0)
		{
			if (append)
				return false;
			else
			{
				_check(m_selected_groups, false);
				m_selected_groups.clear();
			}
		}

		if (append)
		{
			if (m_selected_models.contains(model))
			{
				return false;
			}
		}
		else {
			ModelGroup* model_group = model->getModelGroup();
			for (ModelN* selected_model : m_selected_models)
			{
				if (selected_model->getModelGroup() != model_group)
				{
					//message
					return false;
				}
			}

			_check(m_selected_models, false);
			m_selected_models.clear();
		}

		_unselectWipeTower();
		m_selected_models.append(model);
		return true;
	}

	bool ModelNSelector::_unselectModelN(ModelN* model)
	{
		if (!model)
			return false;

		ModelGroup* model_group = model->getModelGroup();
		if (m_selected_groups.contains(model_group))
		{
			_unselectGroup(model_group);
			for (ModelN* m : model_group->models())
			{
				if (m != model)
				{
					m_selected_models.append(m);
				}
			}
		}
		else 
		{
			m_selected_models.removeOne(model);
		}

		return true;
	}

	void ModelNSelector::_unselectWipeTower()
	{	
		if (!m_selected_wipe_tower)
			return;

		m_selected_wipe_tower->setSelected(false);
		m_selected_wipe_tower = NULL;
	}

	void ModelNSelector::_unselectAllModels()
	{
		_check(m_selected_models, false);
		m_selected_models.clear();
		_check(m_selected_groups, false);
		m_selected_groups.clear();
	}

	void ModelNSelector::_check(QList<ModelGroup*>& groups, bool check)
	{
		for (ModelGroup* group : groups)
		{
			group->setSelected(check);
			for (ModelN* model : group->models())
				model->setSelected(check);
		}
	}

	void ModelNSelector::_check(QList<ModelN*>& models, bool check)
	{
		for (ModelN* model : models)
			model->setSelected(check);
	}

	void ModelNSelector::_notifyTracers()
	{
		emit selectedNumChanged();
		for (ModelNSelectorTracer* tracer : m_selector_tracers)
			tracer->onSelectionsChanged();
	}

	void ModelNSelector::_notifyBoxChange()
	{
		for (ModelNSelectorTracer* tracer : m_selector_tracers)
			tracer->onSelectionsBoxChanged();
	}

	void ModelNSelector::cloneSelectedModels(int num)
	{
		m_model_space->cloneModels(selectionms(), num);
	}
}