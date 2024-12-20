#include "ansycslicer.h"
#include "data/modeln.h"
#include "data/modelgroup.h"
#include "data/kernelmacro.h"

#include "modelninput.h"
#include "ansycworker.h"

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

	QString generateTempFileName()
	{
		QString fileName = QString("%1/").arg(SLICE_PATH);
		QFile::remove(fileName);

		return fileName;
	}

	SliceResultPointer generateResult(const QString& fileName, qtuser_core::ProgressorTracer& tracer)
	{
		tracer.resetProgressScope(0.8f, 1.0f);
		SliceResultPointer result(new gcode::SliceResult());
		result->load(fileName.toLocal8Bit().data(), &tracer);

		qDebug() << QString("Slice : DLLAnsycSlicer52 load over . [%1]").arg(currentProcessMemory());
		if (result->layerCode().size() == 0)
			result.reset();
		return result;
	}

	QString sceneTempFile()
	{
		QString sceneFile;
#if EXPORT_FDM52_DATA
		QDir dir(TEMPGCODE_PATH);
		dir.setFilter(QDir::Files);
		int fileCount = dir.count();
		for (int i = 0; i < fileCount; i++)
			dir.remove(dir[i]);

		sceneFile = QString("%1/%2").arg(TEMPGCODE_PATH).arg(QUuid::createUuid().toString(QUuid::Id128));
		qDebug() << QString("sceneTempFile EXPORT_FDM52_DATA -> [%1]").arg(sceneFile);
#endif
		return sceneFile;
	}

	void produceSliceInput_orca(SliceInput& input, Printer* printer)
	{
		input.G = createCurrentGlobalSettings();
		set_software_version(input.G.data(), version());

		input.Es = createCurrentExtruderSettings();


		{
			int index = 0;

			for (ModelGroup* modelGroup : printer->modelGroupsInside())
			{
				if (!modelGroup->isVisible())
					continue;

				QList<ModelN*> models = modelGroup->models();
				bool have_visible = false;
				for (ModelN* model : models)
				{
					if (model->isVisible())
					{
						have_visible = true;
						break;
					}
				}

				if (!have_visible)
					continue;

				ModelGroupInput* modelGroupInput = new ModelGroupInput(&input);

				QVector3D pos = printer->globalBox().min;
				trimesh::xform printerXf = trimesh::xform::trans(-pos.x(), -pos.y(), -pos.z());

				modelGroupInput->groupTransform = printerXf * modelGroup->getMatrix();
				modelGroupInput->settings->merge(modelGroup->setting());
				modelGroupInput->groupId = modelGroup->getObjectId();

				for (ModelN* model : models)
				{
					if (!model->isVisible())
						continue;

					ModelNInput* meshInput = new ModelNInput(model, printer, modelGroupInput);
					meshInput->setID(index++);
					meshInput->setName(model->name());
					meshInput->setColors2Facets(model->getColors2Facets()); //color
					meshInput->setSeam2Facets(model->getSeam2Facets());
					meshInput->setSupport2Facets(model->getSupport2Facets());
					meshInput->setModelType((int)model->modelNType());

					modelGroupInput->addModelInput(meshInput);
				}

				input.Groups.push_back(modelGroupInput);
			}

		}

		cacheInput(input);
	}

	void produceSliceInput(SliceInput& input, Printer* printer, EngineType engine_type)
	{
		produceSliceInput_orca(input, printer);
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
