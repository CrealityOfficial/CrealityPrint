#ifndef _NULLSPACE_COLOR_OPERATE_MODE_1591235079966_H
#define _NULLSPACE_COLOR_OPERATE_MODE_1591235079966_H

#include "operation/paintoperatemode/paintoperatemode.h"

class ColorOperateMode : public PaintOperateMode
{
	Q_OBJECT;
public:
	explicit ColorOperateMode(QObject* parent = nullptr);
	virtual ~ColorOperateMode();

private slots:
	virtual void handleColorsListChanged(const std::vector<trimesh::vec>& newColors) override;
	void onApplyColor();
	void onDefaultColorIndexChanged();

/* override */
protected:
	virtual void restore(int objectID, const QList<std::vector<std::string>>& data) override;
	virtual void onAttach() override;
	virtual void onDettach() override;

	virtual void onKeyPress(QKeyEvent* e) override;

};

#endif // _NULLSPACE_COLOR_OPERATE_MODE_1591235079966_H