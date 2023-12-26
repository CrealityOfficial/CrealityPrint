#ifndef _NULLSPACE_MODEL_EFFECT_OP_202312091136_H
#define _NULLSPACE_MODEL_EFFECT_OP_202312091136_H

#include <QtCore/QObject>

//#if defined(MODELEFFECT_DLL)
//#  define MODELEFFECT_API Q_DECL_EXPORT
//#else
//#  define MODELEFFECT_API Q_DECL_IMPORT
//#endif
#include <QtGui/qcolor.h>
#include <QColorDialog>

#include "qtuser3d/scene/sceneoperatemode.h"
#include "modeleffectsetter.h"


class ModelEffectOp : public qtuser_3d::SceneOperateMode
{
	Q_OBJECT
public:
	ModelEffectOp(QObject* parent = nullptr);
	virtual ~ModelEffectOp();

protected:
	void onAttach() override;
	void onDettach() override;

	void onKeyPress(QKeyEvent* event) override;

protected slots:
	void onCurrentColorChanged(const QColor& color);
	void onColorSelected(const QColor& color);

private:
	ModelEffectSettor* m_setter;

	QColorDialog* m_dialog;
	EffectItem* m_currentItem;
};

#endif // ! _NULLSPACE_MODEL_EFFECT_OP_202312091136_H


