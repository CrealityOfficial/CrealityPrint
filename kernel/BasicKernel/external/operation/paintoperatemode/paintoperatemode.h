#ifndef _NULLSPACE_PAINT_OPERATE_MODE_1591235079966_H
#define _NULLSPACE_PAINT_OPERATE_MODE_1591235079966_H

#include <QVector3D>

#include "trimesh2/TriMesh.h"

#include "qtuser3d/scene/sceneoperatemode.h"
#include "qtuser3d/refactor/xentity.h"
#include "data/modeln.h"
#include "qtuser3d/module/pickableselecttracer.h"
#include "basickernelexport.h"

namespace qtuser_3d
{
	class OutlineEntity;
	class XEffect;
};

namespace creative_kernel
{
	class ModelN;
};

namespace spread
{
	class MeshSpreadWrapper;
	struct SceneData;
};

class ColorExecutor;
class ColorLoadJob;
class SpreadModel;

class BASIC_KERNEL_API PaintOperateMode : public qtuser_3d::SceneOperateMode, public qtuser_3d::SelectorTracer
{
	Q_OBJECT;
signals: 
	void posChanged();
	void scaleChanged();
	void sectionRateChanged();
	void colorRadiusChanged();
	void highlightOverhangsDegChanged();
	void overHangsEnableChanged();
	void enter();
	void quit();

public:
	explicit PaintOperateMode(QObject* parent = nullptr);
	virtual ~PaintOperateMode();

	bool isRunning();
	virtual void restore(creative_kernel::ModelN* model, const std::vector<std::string>& data);

protected:
	std::vector<std::string> getPaintData() const;
	bool isHoverModel(const QPoint& pos);
	void setWaitForLoadModel(bool enable);

private:
	bool m_isRunning { false };

/* 界面参数 */
protected:
	//read-only
	QPoint m_pos;
	float m_scale;
	bool m_pressed { false };
	bool m_erase{ false };
	bool m_waitForLoadModel{ false };

  	//read-write
	float m_sectionRate { 1.0 };	//横截面截取比率
	float m_colorRadius { 1.0 };		//涂色半径
	int m_colorMethod { 0 };
	int m_colorIndex { 1 };		//颜色索引
	std::vector<trimesh::vec> m_colorsList;
	float m_highlightOverhangsDeg { 0.0 };	//悬空角度
	bool m_overHangsEnable { false }; 

public:
	//read-only
	QPoint pos();
	float scale();
	bool pressed();

	//read-write
	float sectionRate();
	void setSectionRate(float rate);
	float colorRadius();
	void setColorRadius(float radius);
	int colorMethod();
	void setColorMethod(int method);
	int colorIndex();
	void setColorIndex(int index);
	QVariantList colorsList();
	void setColorsList(const QVariantList& datasList);
	void setColorsList(const std::vector<trimesh::vec>& datasList);
	std::vector<trimesh::vec> getColorsList();
	float highlightOverhangsDeg();
	void setHighlightOverhangsDeg(float deg);
	bool overHangsEnable();
	void setOverHangsEnable(bool enable);

private slots:
  	void resize();

/* 剖面功能 */
protected:
	QVector3D m_frontPos;
	QVector3D m_backPos;
	QVector3D m_sectionPos;
	QVector3D m_sectionNormal;	//剖面法线
	float m_offset { 0 };

public:
	void resetDirection();
	void updateSectionBoundary();
private slots:
	void updateSection();

/* 渲染 */
public:
	void clearAllColor();
	void setEnabled(bool enabled);
	void updateScene();

protected:
	std::unique_ptr<spread::MeshSpreadWrapper> m_meshSpreadWrapper;
  	ColorExecutor* m_colorExecutor;
	ColorLoadJob* m_colorLoadJob {NULL};
	creative_kernel::ModelN* m_originModel{NULL};
	creative_kernel::ModelN* m_colorModel{NULL};	//涂色时用的模型
	SpreadModel* m_spreadModel;
	QVector<qtuser_3d::OutlineEntity*> m_outlineEntitys;
	std::vector<std::string> m_lastSpreadData;
	bool m_isWrapperReady{ true };
	bool m_enabled { true };

private:
	void resetRouteNum(int number);	
	void updateRoute(const std::vector<trimesh::vec3>& route);
	void updateRoute(const std::vector<std::vector<trimesh::vec3>>& routes);
	void hideRoute();

  	void choose(const QPoint& pos);
	void chooseFillArea();			//选中填充区域
	void chooseTriangle();			//选中三角形

  	QVector3D getTriangleNormal(const QVector3D& v1, const QVector3D& v2, const QVector3D& v3) const;
	QVector3D mapToSectionArea(const QVector3D& position, const QVector3D& direction);
	bool getSceneData(const QPoint& pos, spread::SceneData& data);
	void color(const QPoint &pos, int colorIndex, bool isFirstColor);	//上色
	void updateSpreadModel(int chunkId = -1);

private slots:
  	void onColorsListChanged();
	void onGotNewData(std::vector<int> dirtyChunks);

/* override */
protected:
	virtual void installEventHandlers();
	virtual void uninstallEventHandlers();
	virtual void onAttach() override;
	virtual void onDettach() override;

	virtual void onHoverMove(QHoverEvent* event) override;
	virtual	void onLeftMouseButtonMove(QMouseEvent* event) override;
	virtual void onLeftMouseButtonPress(QMouseEvent* event) override;
	virtual void onLeftMouseButtonRelease(QMouseEvent* event) override;
	virtual	void onRightMouseButtonMove(QMouseEvent* event) override;
	virtual	void onMidMouseButtonMove(QMouseEvent* event) override;
	virtual	void onWheelEvent(QWheelEvent* event) override;

	// SelectorTracer
	virtual void onSelectionsChanged() override; 
	virtual void selectChanged(qtuser_3d::Pickable* pickable) {}

};

#endif // _NULLSPACE_PAINT_OPERATE_MODE_1591235079966_H