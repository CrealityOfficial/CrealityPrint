#include "meshloadercenter.h"

#include "data/rawdata.h"
#include "kernel/kernel.h"

#include "cxkernel/interface/iointerface.h"
#include "cxkernel/interface/modelninterface.h"
#include "interface/spaceinterface.h"
#include "interface/shareddatainterface.h"
#include "interface/projectinterface.h"
#include "interface/renderinterface.h"
#include "interface/uiinterface.h"
#include "interface/printerinterface.h"
#include "interface/layoutinterface.h"

#include "internal/multi_printer/printer.h"

#include <QtCore/QFile>
#include <QFileInfo>
#include <trimesh2/TriMesh.h>
#include <trimesh2/Vec.h>
#include "common_3mf.h"
namespace creative_kernel
{
	ModelDataPtr convertModelNData2ModelData(cxkernel::ModelNDataPtr data)
	{
		if (!data)
			return nullptr;

		ModelDataPtr model_data(new ModelData());
		model_data->defaultColor = data->defaultColor;
		model_data->colors = data->colors;
		model_data->supports = data->supports;
		model_data->seams = data->seams;
		cxkernel::MeshDataPtr mesh_data(new cxkernel::MeshData());
		mesh_data->mesh = data->mesh;
		mesh_data->hull = data->hull;
		mesh_data->offset = data->offset;
		mesh_data->faces = data->faces;
		model_data->mesh = mesh_data;
		model_data->name = data->input.name;

		return model_data;
	}

	MeshLoadCenter::MeshLoadCenter(QObject* parent)
		: QObject(parent)
		, m_load_count(0)
		, m_model_group_into(nullptr)
	{

	}

	MeshLoadCenter::~MeshLoadCenter()
	{

	}

	void MeshLoadCenter::adaptTempModels()
	{
		Printer* printer = currentPrinter();
		assert(printer);

		trimesh::dbox3 base_box = printer->printerBox();

		for (ModelDataPtr data : m_big_datas)
		{
			data->mesh->adaptBigBox(base_box);
		}

		for (ModelDataPtr data : m_small_datas)
		{
			data->mesh->adaptSmallBox(base_box);
		}
		for (ModelDataPtr data : m_3mf_small_datas)
		{
			data->mesh->adaptSmallBox(base_box);
		}

		m_datas.append(m_small_datas);
		m_datas.append(m_big_datas);
		m_datas.append(m_3mf_small_datas);
		m_small_datas.clear();
		m_big_datas.clear();
		m_3mf_small_datas.clear();
		processDatas();
	}

	void MeshLoadCenter::clearTempModels()
	{
		m_small_datas.clear();
		m_big_datas.clear();
		m_3mf_small_datas.clear();
	}

	void MeshLoadCenter::ignoreTempModels()
	{
		m_datas.append(m_small_datas);
		m_datas.append(m_big_datas);
		m_small_datas.clear();
		m_big_datas.clear();
		m_3mf_small_datas.clear();

		processDatas();
	}

	void MeshLoadCenter::openGroupsMeshFile(const QList<QStringList>& group_files)
	{

	}

	void MeshLoadCenter::setJoinedModelGroup(ModelGroup* group)
	{
		m_model_group_into = group;
	}
	
	void MeshLoadCenter::setOpenedModelType(ModelNType type, ModelGroup* _model_group)
	{
		m_type = type;
		m_model_group_into = _model_group;
	}

	void MeshLoadCenter::openMeshFile()
	{
		getKernel()->openMeshFile();
	}

	void MeshLoadCenter::openSpecifyMeshFile(const QString& file_name)
	{
		cxkernel::openFileWithString(file_name);
	}

	void MeshLoadCenter::createFromInput(const MeshCreateInput& create_input)
	{
		QList<MeshCreateInput> create_inputs;
		create_inputs.append(create_input);
		createFromInputs(create_inputs);
	}

