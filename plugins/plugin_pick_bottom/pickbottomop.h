#ifndef _NULLSPACE_PICKBOTTOMOP_1589851340567_H
#define _NULLSPACE_PICKBOTTOMOP_1589851340567_H
#include "qtuser3d/scene/sceneoperatemode.h"
#include "qtuser3d/module/pickableselecttracer.h"
#include "pickface.h"
#include "operation/moveoperatemode.h"

class PickBottomOp: public MoveOperateMode
	, public qtuser_3d::SelectorTracer
{
	Q_OBJECT
public:
	PickBottomOp(QObject* parent = nullptr);
	virtual ~PickBottomOp();

	bool getMessage();
	void setMessage(bool isRemove);
	void setMaxFaceBottom();
protected:
	void onAttach() override;
	void onDettach() override;
	void onLeftMouseButtonClick(QMouseEvent* event) override;
	void onHoverMove(QHoverEvent* event) override;

	void selectChanged(qtuser_3d::Pickable* pickable) override;
	void onSelectionsChanged() override;
	void reGenerateFaces();
	void removeFaces();
	
	void executeFace(PickFace* face);
signals:
	void mouseLeftClicked();
protected:
	
	std::vector<std::vector<PickFace*>> m_pickFaces;
};
#endif // _NULLSPACE_PICKBOTTOMOP_1589851340567_H
