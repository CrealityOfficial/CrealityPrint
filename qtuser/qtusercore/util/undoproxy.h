#ifndef _QTUSER_CORE_UNDOPROXY_1588906890012_H
#define _QTUSER_CORE_UNDOPROXY_1588906890012_H
#include "qtusercore/qtusercoreexport.h"
#include <QtWidgets/QUndoStack>

namespace qtuser_core
{
	class UndoCallback
	{
	public:
		virtual ~UndoCallback() {}
		virtual void onUndo() = 0;
		virtual void onRedo() = 0;
	};

	class QTUSER_CORE_API UndoProxy : public QObject
	{
		Q_OBJECT

		Q_PROPERTY(bool canRedo READ canRedo NOTIFY canRedoChanged)
		Q_PROPERTY(bool canUndo READ canUndo NOTIFY canUndoChanged)

		// Text for the undo and redo actions
		Q_PROPERTY(QString redoText READ redoText NOTIFY redoTextChanged)
		Q_PROPERTY(QString undoText READ undoText NOTIFY undoTextChanged)
	public:
		UndoProxy(QObject* parent = nullptr);
		virtual ~UndoProxy();

		bool canRedo() const;
		bool canUndo() const;
		QString redoText() const;
		QString undoText() const;

		Q_INVOKABLE void undo();
		Q_INVOKABLE void redo();

		void setUndoStack(QUndoStack* undoStack);

		void addUndoCallback(UndoCallback* callback);
		void removeUndoCallback(UndoCallback* callback);
	signals:
		void canRedoChanged();
		void canUndoChanged();
		void redoTextChanged();
		void undoTextChanged();
	protected:
		QUndoStack* m_undoStack;
		QList<UndoCallback*> m_callbacks;
	};
}

#endif // _QTUSER_CORE_UNDOPROXY_1588906890012_H