	void MeshLoadCenter::createFromInputs(const QList<MeshCreateInput>& create_inputs)
	{
		int count = 0;
		for (const MeshCreateInput& create_input : create_inputs)
		{
			if (create_input.mesh)
				++count;
		}

		if (count <= 0)
			return;

		modelMeshLoadStarted(count);

		ModelSpace* space = getModelSpace();
		for (const MeshCreateInput& create_input : create_inputs)
		{
			if (create_input.mesh)
			{
				cxkernel::ModelCreateInput input;
				input.defaultColor = create_input.defaultColor;
				input.mesh = create_input.mesh;
				input.colors = create_input.colors;
				input.supports = create_input.supports;
				input.seams = create_input.seams;
				input.fileName = create_input.fileName;
				input.name = create_input.name;
				input.description = create_input.description;
				cxkernel::addModelFromCreateInput(input);

				m_type = create_input.type;
				m_model_group_into = space->getModelGroupByObjectId(create_input.group);
				
			}
		}
	}
	creative_kernel::ModelNType getModelNTypeBySubType(const std::string& stype)
    {
        if ("normal_part" == stype)
            return ModelNType::normal_part;
        if ("negative_part" == stype)
            return ModelNType::negative_part;
        if ("modifier_part" == stype)
            return ModelNType::modifier;
        if ("support_blocker" == stype)
            return ModelNType::support_defender;
        if ("support_enforcer" == stype)
            return ModelNType::support_generator;

        return ModelNType::normal_part;
    }
\
	void MeshLoadCenter::process(common_3mf::Scene3MF scene)
	{
		SceneCreateData scene_data;
		for (common_3mf::Group3MF& group : scene.groups)
        {
            GroupCreateData group_create_data;
            group_create_data.model_group_id = -1;
            group_create_data.xf = group.xf;
            group_create_data.name = QString("%1").arg(group.name.c_str());
			//非组合体模型检测模型是否太小
			if(group.models.size() == 1)
			{
				common_3mf::Model3MF& model = group.models[0];
				model.mesh->need_bbox();
                trimesh::vec3 data_box = model.mesh->bbox.size();
				const double volume_threshold_inches = 8.0;
				double volume = data_box.x * data_box.y * data_box.z;
				if (data_box.x * data_box.y * data_box.z < volume_threshold_inches)
				{
					
					ModelDataPtr model_data = createModelData(model.mesh, model.colors, model.seams, model.supports, model.extruder - 1);
                	if (!model_data)
                    	continue;
					if(!model.name.empty())
					{
						model_data->name = QString::fromStdString(model.name);
					}else{
						model_data->name = QString("Object_%1").arg(m_3mf_small_datas.size());
					}
					
					m_3mf_small_datas.append(model_data);
					continue;
				}
			}
            for (common_3mf::Model3MF& model : group.models)
            {
                //float start = 0.3f + (float)model_count_load * 0.7f / (float)m_scene.model_counts;
                //float end = 0.3f + (float)(model_count_load + 1) * 0.7f / (float)m_scene.model_counts;

                //tracer.resetProgressScope(start, end);

                model.mesh->need_bbox();
                trimesh::vec3 size = model.mesh->bbox.size();
                if ((model.mesh->vertices.size() < 4)
                    || (model.mesh->faces.size() <= 3))
                    //|| (size.x * size.y * size.z < 1.0f))   // fix bug:[ID1027978]: [3mf文件]导入Orca创建的3mf文件到C3D软件后，组合体的部分零件数据丢失
                    continue;

                makeSureDataValid(model.mesh, model.colors, model.seams, model.supports);
                ModelDataPtr model_data = createModelData(model.mesh, model.colors, model.seams, model.supports, model.extruder - 1);
                if (!model_data)
                    continue;

                ModelCreateData model_create_data;
                model_create_data.name = QString("%1").arg(model.name.c_str());
                model_create_data.xf = model.xf;
                model_create_data.dataID = registerModelData(model_data);
                model_create_data.dataID(ModelType) = int(getModelNTypeBySubType(model.subtype));
                group_create_data.models.append(model_create_data);
                //++model_count_load;
            }

            if (group_create_data.models.size() > 0)
                scene_data.add_groups.append(group_create_data);
			
        }
		
        modifyScene(scene_data);
		if(m_3mf_small_datas.size() > 0)
		{
				requestQmlDialog("idModelUnfitMessageDlg_small");
			}
	}
	void MeshLoadCenter::process(cxkernel::ModelNDataPtr data)
	{
		if (data && !data->input.fileName.isEmpty())
		{
			QString fileName = data->input.fileName;
			QFileInfo fileInfo(fileName);
			QString shortName = fileName;
			if (fileInfo.exists())
			{
				QString path = fileInfo.absoluteFilePath();
				setMenuMostRecentFile(path);
				shortName = fileInfo.fileName();
			}
			data->input.name = shortName;
		}

		Printer* printer = currentPrinter();
		assert(printer);

		trimesh::dbox3 base_box = printer->printerBox();

		trimesh::dvec3 base_box_size = base_box.size();
		trimesh::dvec3 data_box = trimesh::dvec3(data->calculateBox().size());
		ModelDataPtr model_data = convertModelNData2ModelData(data);

		const double volume = 8;
		if (data_box.x > base_box_size.x ||
			data_box.y > base_box_size.y ||
			data_box.z > base_box_size.z)
		{
			m_big_datas.append(model_data);
			requestQmlDialog("idModelUnfitMessageDlg");
		}
		else if (data_box.x * data_box.y * data_box.z < volume)
		{
			m_small_datas.append(model_data);
			requestQmlDialog("idModelUnfitMessageDlg_small");
		}
		else
		{
			m_datas.append(model_data);
		}

		if (m_datas.size() == m_load_count)
			processDatas();
	}

