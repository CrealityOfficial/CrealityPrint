#ifndef CREATIVE_KERNEL_MODELNSELECTOR_1595659762117_H
#define CREATIVE_KERNEL_MODELNSELECTOR_1595659762117_H
#include "basickernelexport.h"
#include "qtuser3d/module/selector.h"
#include "data/interface.h"
#include <QtGui/qevent.h>

namespace creative_kernel
{
	class ModelN;
	class ModelGroup;
	class ModelSpace;
	class WipeTowerPickable;
	class PlatePickable;
	class Printer;
	class BASIC_KERNEL_API ModelNSelector : public qtuser_3d::Selector
		, public ModelNSelectorTracer, public SpaceTracer
	{
		Q_OBJECT
		Q_PROPERTY(int selectedNum READ selectedNum NOTIFY selectedNumChanged)
		Q_PROPERTY(int selectedObjectsNum READ selectedObjectsNum NOTIFY selectedNumChanged)
		Q_PROPERTY(int selectedPartsNum READ selectedPartsNum NOTIFY selectedNumChanged)
		Q_PROPERTY(int selectedGroupsNum READ selectedGroupsNum NOTIFY selectedNumChanged)
		Q_PROPERTY(QList<QObject*> selectionmObjects READ selectionmObjects)
		Q_PROPERTY(QList<QObject*> selectedModelObjects READ selectedModelObjects NOTIFY selectedNumChanged)
		Q_PROPERTY(QList<QObject*> selectedModelGroupObjects READ selectedModelGroupObjects NOTIFY selectedNumChanged)
		Q_PROPERTY(bool wipeTowerSelected READ wipeTowerSelected NOTIFY wipeTowerSelectedChanged)
		friend class ModelSpace;
	public:
		ModelNSelector(QObject* parent = nullptr);
		virtual ~ModelNSelector();

		int modelTracersCount();

		void setModelSpace(ModelSpace* space);

		void addModelNSelectorTracer(ModelNSelectorTracer* tracer);
		void removeModelNSelectorTracer(ModelNSelectorTracer* tracer);

		Q_INVOKABLE void sizeChanged();
		QList<ModelN*> selectionms();
		QList<ModelN*> selectionms(ModelNType type);
		QList<ModelN*> selectedParts();
		QList<ModelN*> selectedParts(ModelNType type);
		QList<ModelN*> unselectionms();
		bool canPasteEnabled();
		
		int selectedNum();
		int selectedObjectsNum();
		int selectedPartsNum();
		int selectedGroupsNum();

		qtuser_3d::Box3D selectionsBoundingbox();
		Q_INVOKABLE QVector3D selectionmsSize();

		bool wipeTowerSelected();

		bool pointHover(QHoverEvent* event);
		void pointSelect(QMouseEvent* event);
		void areaSelect(const QRect& rect, QMouseEvent* event);
		void ctrlASelectAll();
		void selectGroup(ModelGroup* group, bool append = false);
		void selectGroups(const QList<ModelGroup*>& groups);
		void selectModelN(ModelN* model, bool append = false, bool force = false);
		void unselectGroups(const QList<ModelGroup*>& groups);
		void unselectModels(const QList<ModelN*>& models);
		void selectAllModelGroup();
		void selectAllModels();
		void unselectAll();

		Printer* selectedPlate();
		QList<ModelGroup*> selectedGroups();
		QList<ModelGroup*> unselectedGroups();
		QList<QList<ModelN*>> selectedModelnsList();
		ModelN* checkPickModel(const QPoint& point, QVector3D& position, QVector3D& normal, int* faceID);
		ModelN* checkPickModel(const QPoint& point);

		// the normal is returned after applyed by the (normal matrix)
		ModelN* checkPickModelEx(const QPoint& point, QVector3D& position, QVector3D& normal, int* faceID);

		void removeSelections();

		void mirrorXSelections(bool reversible = false);
		void mirrorYSelections(bool reversible = false);
		void mirrorZSelections(bool reversible = false);

		// clone selected Groups
		void cloneSelections(int num);

		void onCopySelections();
		void onPasteSelections();

		void mergeSelectionsGroup();
		void splitSelectionsGroup2Objects();
		void splitSelectionsModels2Parts();
	private:
		QList<QObject*> selectionmObjects();
		QList<QObject*> selectedModelObjects();
		QList<QObject*> selectedModelGroupObjects();

	protected:
		void onSelectionsChanged() override;
		void onModelGroupModified(ModelGroup* _model_group, const QList<ModelN*>& removes, const QList<ModelN*>& adds) override;

		bool _selectGroup(ModelGroup* group, bool append = false);
		bool _unselectGroup(ModelGroup* group);
		bool _selectModelN(ModelN* model, bool append = false);
		bool _unselectModelN(ModelN* model);
		void _unselectWipeTower();
		void _unselectAllModels();
		void _notifyTracers();
		void _notifyBoxChange();

		void _check(QList<ModelGroup*>& groups, bool check);
		void _check(QList<ModelN*>& models, bool check);

		//clone selected models, the selected models should be in the same group
		void cloneSelectedModels(int num);

		void modifyModels(const QList<ModelN*>& adds, const QList<ModelN*>& removes);
		void modifyModelGroups(const QList<ModelGroup*>& removes);
		void changeModels(const QList<ModelN*>& models);
		void changeModelGroups(const QList<ModelGroup*>& groups);

	public slots:
		void onSelectedPlateChanged();

	signals:
		void selectedNumChanged();
		void wipeTowerSelectedChanged();
		void selectedPlateChanged();

	protected:
		ModelSpace* m_model_space;
		QList<ModelNSelectorTracer*> m_selector_tracers;

		PlatePickable* m_selected_plate { NULL };
		WipeTowerPickable* m_selected_wipe_tower;
		QList<ModelGroup*> m_selected_groups;
		QList<ModelN*> m_selected_models;
		QList<int64_t> m_onCopyObjectIds;
	};
}
#endif // CREATIVE_KERNEL_MODELNSELECTOR_1595659762117_H