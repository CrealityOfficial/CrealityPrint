#include "lettercommand.h"

#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include <QCoreApplication>
#include <QDirIterator>

#include <QStandardPaths>
#include "kernel/kernel.h"
#include "kernel/kernelui.h"
#include "interface/uiinterface.h"
#include "qtusercore/string/resourcesfinder.h"
#include "interface/commandinterface.h"
#include "letterjob.h"
#include "cxkernel/interface/jobsinterface.h"
#include "operation/moveoperatemode.h"

LetterCommand::LetterCommand(QObject* parent) : ToolCommand(parent)
{
	orderindex = 6;
	m_model = new cxkernel::LetterModel(this);	
}

LetterCommand::~LetterCommand()
{

}

void LetterCommand::execute()
{
	// find fonts
	QStringList search_paths;
	search_paths.append(QCoreApplication::applicationDirPath());
	search_paths.append(QCoreApplication::applicationDirPath() + "/resources");
	int index = QCoreApplication::applicationDirPath().lastIndexOf("/");
	search_paths.append(QCoreApplication::applicationDirPath().left(index) + "/Resources/resources");
	search_paths.append(qtuser_core::getOrCreateAppDataLocation("") + "/resources");

	m_listFonts.clear();

	for (QString& dir : search_paths)
	{
		QDirIterator iter(dir + "/fonts", QDirIterator::IteratorFlag::NoIteratorFlags);
		while (iter.hasNext())
		{
			iter.next();

			QFileInfo fileinfo = iter.fileInfo();
			if (fileinfo.isFile() && fileinfo.suffix() == "ttf")
			{
				m_listFonts.append(fileinfo.baseName());
			}
		}
	}
	m_listFonts = m_listFonts.toSet().toList();

	if (m_mode == NULL)
	{
		m_mode = new creative_kernel::MoveOperateMode(this); 
		m_mode->setType(qtuser_3d::SceneOperateMode::FixedMode);
	}

	creative_kernel::setVisOperationMode(m_mode);
	creative_kernel::sensorAnlyticsTrace("Model Editing & Layout", "Letter");
}

bool LetterCommand::checkSelect()
{
	return creative_kernel::selectionms().size() > 0;
}

QStringList LetterCommand::getFontList()
{
	return m_listFonts;
}

float LetterCommand::getThickness()
{
	if (m_model)
		return 0.05f;
		//return m_model->textThickness();
	return 0.0f;
}

void LetterCommand::setThickness(float the_thickness)
{
	if (m_model)
		m_model->setTextThickness(the_thickness);
}

int LetterCommand::getTextSide()
{
	if (m_model)
		return m_model->textSide();
	return 0;
}

void LetterCommand::setTextSide(int the_text_side)
{
	if (m_model)
		m_model->setTextSide(the_text_side);
}

void LetterCommand::generatePolygonData(const QList<QObject*>& objectList)
{
	/*if (creative_kernel::notifyDeleteSupports(creative_kernel::selectionms()))
		return;
	if (objectList.size() <= 0)
		return;*/

	m_model->parseQmlData(objectList);
	
	auto modelList = creative_kernel::selectionms();
	if (modelList.isEmpty())
	{
		getKernelUI()->requestQmlTipDialog(tr("Please select the model first."));
		return;
	}

	auto modelgroup = creative_kernel::selectedGroups();
	if (modelgroup.isEmpty())
	{
		getKernelUI()->requestQmlTipDialog(tr("Please select the model first."));
		return;
	}

	{
		getKernelUI()->setAutoResetOperateMode(false);
		LetterJob* job = new LetterJob(this);
		job->SetModelGroup(modelgroup[0]);
		job->SetLetterModel(m_model);
		job->SetObjects(objectList);
		connect(job, &LetterJob::finished, this, [=]()
			{
				getKernelUI()->setAutoResetOperateMode(true);
			});
		cxkernel::executeJob(qtuser_core::JobPtr(job)); 
	}

	//for (QList<creative_kernel::ModelN*>::iterator it = modelList.begin(); it != modelList.end(); ++it)
	//{//creative_kernel::ModelN* model = modelList.first();
	//	creative_kernel::ModelN* model = *it;
	//	if (model->modelNType() != creative_kernel::ModelNType::normal_part)
	//		continue;

	//	getKernelUI()->setAutoResetOperateMode(false);

	//	LetterJob* job = new LetterJob(this);
	//	job->SetModel(model);
	//	job->SetLetterModel(m_model);
	//	job->SetObjects(objectList);
	//	connect(job, &LetterJob::finished, this, [=]()
	//		{
	//			getKernelUI()->setAutoResetOperateMode(true);
	//		});
	//	cxkernel::executeJob(qtuser_core::JobPtr(job));
	//}	

}
