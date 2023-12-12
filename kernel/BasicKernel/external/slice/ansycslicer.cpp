#include "ansycslicer.h"
#include "data/modeln.h"
#include "data/fdmsupportgroup.h"

#include "supportinput.h"
#include "modelninput.h"

#include "qtusercore/module/systemutil.h"
#include "qtuser3d/trimesh2/conv.h"

#include "interface/spaceinterface.h"
#include "interface/machineinterface.h"
#include "interface/appsettinginterface.h"

namespace creative_kernel
{
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

	SliceResultPointer AnsycSlicer::doSlice(SliceInput& input, qtuser_core::ProgressorTracer& tracer, crslice::PathData* _fDebugger)
	{
		return nullptr;
	}

	void AnsycSlicer::stop()
	{

	}

	ModelInput* produceModelNSupport(ModelN* model, SliceInput& input)
	{
		if (!model || !model->hasFDMSupport())
			return nullptr;

		SettingsPointer settings = input.G;
		TriMeshPtr mesh(model->fdmSupport()->createFDMSupportMesh());
		if (mesh)
		{
			ModelInput* supportInput = new ModelInput();

			trimesh::fxform xf = qtuser_3d::qMatrix2Xform(model->globalMatrix());
			trimesh::apply_xform(mesh.get(), trimesh::xform(xf));

			supportInput->setPtr(mesh);
			supportInput->settings->add("support_enable", "true");
			supportInput->settings->add("support_mesh", "true");
			supportInput->settings->add("support_mesh_drop_down", "false");
			return supportInput;
		}
		return nullptr;
	}

	void produceSliceInput(SliceInput& input)
	{
		input.G = createCurrentGlobalSettings();
		input.G->add("software_version", version(), true);

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

#if 1 // _DEBUG
		clearPath(SLICE_PATH);
		QString globalFile = QString("%1/global").arg(SLICE_PATH);
		input.G->saveAsDefault(globalFile);
		for (int i = 0; i < input.Es.count(); ++i)
		{
			QString extruderFile = QString("%1/extruder_%2").arg(SLICE_PATH).arg(i);
			input.Es.at(i)->saveAsDefault(extruderFile);
		}
		for (int i = 0; i < groups.count(); ++i)
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
#endif
	}
}
