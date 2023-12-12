#ifndef QTUSER_3D_KERNEL_SELECTOR_1652767941893_H
#define QTUSER_3D_KERNEL_SELECTOR_1652767941893_H
#include "qtuser3d/qtuser3dexport.h"
#include "qtuser3d/module/pickable.h"
#include "qtuser3d/module/facepicker.h"
#include "qtuser3d/module/pickableselecttracer.h"

namespace qtuser_3d
{
	class QTUSER_3D_API Selector : public QObject
	{
		Q_OBJECT
	public:
		Selector(QObject* parent = nullptr);
		virtual ~Selector();

		void setPickerSource(qtuser_3d::FacePicker* picker);
		void disableReverseSelect(bool disable);
		void setEnabled(bool enabled);

		void addPickable(qtuser_3d::Pickable* pickable);
		void removePickable(qtuser_3d::Pickable* pickable);
		void addTracer(qtuser_3d::SelectorTracer* tracer);
		void removeTracer(qtuser_3d::SelectorTracer* tracer);
		QList<qtuser_3d::Pickable*> selectionmPickables();

		bool facePick(const QPoint& p, int* faceID);
		qtuser_3d::Pickable* check(const QPoint& p, int* primitiveID = nullptr);

		void clearHover();
		bool hover(const QPoint& p);
		virtual void select(const QPoint& p, bool sGroup = false);
		virtual void selectCtrl(const QPoint& p, bool sGroup = false);
		void selectPickable(qtuser_3d::Pickable* pickable);
		void appendSelect(qtuser_3d::Pickable* pickable);
		void appendSelects(const QList<qtuser_3d::Pickable*> pickables);
		void selectGroup(qtuser_3d::Pickable* pickable);
		void updateFaceBases();

		void selectOne(qtuser_3d::Pickable* pickable);
		void selectMore(const QList<qtuser_3d::Pickable*>& pickables);

		void unselectOne(qtuser_3d::Pickable* pickable);
		void unselectMore(const QList<qtuser_3d::Pickable*>& pickables);

		void selectPickables(const QList<qtuser_3d::Pickable*>& onList, const QList<qtuser_3d::Pickable*>& offList);
		
		void selectRect(const QRect& rect, bool exclude = true);


		Q_INVOKABLE void selectAll();
		Q_INVOKABLE void selectNone();

		void changed(qtuser_3d::Pickable* pickable);

		void notifyTracers(qtuser_3d::SelectorTracer* tracer = nullptr);
	
	protected:
		void _add(qtuser_3d::Pickable* pickable);
		void _remove(qtuser_3d::Pickable* pickable);

		virtual void onChanged();
	protected:
		qtuser_3d::FacePicker* m_pickSource;
		QList<qtuser_3d::Pickable*> m_pickables;

		qtuser_3d::Pickable* m_hoverPickable;

		QList<qtuser_3d::Pickable*> m_selectedPickables;
		QList<qtuser_3d::SelectorTracer*> m_selectorTracers;

		int m_currentFaceBase;
		bool selectNotifying;

		bool m_disableReverseSelect;
		bool m_enabled;
	};
}

#endif // QTUSER_3D_KERNEL_SELECTOR_1652767941893_H