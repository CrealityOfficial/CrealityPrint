#include "undoproxy.h"

namespace qtuser_core
{
	UndoProxy::UndoProxy(QObject* parent)
		:QObject(parent)
		, m_undoStack(nullptr)
	{
	}

	UndoProxy::~UndoProxy()
	{
	}

	void UndoProxy::setUndoStack(QUndoStack* undoStack)
	{
		if (m_undoStack == undoStack)
			return;

		m_undoStack = undoStack;
		if (m_undoStack)
		{
			connect(m_undoStack, SIGNAL(canRedoChanged(bool)), this, SIGNAL(canRedoChanged()));
			connect(m_undoStack, SIGNAL(canUndoChanged(bool)), this, SIGNAL(canUndoChanged()));
			connect(m_undoStack, SIGNAL(redoTextChanged(QString)), this, SIGNAL(redoTextChanged()));
			connect(m_undoStack, SIGNAL(undoTextChanged(QString)), this, SIGNAL(undoTextChanged()));
		}

		emit canRedoChanged();
		emit canUndoChanged();
		emit redoTextChanged();
		emit undoTextChanged();
	}

	void UndoProxy::addUndoCallback(UndoCallback* callback)
	{
		if (callback && !m_callbacks.contains(callback))
			m_callbacks.append(callback);
	}

	void UndoProxy::removeUndoCallback(UndoCallback* callback)
	{
		if (callback && m_callbacks.contains(callback))
			m_callbacks.removeOne(callback);
	}

	void UndoProxy::undo()
	{
		if (m_undoStack)
		{
			m_undoStack->undo();

			for (UndoCallback* callback : m_callbacks)
				callback->onUndo();
		}
	}

	void UndoProxy::redo()
	{
		if (m_undoStack)
		{
			m_undoStack->redo();

			for (UndoCallback* callback : m_callbacks)
				callback->onRedo();
		}
	}

	bool UndoProxy::canRedo() const
	{
		if (m_undoStack)
			return m_undoStack->canRedo();
		return false;
	}

	bool UndoProxy::canUndo() const
	{
		if (m_undoStack)
			return m_undoStack->canUndo();
		return false;
	}

	QString UndoProxy::redoText() const
	{
		if (m_undoStack)
			return m_undoStack->redoText();
		return QString();
	}

	QString UndoProxy::undoText() const
	{
		if (m_undoStack)
			return m_undoStack->undoText();
		return QString();
	}
}
