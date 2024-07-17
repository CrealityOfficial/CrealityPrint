#include "ansycslicer.h"
#include "data/modeln.h"
#include "data/modelgroup.h"
#include "data/kernelmacro.h"

#include "supportinput.h"
#include "modelninput.h"

#include "qtusercore/module/systemutil.h"
#include "qtuser3d/trimesh2/conv.h"

#include "interface/spaceinterface.h"
#include "interface/machineinterface.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/printerinterface.h"
#include "us/usettingwrapper.h"

#include <QtCore/QUuid>
#include <QtCore/QFile>
#include <QtCore/QDir>

#include "internal/multi_printer/printer.h"

namespace creative_kernel
{
	SettingsPtr convert(const cxgcode::USettings& settings)
	{
		SettingsPtr ptr(new crslice::Settings());
		const QHash<QString, cxgcode::USetting*>& ss = settings.settings();
		for (QHash<QString, cxgcode::USetting*>::ConstIterator it = ss.begin();
			it != ss.end(); ++it)
		{
			ptr->add(it.key().toStdString(), it.value()->str().toStdString());
		}
		return ptr;
	}

	SliceInput::SliceInput(QObject* parent)
		:QObject(parent)
	{

	}

	SliceInput::~SliceInput()
	{

	}

	bool SliceInput::canSlice()
	{
		return Groups.count() > 0;
	}

	trimesh::box3 SliceInput::sceneBox()
	{
		trimesh::box3 box;
		for (ModelGroupInput* Group : Groups)
		{
			for (ModelInput* Model : Group->modelInputs)
			{
				if (Model->ptr())
				{
					Model->ptr()->need_bbox();
					box += Model->ptr()->bbox;
				}
			}
		}
		return box;
	}

	bool SliceInput::hasModel()
	{
		for (ModelGroupInput* Group : Groups)
		{
			if (!Group->modelInputs.empty())
			{
				return true;
			}
		}
		return false;
	}

	AnsycSlicer::AnsycSlicer(QObject* parent)
		:QObject(parent)
	{
	}

	AnsycSlicer::~AnsycSlicer()
	{
	}

	SliceResultPointer AnsycSlicer::doSlice(SliceInput& input, qtuser_core::ProgressorTracer& tracer, SliceAttain* _fDebugger)
	{
		return nullptr;
	}

	void AnsycSlicer::stop()
	{

	}

	QString AnsycSlicer::generateTempGCodeFileName()
	{
		QString fileName = QString("%1/temp.gcode").arg(SLICE_PATH);
		QFile::remove(fileName);

		return fileName;
	}

	QString AnsycSlicer::generateTempFileName()
	{
		QString fileName = QString("%1/").arg(SLICE_PATH);
		QFile::remove(fileName);

		return fileName;
	}

	SliceResultPointer AnsycSlicer::generateResult(const QString& fileName, qtuser_core::ProgressorTracer& tracer)
	{
		tracer.resetProgressScope(0.8f, 1.0f);
		SliceResultPointer result(new gcode::SliceResult());
		result->load(fileName.toLocal8Bit().data(), &tracer);

		qDebug() << QString("Slice : DLLAnsycSlicer52 load over . [%1]").arg(currentProcessMemory());
		if (result->layerCode().size() == 0)
			result.reset();
		return result;
	}

	QString AnsycSlicer::sceneTempFile()
	{
		QString sceneFile;
#if EXPORT_FDM52_DATA
		QDir dir(TEMPGCODE_PATH);
		dir.setFilter(QDir::Files);
		int fileCount = dir.count();
		for (int i = 0; i < fileCount; i++)
			dir.remove(dir[i]);

		sceneFile = QString("%1/%2-%3").arg(TEMPGCODE_PATH).arg(generateSceneName())
			.arg(QUuid::createUuid().toString(QUuid::Id128));

		qDebug() << QString("sceneTempFile EXPORT_FDM52_DATA -> [%1]").arg(sceneFile);
#endif
		return sceneFile;
	}

	ModelInput* produceModelNSupport(ModelN* model, SliceInput& input)
	{
		return nullptr;
	}

