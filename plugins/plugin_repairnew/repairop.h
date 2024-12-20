#ifndef _NULLSPACE_RepairOP_1589851340567_H
#define _NULLSPACE_RepairOP_1589851340567_H
#include "qtuser3d/scene/sceneoperatemode.h"

namespace creative_kernel
{
	class ModelN;
}

namespace qtuser_3d
{
	class PureColorEntity;
	class ModelNEntity;
}

class RepairOp: public qtuser_3d::SceneOperateMode
{
	Q_OBJECT
public:
	RepairOp(QObject* parent = nullptr);
	virtual ~RepairOp();
	void repairModel(QList<creative_kernel::ModelN*> selections);
	
protected:
	void onAttach() override;
	void onDettach() override;
	void onLeftMouseButtonClick(QMouseEvent* event) override;
	void onHoverMove(QHoverEvent* event) override;
signals:
	void repairSuccess();
//	void judgeSuccess();
public:
//	void judgeModel(QList<creative_kernel::ModelN*> selections);
protected:

};
#endif // _NULLSPACE_RepairOP_1589851340567_H
