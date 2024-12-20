#ifndef _NULLSPACE_PAINT_OPERATE_MODE_1591235079966_H
#define _NULLSPACE_PAINT_OPERATE_MODE_1591235079966_H
#include "basickernelexport.h"
#include <QVector3D>

#include "data/interface.h"
#include "qtuser3d/scene/sceneoperatemode.h"
#include "qtuser3d/refactor/xentity.h"
#include "data/modeln.h"

namespace qtuser_3d
{
	class AlonePointEntity;
	class PureEntity;
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

class BASIC_KERNEL_API PaintOperateMode : public qtuser_3d::SceneOperateMode
	, public creative_kernel::ModelNSelectorTracer
	, public creative_kernel::SpaceTracer
{
	Q_OBJECT;
signals: 
	void posChanged();
	void scaleChanged();
	void sectionRateChanged();
	void colorRadiusChanged();
	void colorMethodChanged();
	void colorIndexChanged();
	void screenRadiusChanged();
	void highlightOverhangsDegChanged();
	void overHangsEnableChanged();
	void smartFillAngleChanged();
	void smartFillEnableChanged();	
	void heightRangeChanged();
	void gapAreaChanged();
	void sphereSizeChanged();
	void enter();
	void quit();

public:
	explicit PaintOperateMode(QObject* parent = nullptr);
	virtual ~PaintOperateMode();

	bool routeFlag{ false };
	bool isRunning();
	virtual void restore(int objectID, const QList < std::vector<std::string>>& data);

protected:
	std::vector<std::string> getPaintData(int index) const;
	bool isHoverModel(const QPoint& pos);
	void setWaitForLoadModel(bool enable);

protected:
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
	float m_screenRadius;
	int m_colorMethod { 1 };
	int m_colorIndex { 1 };		//颜色索引
	std::vector<trimesh::vec> m_colorsList;
	float m_highlightOverhangsDeg { 0.0 };	//悬空角度
	bool m_overHangsEnable { false }; 
	float m_smartFillAngle { 30.0 };
	bool m_smartFillEnable { true };
	float m_heightRange { 0.1f };
	float m_gapArea { 0.0f };
	float m_sphereSize { 1.0f }; // 球大小

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

	float screenRadius();

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

	float smartFillAngle();
	void setSmartFillAngle(float angle);

	bool smartFillEnable();
	void setSmartFillEnable(bool enable);

	float heightRange();
	void setHeightRange(float angle);
	
	float gapArea();
	void setGapArea(float angle);

	float sphereSize();
	void setSphereSize(float size);

private:
	float radiusMapToScreen(QVector3D pos);
	void updateSphereColor();

private slots:
  	void resize();

/* 剖面功能 */
protected:
	QVector3D m_frontPos;
	QVector3D m_backPos;
	QVector3D m_sectionPos;
	QVector3D m_sectionNormal;	//剖面法线
	float m_offset { 0 };
	QVector3D m_normal;
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
	QList<spread::MeshSpreadWrapper*> m_meshSpreadWrappers;
  	ColorExecutor* m_colorExecutor;
	ColorLoadJob* m_colorLoadJob {NULL};
	QList<creative_kernel::ModelN*> m_originModels;
	creative_kernel::ModelGroup* m_modelGroup {NULL};
	QList<SpreadModel*> m_spreadModels;
	QVector<qtuser_3d::OutlineEntity*> m_outlineEntitys;
	QList<std::vector<std::string>> m_lastSpreadDatas;
	qtuser_3d::PureEntity* m_sphereEntity;
	//qtuser_3d::AlonePointEntity* m_sphereEntity;
	QList<int> m_visibleTypes;
	QMap<creative_kernel::ModelN*, bool> m_modelsVisibleStateMap;

	bool m_isWrapperReady{ true };
	bool m_enabled { true };
	bool m_hasGap { false };
	bool m_hoverOnModel { false };
	int m_hoveredIndex { -1 };

protected:
	virtual void handleColorsListChanged(const std::vector<trimesh::vec>& newColors);

private:
	/* combind adapt */
	void wrappersUpdateData();
	void wrappersUnselect();
protected:
	int groupSize();

private:
	void resetRouteNum(int number);	
	void updateRoute(const std::vector<trimesh::vec3>& route);
	void updateRoute(const std::vector<std::vector<trimesh::vec3>>& routes);
	void hideRoute();

  	void choose(const QPoint& pos);
	void chooseFillArea();			//选中填充区域
	void chooseTriangle();			//选中三角形
	void chooseCircle();
	void chooseSphere();
	void chooseHeightRange();
	void chooseGapFillArea();

  	QVector3D getTriangleNormal(const QVector3D& v1, const QVector3D& v2, const QVector3D& v3) const;
	QVector3D mapToSectionArea(const QVector3D& position, const QVector3D& direction);
	int getSceneData(const QPoint& pos, spread::SceneData& data);
	void color(const QPoint &pos, int colorIndex, bool isFirstColor);	//上色
	void updateSpreadModel();

protected slots:
  	void onColorsListChanged();
private slots:
	void onGotNewData(void* wrapperObj, std::vector<int> dirtyChunks);
	void onTriggerGapFill();

/* override */
protected:
	void refreshRender();
	void setDefaultFlag(const QList<int>& flags);
	bool paintDataEqual(std::vector<std::string> data1, std::vector<std::string> data2);
	void initValidModels();
	void recordModelsState();
	void showValidModels();
	void resetValidModels();
	virtual void installEventHandlers();
	virtual void uninstallEventHandlers();
	virtual void onAttach() override;
	virtual void onDettach() override;

	virtual void onHoverMove(QHoverEvent* event) override;
	virtual	void onLeftMouseButtonClick(QMouseEvent* event) override;
	virtual	void onLeftMouseButtonMove(QMouseEvent* event) override;
	virtual void onLeftMouseButtonPress(QMouseEvent* event) override;
	virtual void onLeftMouseButtonRelease(QMouseEvent* event) override;
	virtual	void onRightMouseButtonMove(QMouseEvent* event) override;
	virtual	void onMidMouseButtonMove(QMouseEvent* event) override;
	virtual	void onWheelEvent(QWheelEvent* event) override;

	// SelectorTracer
	virtual void onSelectionsBoxChanged() override;
	virtual void onSelectionsChanged() override; 

	//// SpaceTracer
	//virtual void onBoxChanged(const trimesh::dbox3& box) override;
	//virtual void onSceneChanged(const trimesh::dbox3& box) override;

};

#endif // _NULLSPACE_PAINT_OPERATE_MODE_1591235079966_H