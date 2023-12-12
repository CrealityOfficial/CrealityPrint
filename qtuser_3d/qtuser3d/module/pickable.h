#ifndef QTUSER_3D_PICKABLE_1595165203011_H
#define QTUSER_3D_PICKABLE_1595165203011_H
#include "qtuser3d/qtuser3dexport.h"
#include "qtuser3d/utils/qtuser3denum.h"
#include <QtCore/QPoint>

namespace qtuser_3d
{
	class QTUSER_3D_API Pickable : public QObject
	{
		Q_OBJECT
	public:
		Pickable(QObject* parent = nullptr);
		virtual ~Pickable();

		virtual bool isGroup();

		virtual bool hoverPrimitive(int primitiveID);
		virtual void selectPrimitive(int chunk);
		virtual void unHoverPrimitive();

		virtual int primitiveNum();   // 0
		virtual int chunk(int primitiveID);

		virtual void pick(int primitiveID);

		///////////////////////
		ControlState state();
		void setState(ControlState state);
		bool selected();

		void setFaceBase(int faceBase);
		int faceBase();

		bool enableSelect();
		void setEnableSelect(bool enable);

		bool noPrimitive();
		void setNoPrimitive(bool noPrimitive);  // only set once at construct

		bool notSupportData();
		void setNotSupportData(bool notSupportData);

		virtual void setVisible(bool visible);
		bool isVisible();
	public slots:
		void setSelected(bool selected);
	protected:
		virtual void onStateChanged(ControlState state);
		virtual void faceBaseChanged(int faceBase);
		virtual void noPrimitiveChanged(bool noPrimitive);

	signals:
		void signalStateChanged(ControlState state);
		void signalFaceBaseChanged(int faceBase);
	protected:
		bool m_visible;

		ControlState m_state;
		int m_faceBase;
		bool m_noPrimitive;
		bool m_enableSelect;

	};

	class FacePicker;
	QTUSER_3D_API Pickable* checkPickableFromList(FacePicker* picker, QPoint point, QList<Pickable*>& list, int* primitiveID);
	QTUSER_3D_API bool checkPickerColor(FacePicker* picker, QPoint point, int* faceID);
}
#endif // QTUSER_3D_PICKABLE_1595165203011_H