	void produceSliceInput(SliceInput& input)
	{
		input.G = createCurrentGlobalSettings();
		set_software_version(input.G.data(), version());

		input.Es = createCurrentExtruderSettings();

		QList<ModelGroup*> groups = modelGroups();
		for (ModelGroup* group : groups)
		{
			ModelGroupInput* modelGroupInput = nullptr;
			if (input.G->value("print_sequence", "one_at_a_time") == "all_at_once")
			{
				modelGroupInput = new ModelGroupInput(&input);
				modelGroupInput->settings->merge(group->setting());
			}
			{
				QList<ModelN*> models = group->models();
				int index = 0;
				for (ModelN* model : models)
				{
					if (!model->isVisible() || modelOutPlatform(model))
						continue;

					if (input.G->value("print_sequence", "one_at_a_time") == "one_at_a_time")
					{
						modelGroupInput = new ModelGroupInput(&input);
						modelGroupInput->settings->merge(group->setting());
					}

					if (modelGroupInput == nullptr)
					{
						continue;
					}

					ModelNInput* meshInput = new ModelNInput(model, modelGroupInput);
					meshInput->setID(index++);
					modelGroupInput->addModelInput(meshInput);

					bool needAddSurpport = true;
					float angle = input.G->value("special_slope_slice_angle", "0").toFloat();
					QString Axis = input.G->value("special_slope_slice_axis", "X");
					trimesh::vec axis = trimesh::vec(0, 1, 0);
					if (Axis == "X") axis = trimesh::vec(1, 0, 0);
					if (angle != 0)
					{
						meshInput->tiltSliceSet(axis, angle);
						if (input.G->vvalue("support_enable").toBool())
						{
							meshInput->settings->add("support_enable", "false");
							float support_angle = input.G->vvalue("support_angle").toFloat();
							trimesh::TriMesh* supportMesh = meshInput->generateSlopeSupport(angle, axis, support_angle, false);
							if (supportMesh)
							{
								SupportInput* supportInput = new SupportInput(modelGroupInput);
								supportInput->setID(index++);
								supportInput->setName(QString("%1").arg(1));
								supportInput->setModel(model);
								supportInput->setTrimesh(supportMesh);
								supportInput->build();
								supportInput->tiltSliceSet(axis, angle);
								supportInput->settings->add("support_enable", "false");
								supportInput->settings->add("support_mesh", "true");
								supportInput->settings->add("support_mesh_drop_down", "false");
								modelGroupInput->addModelInput(supportInput);
								modelGroupInput->modelInputs.back()->setPtr(supportInput->ptr());
								needAddSurpport = false;
							}
						}
					}

					if (needAddSurpport)
					{
						ModelInput* support = produceModelNSupport(model, input);
						if (support)
							modelGroupInput->addModelInput(support);
					}

					if (input.G->value("print_sequence", "one_at_a_time") == "one_at_a_time")
					{
						input.Groups.push_back(modelGroupInput);
					}
				}
			}
			if (input.G->value("print_sequence", "one_at_a_time") == "all_at_once")
			{
				input.Groups.push_back(modelGroupInput);
			}
		}

		cacheInput(input);
	}

	void produceSliceInput(SliceInput& input, Printer* printer)
	{
		input.G = createCurrentGlobalSettings();
		set_software_version(input.G.data(), version());

		input.Es = createCurrentExtruderSettings();

		ModelGroup* group = modelGroups().first();
		// QList<ModelGroup*> groups = modelGroups();
		// for (ModelGroup* group : groups)
		{
			ModelGroupInput* modelGroupInput = nullptr;
			if (input.G->value("print_sequence", "one_at_a_time") == "all_at_once")
			{
				modelGroupInput = new ModelGroupInput(&input);
				modelGroupInput->settings->merge(group->setting());
			}
			{
				// QList<ModelN*> models = group->models();
				int index = 0;
				for (ModelN* model : printer->modelsInside())
				{
					//if (!model->isVisible() || modelOutPlatform(model))
					if (!model->isVisible())
						continue;

					if (input.G->value("print_sequence", "one_at_a_time") == "one_at_a_time")
					{
						modelGroupInput = new ModelGroupInput(&input);
						modelGroupInput->settings->merge(group->setting());
					}

					if (modelGroupInput == nullptr)
					{
						continue;
					}

					ModelNInput* meshInput = new ModelNInput(model, printer, modelGroupInput);
					meshInput->setID(index++);
					modelGroupInput->addModelInput(meshInput);

					bool needAddSurpport = true;
					float angle = input.G->value("special_slope_slice_angle", "0").toFloat();
					QString Axis = input.G->value("special_slope_slice_axis","X");
					trimesh::vec axis = trimesh::vec(0, 1, 0);
					if (Axis == "X") axis = trimesh::vec(1, 0, 0);
					if (angle != 0)
					{
						meshInput->tiltSliceSet(axis, angle);
						if (input.G->vvalue("support_enable").toBool())
						{
							meshInput->settings->add("support_enable", "false");
							float support_angle = input.G->vvalue("support_angle").toFloat();
							trimesh::TriMesh* supportMesh = meshInput->generateSlopeSupport(angle, axis, support_angle, false);
							if (supportMesh)
							{
								SupportInput* supportInput = new SupportInput(modelGroupInput);
								supportInput->setID(index++);
								supportInput->setName(QString("%1").arg(1));
								supportInput->setModel(model);
								supportInput->setTrimesh(supportMesh);
								supportInput->build();
								supportInput->tiltSliceSet(axis, angle);
								supportInput->settings->add("support_enable", "false");
								supportInput->settings->add("support_mesh", "true");
								supportInput->settings->add("support_mesh_drop_down", "false");
								modelGroupInput->addModelInput(supportInput);
								modelGroupInput->modelInputs.back()->setPtr(supportInput->ptr());
								needAddSurpport = false;
							}
						}
					}

					if (needAddSurpport)
					{
						ModelInput* support = produceModelNSupport(model, input);
						if (support)
							modelGroupInput->addModelInput(support);
					}

					if (input.G->value("print_sequence", "one_at_a_time") == "one_at_a_time")
					{
						input.Groups.push_back(modelGroupInput);
					}
				}
			}
			if (input.G->value("print_sequence", "one_at_a_time") == "all_at_once")
			{
				input.Groups.push_back(modelGroupInput);
			}
		}

		cacheInput(input);
	}

