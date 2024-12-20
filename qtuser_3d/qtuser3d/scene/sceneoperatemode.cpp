#include "sceneoperatemode.h"
#include <QDebug>

namespace qtuser_3d
{
	SceneOperateMode::SceneOperateMode(QObject* parent)
		:QObject(parent)
	{
	}
	
	SceneOperateMode::~SceneOperateMode()
	{
	}

	void SceneOperateMode::onHoverEnter(QHoverEvent* event)
	{
		(void)event;
	}

	void SceneOperateMode::onHoverLeave(QHoverEvent* event)
	{
		(void)event;
	}

	void SceneOperateMode::onHoverMove(QHoverEvent* event)
	{
		(void)event;
	}

	void SceneOperateMode::onLeftMouseButtonPress(QMouseEvent* event)
	{
		(void)event;
	}

	void SceneOperateMode::onLeftMouseButtonRelease(QMouseEvent* event)
	{
		(void)event;
	}

	void SceneOperateMode::onLeftMouseButtonMove(QMouseEvent* event)
	{
		(void)event;
	}

	void SceneOperateMode::onLeftMouseButtonClick(QMouseEvent* event)
	{
		(void)event;
	}

	void SceneOperateMode::onMidMouseButtonPress(QMouseEvent* event)
	{
		(void)event;
	}

	void SceneOperateMode::onMidMouseButtonRelease(QMouseEvent* event)
	{
		(void)event;
	}

	void SceneOperateMode::onMidMouseButtonMove(QMouseEvent* event)
	{
		(void)event;
	}

	void SceneOperateMode::onMidMouseButtonClick(QMouseEvent* event)
	{
		(void)event;
	}

	void SceneOperateMode::onWheelEvent(QWheelEvent* event)
	{
		(void)event;
	}

	void SceneOperateMode::onRightMouseButtonPress(QMouseEvent* event)
	{
		(void)event;
	}

	void SceneOperateMode::onRightMouseButtonRelease(QMouseEvent* event)
	{
		(void)event;
	}

	void SceneOperateMode::onRightMouseButtonMove(QMouseEvent* event)
	{
		(void)event;
	}

	void SceneOperateMode::onRightMouseButtonClick(QMouseEvent* event)
	{
		(void)event;
	}

	void SceneOperateMode::onKeyPress(QKeyEvent* event)
	{
		(void)event;
	}

	void SceneOperateMode::onKeyRelease(QKeyEvent* event)
	{
		(void)event;
	}

	bool SceneOperateMode::shouldMultipleSelect()
	{
		return false;
	}
}