	void MeshLoadCenter::modelMeshLoadStarted(int num)
	{
		//setContinousRender();
		m_datas.clear();

		// fix:[ID1028107]在模型列表点击导入按钮导入多个大尺寸模型的问题
		//m_big_datas.clear();
		//m_small_datas.clear();

		m_load_count = num;
	}

	void MeshLoadCenter::getModelDataImportPos(QList<QVector3D>& outPoses)
	{
		QList<CONTOUR_PATH> activePathItems;
		for (ModelDataPtr data : m_datas)
		{
			trimesh::xform unitXf;
			std::vector<trimesh::dvec3> ddatas;
			data->mesh->convex(unitXf, ddatas);

			std::vector<trimesh::vec3> pathData;
			pathData.clear();
			for (const trimesh::dvec3& v : ddatas)
				pathData.push_back(trimesh::vec3(v));

			activePathItems.append(pathData);
		}

		getPositionBySimpleLayout(activePathItems, outPoses);
	}

	void MeshLoadCenter::processDatas()
	{
		if (m_datas.size() > 0)
		{
			Printer* printer = currentPrinter();
			assert(printer);

			trimesh::dbox3 base_box = printer->printerBox();
			trimesh::dvec3 base_center = base_box.center();

			SceneCreateData scene_data;
			if (m_model_group_into)  //add part
			{
				for (ModelDataPtr data : m_datas)
				{
					addPart2Group(data, m_type, m_model_group_into);
				}
				
			} else {
				QList<QVector3D> outPoses;
				outPoses.reserve(m_datas.size());
				getModelDataImportPos(outPoses);

				for(int i = 0; i < m_datas.size(); i++)
				{
					ModelDataPtr data = m_datas.at(i);
					GroupCreateData group_data;
					ModelCreateData model_create_data;

					if (m_model_group_into)
						group_data.model_group_id = m_model_group_into->getObjectId();

					SharedDataID id = registerModelData(data);
					id(ModelType) = (int)m_type;

					if (m_type == ModelNType::modifier)
					{
						ModelGroup* group = getModelGroupByObjectId(group_data.model_group_id);
						if (group)
							id(MaterialID) = group->defaultColorIndex();
					}

					model_create_data.dataID = id;
					model_create_data.name = data->name;

					trimesh::dbox3 b = data->mesh->calculateBox();
					model_create_data.xf = trimesh::xform::trans(0.0, 0.0, 0.0);

					group_data.name = data->name;
					group_data.models.append(model_create_data);
					group_data.xf = trimesh::xform::trans(outPoses[i].x(), outPoses[i].y(), -b.min.z);
					scene_data.add_groups.append(group_data);
				}

				modifyScene(scene_data);
			}

		}

		m_datas.clear();
		m_load_count = 0;
		m_type = ModelNType::normal_part;
		m_model_group_into = nullptr;
	}
}
