#ifndef CREATIVE_KERNEL_MODELNDATA_SERIALCOMMAND_202405101352_H
#define CREATIVE_KERNEL_MODELNDATA_SERIALCOMMAND_202405101352_H
#include "trimesh2/Vec.h"
#include "cxkernel/data/modelndata.h"
#include <QtWidgets/QUndoCommand>
#include <QtGui/QQuaternion>

namespace creative_kernel
{
	class ModelN;

	// replace ModelNData, keep the ModelN
	class ModelNDataSerialCommand : public QObject, public QUndoCommand
	{
		typedef struct {
			QString serialName;
			QString inputName;
		}MD_SAVED_INFO;

		Q_OBJECT
	public:
		ModelNDataSerialCommand( QList<ModelN*>& models, QList<cxkernel::ModelNDataPtr>& modelnDatas, bool resetModel);
		virtual ~ModelNDataSerialCommand();

	protected:
		void undo() override;
		void redo() override;
		void serializeRemovedModelNDatas(const QList<ModelN*>& theModels, QList<MD_SAVED_INFO>& savedInfos);
		void recoverModelNDataFromSerialData(const QList<MD_SAVED_INFO>& savedSerialInfos, QList<cxkernel::ModelNDataPtr>& serialdatas);

	protected:
		QStringList m_modelSerialNameList;
		QList<MD_SAVED_INFO> m_savedSerialInfos;
		QList<cxkernel::ModelNDataPtr> m_modelnDatas;
		bool m_resetModel;

	};
}
#endif // CREATIVE_KERNEL_MODELNDATA_SERIALCOMMAND_202405101352_H