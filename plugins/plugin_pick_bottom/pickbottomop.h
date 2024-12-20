#ifndef _NULLSPACE_PICKBOTTOMOP_1589851340567_H
#define _NULLSPACE_PICKBOTTOMOP_1589851340567_H
#include "qtuser3d/scene/sceneoperatemode.h"
#include "pickface.h"
#include "operation/moveoperatemode.h"
#include <QTimer>

class PickBottomOp: public creative_kernel::MoveOperateMode
	, public creative_kernel::ModelNSelectorTracer
{
	Q_OBJECT
public:
	PickBottomOp(QObject* parent = nullptr);
	virtual ~PickBottomOp();

	bool getMessage();
	void setMessage(bool isRemove);
protected:
	void onAttach() override;
	void onDettach() override;
	void onLeftMouseButtonClick(QMouseEvent* event) override;
	void onHoverMove(QHoverEvent* event) override;

	void onSelectionsChanged() override;
	void onSelectionsBoxChanged() override;

	void reGenerateFaces();
	void removeFaces();
	
	void executeFace(PickFace* face);
signals:
	void mouseLeftClicked();
protected:
	
	std::vector<std::vector<PickFace*>> m_pickFaces;

};
#endif // _NULLSPACE_PICKBOTTOMOP_1589851340567_H
