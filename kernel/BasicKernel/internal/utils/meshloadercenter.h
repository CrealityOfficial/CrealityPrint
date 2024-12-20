#ifndef _KERNEL_MESH_LOADER_1590982007351_H
#define _KERNEL_MESH_LOADER_1590982007351_H
#include "cxkernel/data/modelndata.h"
#include "data/rawdata.h"
#include "data/kernelenum.h"

namespace creative_kernel
{
	class ModelN;
	class ModelGroup;
	class MeshLoadCenter : public QObject , public cxkernel::ModelNDataProcessor
	{
		Q_OBJECT
	public:
		MeshLoadCenter(QObject* parent = nullptr);
		virtual ~MeshLoadCenter();

		Q_INVOKABLE void adaptTempModels();
		Q_INVOKABLE void clearTempModels();
		Q_INVOKABLE void ignoreTempModels();
		Q_INVOKABLE void openGroupsMeshFile(const QList<QStringList>& group_files);

		void setJoinedModelGroup(ModelGroup* group);
		void setOpenedModelType(ModelNType type, ModelGroup* _model_group = nullptr);
		void openMeshFile();
		void openSpecifyMeshFile(const QString& file_name);
		void createFromInput(const MeshCreateInput& create_input);
		void createFromInputs(const QList<MeshCreateInput>& create_inputs);
	protected:
		void process(cxkernel::ModelNDataPtr data) override;
		void process(common_3mf::Scene3MF data) override;
		void modelMeshLoadStarted(int num) override;
		void getModelDataImportPos(QList<QVector3D>& outPoses);

		void processDatas();
	protected:
		int m_load_count;
		ModelGroup* m_model_group_into;
		ModelNType m_type { ModelNType::normal_part };

		QList<ModelDataPtr> m_small_datas;
		QList<ModelDataPtr> m_3mf_small_datas;
		QList<ModelDataPtr> m_big_datas;
		QList<ModelDataPtr> m_datas;


	};
}
#endif // _KERNEL_MESH_LOADER_1590982007351_H