	void produceSliceInput_orca(SliceInput& input, Printer* printer)
	{
		input.G = createCurrentGlobalSettings();
		set_software_version(input.G.data(), version());

		input.Es = createCurrentExtruderSettings();


		{
			int index = 0;
			for (ModelN* model : printer->modelsInside())
			{
				if (!model->isVisible())
					continue;

				if (model->getGroupId() < 0)
				{
					ModelGroupInput* modelGroupInput = new ModelGroupInput(&input);
					modelGroupInput->settings->merge(model->setting());

					ModelNInput* meshInput = new ModelNInput(model, printer, modelGroupInput);
					meshInput->setID(index++);
					meshInput->setName(model->objectName());
					meshInput->setColors2Facets(model->getColors2Facets()); //color
					meshInput->setSeam2Facets(model->getSeam2Facets());
					meshInput->setSupport2Facets(model->getSupport2Facets());

					modelGroupInput->groupId = -1;
					modelGroupInput->addModelInput(meshInput);

					QVector3D pos = printer->globalBox().min;
					QMatrix4x4 printerMatrix;
					printerMatrix.translate(-pos);

					//QMatrix4x4 gm = qtuser_3d::xform2QMatrix(trimesh::fxform(modelGroup->getGroupTransform()));
					//QMatrix4x4 gm = model->globalMatrix();

					//QMatrix4x4 ans = printerMatrix * gm;

					//modelGroupInput->groupTransform = trimesh::xform(qtuser_3d::qMatrix2Xform(ans));

					input.Groups.push_back(modelGroupInput);
				}
				else
				{
					bool haveGroup = false;
					for (ModelGroupInput* mg : input.Groups)
					{
						if (model->getGroupId() == mg->groupId)
						{
							haveGroup = true;

							ModelNInput* meshInput = new ModelNInput(model, printer, mg);
							meshInput->setID(index++);
							meshInput->setName(model->objectName());
							meshInput->setColors2Facets(model->getColors2Facets()); //color
							meshInput->setSeam2Facets(model->getSeam2Facets());
							meshInput->setSupport2Facets(model->getSupport2Facets());

							mg->addModelInput(meshInput);
							//mg->groupTransform = model->getGroupTransform();

							break;

						}
					}

					if (!haveGroup)
					{
						ModelGroupInput* modelGroupInput = new ModelGroupInput(&input);
						modelGroupInput->settings->merge(model->setting());

						ModelNInput* meshInput = new ModelNInput(model, printer, modelGroupInput);
						meshInput->setID(index++);
						meshInput->setName(model->objectName());
						meshInput->setColors2Facets(model->getColors2Facets()); //color
						meshInput->setSeam2Facets(model->getSeam2Facets());
						meshInput->setSupport2Facets(model->getSupport2Facets());

						modelGroupInput->groupId = model->getGroupId();
						//modelGroupInput->groupTransform = model->getGroupTransform();
						modelGroupInput->addModelInput(meshInput);
						input.Groups.push_back(modelGroupInput);
					}
				}

			}

		}


		cacheInput(input);
	}

	void produceSliceInput(SliceInput& input, Printer* printer, EngineType engine_type)
	{
		switch (engine_type)
		{
		case creative_kernel::EngineType::ET_ORCA:
			produceSliceInput_orca(input, printer);
			break;
		default:
			produceSliceInput(input, printer);
			break;
		}
	}

	void cacheInput(const SliceInput& input)
	{
#if 1 // _DEBUG
		clearPath(SLICE_PATH);
		QString globalFile = QString("%1/global").arg(SLICE_PATH);
		input.G->saveAsDefault(globalFile);
		for (int i = 0; i < input.Es.count(); ++i)
		{
			QString extruderFile = QString("%1/extruder_%2").arg(SLICE_PATH).arg(i);
			input.Es.at(i)->saveAsDefault(extruderFile);
		}
		// for (int i = 0; i < groups.count(); ++i)
		for (int i = 0; i < 1; ++i)
		{
			if (!input.Groups.empty())
			{
				ModelGroupInput* groupInput = input.Groups.at(i);
				QString groupFile = QString("%1/group_%2").arg(SLICE_PATH).arg(i);
				groupInput->settings->saveAsDefault(groupFile);

				for (int j = 0; j < groupInput->modelInputs.count(); ++j)
				{
					ModelInput* modelInput = groupInput->modelInputs.at(j);
					QString modelFile = QString("%1/group_%2_model_%3").arg(SLICE_PATH).arg(i).arg(j);
					modelInput->settings->saveAsDefault(modelFile);
				}
			}
		}
#endif
	}
}
