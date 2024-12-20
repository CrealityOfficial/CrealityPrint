#ifndef __ADAPTIVELAYERMODE_H
#define __ADAPTIVELAYERMODE_H

#include <QObject>
#include <QPointer>
#include "qtuser3d/module/pickable.h"
#include "operation/moveoperatemode.h"

namespace creative_kernel
{
	class ModelN;
};

class AdaptiveLayerMode : public creative_kernel::MoveOperateMode
	, public creative_kernel::ModelNSelectorTracer
{
	Q_OBJECT
public:
	explicit AdaptiveLayerMode(QObject* parent = nullptr);
	virtual ~AdaptiveLayerMode();
	bool isPopupVisible();

	void installEventHandlers();

	void uninstallEventHandlers();


	void adapted(const float percentage);
	void smooth(const int radiusVal,bool keepMin);
	void reset();

	void restore(creative_kernel::ModelN* model, const std::vector<double>& layersData);
	void stackUndo(creative_kernel::ModelN* model, const std::vector<double>& oldLayerData, const std::vector<double>& newLayerData);
/* override */
protected:
	virtual void onAttach() override;
	virtual void onDettach() override;

	void onLeftMouseButtonPress(QMouseEvent* event) override;
	void onLeftMouseButtonRelease(QMouseEvent* event) override;
	void onLeftMouseButtonClick(QMouseEvent* event) override;
	void onLeftMouseButtonMove(QMouseEvent* event)override;

	void onSelectionsChanged()override;

protected:
	creative_kernel::ModelGroup* getSelectedGroup();
	void attachToNewGroup();
	void attachLayerHeightProfileTo(creative_kernel::ModelGroup* target, const std::vector<double>& layers, const std::vector<TriMeshPtr>& meshes);
	void dettachLayerHeightProfileTo(creative_kernel::ModelGroup* target);

	void setLayerHeightProfileTo(creative_kernel::ModelGroup* target, const std::vector<double>& layers, const std::vector<TriMeshPtr>& meshes);
	QImage generateLayerHeightImage(const std::vector<double>& layers, float minLayerHeight, float maxLayerHeight, float uniqueLayerHeight);


private:
	bool popup_visible_;
	QPointer <creative_kernel::ModelGroup> m_lastSelectedGroup = nullptr;
	std::vector<TriMeshPtr> m_groupMeshes;
};

#endif // __ADAPTIVELAYERMODE_H