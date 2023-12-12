#ifndef CREATIVE_KERNEL_MODELSERIALCOMMAND_1592851419297_H
#define CREATIVE_KERNEL_MODELSERIALCOMMAND_1592851419297_H
//#include "creativekernelexport.h"
#include "trimesh2/Vec.h"
#include "cxkernel/data/modelndata.h"
#include <QtWidgets/QUndoCommand>
#include <QtGui/QQuaternion>

namespace creative_kernel
{
	class ModelN;
	class ModelSerialCommand : public QObject, public QUndoCommand
	{
		typedef struct {
			QString serialName;
			QString inputName;
			trimesh::vec3 localPosition;
			trimesh::vec3 localScale;
			QQuaternion localQuaternion;
			//bool isHoneyFilled;  //ģ���Ƿ���й����ѻ������
			//bool isHoneyHollowed;//ģ���Ƿ���й����ѳ�ǲ���
		}SAVED_INFO;

		Q_OBJECT
	public:
		ModelSerialCommand(QList<ModelN*>& removes, QList<ModelN*>& adds);
		virtual ~ModelSerialCommand();

	protected:
		void undo() override;
		void redo() override;
		void serializeRemovedModels(const QList<ModelN*>& removes, QList<SAVED_INFO>& savedInfos);
		void recoverModelFromSerialData(const QList<SAVED_INFO>& savedSerialInfos, QList<ModelN*>& newModels);

	protected:
		QList<ModelN*> m_adds;
		QList<ModelN*> m_removes;
		QStringList m_serialRemoveNameList;
		QStringList m_serialAddNameList;
		QList<SAVED_INFO> m_savedSerialInfos;
		bool m_firstRedo;

		//SpaceManager* m_space;
	};
}
#endif // CREATIVE_KERNEL_MODELSERIALCOMMAND_1592851419297_H