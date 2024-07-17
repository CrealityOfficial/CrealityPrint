#ifndef __ADAPTIVELAYERMODE_H
#define __ADAPTIVELAYERMODE_H

#include <QObject>
#include <QPointer>
#include "qtuser3d/module/pickableselecttracer.h"
#include "qtuser3d/module/pickable.h"
#include "operation/moveoperatemode.h"
namespace creative_kernel
{
	class ModelN;
};

class AdaptiveLayerMode : public MoveOperateMode
	, public qtuser_3d::SelectorTracer
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
	void selectChanged(qtuser_3d::Pickable* pickable)override;
private:
	bool popup_visible_;

	QPointer<creative_kernel::ModelN> m_lastSelectModelPt = nullptr;
	//creative_kernel::ModelN* m_lastSelectedModel;
};

#endif // __ADAPTIVELAYERMODE_H