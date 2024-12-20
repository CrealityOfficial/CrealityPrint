#ifndef CREATIVE_KERNEL_MODELSERIALCOMMAND_1592851419297_H
#define CREATIVE_KERNEL_MODELSERIALCOMMAND_1592851419297_H
#include "data/rawdata.h"
#include "data/modelgroup.h"
#include "data/modeln.h"

#include <QtWidgets/QUndoCommand>

namespace creative_kernel
{
	class SpaceSerialCommand : public QObject, public QUndoCommand
	{
		Q_OBJECT
	public:
		SpaceSerialCommand(const SceneCreateData& data, QObject* parent = nullptr);
		virtual ~SpaceSerialCommand();

	protected:
		struct ModelBunch
		{
			QList<ModelN*> models;
			ModelGroup* _model_group;
		};
		struct SceneBunch {
			QList<ModelBunch> add_groups;
			QList<ModelBunch> remove_groups;
		};

		void undo() override;
		void redo() override;

		void invoke(const QList<ModelBunch>& adds, const QList<ModelBunch>& removes);
		void invokeModify(const QList<ModelGroup*>& add_model_groups, const QList<ModelGroup*>& remove_model_groups,
			const QList<ModelBunch>& add_models, const QList<ModelBunch>& remove_models);

	protected:
		SceneBunch m_data;
	};
}
#endif // CREATIVE_KERNEL_MODELSERIALCOMMAND_1592851419297_H