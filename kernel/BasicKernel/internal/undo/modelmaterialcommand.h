#ifndef CREATIVE_KERNEL_MODEL_MATERIAL_COMMAND_1592851419297_H
#define CREATIVE_KERNEL_MODEL_MATERIAL_COMMAND_1592851419297_H
//#include "creativekernelexport.h"
#include "trimesh2/Vec.h"
#include "cxkernel/data/modelndata.h"
#include <QtWidgets/QUndoCommand>
#include <QtGui/QQuaternion>

namespace creative_kernel
{
	class ModelN;
	class ModelMaterialCommand : public QObject, public QUndoCommand
	{
		Q_OBJECT
	public:
		ModelMaterialCommand(QList<ModelN*>& models, int newMaterial);
		virtual ~ModelMaterialCommand();

	protected:
		void undo() override;
		void redo() override;

	protected:
		QStringList m_serialNames;
		QList<int> m_oldMaterials;
		int m_newMaterial;
	};
}
#endif // CREATIVE_KERNEL_MODEL_MATERIAL_COMMAND_1592851419297_H