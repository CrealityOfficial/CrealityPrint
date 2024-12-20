#ifndef _NULLSPACE_SEAM_PAINTING_OPERATE_MODE_1591235079966_H
#define _NULLSPACE_SEAM_PAINTING_OPERATE_MODE_1591235079966_H

#include "operation/paintoperatemode/paintoperatemode.h"

class SeamPaintingOperateMode : public PaintOperateMode
{
	Q_OBJECT;
public:
	explicit SeamPaintingOperateMode(QObject* parent = nullptr);
	virtual ~SeamPaintingOperateMode();

private:
	int m_paintIndex { 0 };
	int m_blockIndex { 1 };

private:
	void initColors();

/* override */
protected:
	virtual void restore(int objectID, const QList<std::vector<std::string>>& data) override;
	virtual void onAttach() override;
	virtual void onDettach() override;

	virtual void onLeftMouseButtonPress(QMouseEvent* event);
	virtual void onRightMouseButtonPress(QMouseEvent* event);
	virtual void onRightMouseButtonMove(QMouseEvent* event);
	virtual void onRightMouseButtonRelease(QMouseEvent* event);

};

#endif // _NULLSPACE_SEAM_PAINTING_OPERATE_MODE_1591235079966_H