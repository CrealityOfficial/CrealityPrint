#ifndef _DISPLAY_MESSAGE_MANAGER_202405241631_H
#define _DISPLAY_MESSAGE_MANAGER_202405241631_H
#include <QtCore/QObject>
#include <QVariant>
#include "basickernelexport.h"
#include "data/interface.h"

namespace creative_kernel
{
	class Printer;

	class BASIC_KERNEL_API DisplayMessageManager : public QObject, public SpaceTracer
	{
		Q_OBJECT

	public:
		DisplayMessageManager(QObject* parent = nullptr);
		virtual ~DisplayMessageManager();

		void checkGroupHigherThanBottom();
		bool checkGCodePathScope(const qtuser_3d::Box3D& gcodePathBox, Printer* printer);
		bool checkSliceWarning(const QMap<QString, QPair<QString, int64_t> >& warningDetails);
		void checkSliceEngineError(const QString& sliceErrorStr);
		void updateBedTypeInvalidMessage();

	public slots:
		void onGlobalEnableSupportChanged(bool bEnable);

	protected:
		void onSceneChanged(const trimesh::dbox3& box) override;
		void onModelGroupRemoved(ModelGroup* _model_group) override;

	private :
		QVector<int64_t> m_sliceWarningRelatedObjectIds;
		QVector<int64_t> m_sliceErrorRelatedObjecIds;
	};

}

#endif // _PREFERENCE_MANAGER_H
