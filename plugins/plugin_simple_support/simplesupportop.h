#ifndef _NULLSPACE_SIMPLESUPPORTOP_1590465245043_H
#define _NULLSPACE_SIMPLESUPPORTOP_1590465245043_H
#include "data/header.h"
#include "qtuser3d/scene/sceneoperatemode.h"

#include "qtuser3d/module/pickableselecttracer.h"
#include <QtCore/QPoint>
#include "qtuser3d/math/ray.h"

namespace creative_kernel
{
	class ModelN;
	class GridCache;
}

enum class SupportMode
{
	esm_add,
	esm_delete
};

namespace qtuser_3d
{
	class XEntity;
	class AlonePointEntity;
	class LineEntity;
}

class FDMSupportHandler;
class SimpleSupportOp: public qtuser_3d::SceneOperateMode, 
	public qtuser_3d::SelectorTracer
{
public:
	SimpleSupportOp(QObject* parent = nullptr);
	virtual ~SimpleSupportOp();

	bool hasSupport();
	void setAddMode();
	void setDeleteMode();
	void deleteAllSupport();

	void setSupportAngle(float angle);
	void setSupportSize(float size);
	void autoSupport(bool platform);
	bool getShowPop();


protected:
	void onAttach() override;
	void onDettach() override;
	void onLeftMouseButtonClick(QMouseEvent* event) override;
	void onHoverMove(QHoverEvent* event) override;

	qtuser_3d::Pickable* surfaceXy(const QPoint& pos, int* primitiveID);

	void changeShaders(const QString& name);

	void onSelectionsChanged() override;
	void selectChanged(qtuser_3d::Pickable* pickable) override;
protected:
	qtuser_3d::AlonePointEntity* m_debugEntity;
	/*qtuser_3d::LineEntity* m_lineEntity;*/

	SupportMode m_supportMode;

	float m_supportSize;
	float m_supportAngle;

	creative_kernel::GridCache* m_cache;

	FDMSupportHandler* m_handler;
	bool m_bShowPop = false;

	//
	qtuser_3d::XEntity* m_cylinderEntity;//圆柱替代线条
	std::auto_ptr<trimesh::TriMesh> m_cylinderMesh;
};
#endif // _NULLSPACE_SIMPLESUPPORTOP_1590465245043_H
