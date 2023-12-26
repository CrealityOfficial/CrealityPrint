#ifndef _NULLSPACE_SUPPORT_PAINTING_OPERATE_MODE_1591235079966_H
#define _NULLSPACE_SUPPORT_PAINTING_OPERATE_MODE_1591235079966_H

#include "operation/paintoperatemode/paintoperatemode.h"

class SupportPaintingOperateMode : public PaintOperateMode
{
	Q_OBJECT;
public:
	explicit SupportPaintingOperateMode(QObject* parent = nullptr);
	virtual ~SupportPaintingOperateMode();

/* override */
protected:
	virtual void restore(creative_kernel::ModelN* model, const std::vector<std::string>& data) override;
	
	virtual void onAttach() override;
	virtual void onDettach() override;

	virtual void onLeftMouseButtonPress(QMouseEvent* event);
	virtual void onRightMouseButtonPress(QMouseEvent* event);
	virtual void onRightMouseButtonMove(QMouseEvent* event);
	virtual void onRightMouseButtonRelease(QMouseEvent* event);

};

#endif // _NULLSPACE_SUPPORT_PAINTING_OPERATE_MODE_1591235079966_H