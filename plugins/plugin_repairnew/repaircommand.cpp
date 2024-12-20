#include "repaircommand.h"
#include "repairop.h"
#include "cmesh/mesh/repair.h"

#include "interface/spaceinterface.h"
#include "interface/visualsceneinterface.h"
#include "kernel/translator.h"

#include "kernel/kernelui.h"
#include "data/modeln.h" 
#include "interface/selectorinterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

using namespace creative_kernel;
RepairCommand::RepairCommand()
	: m_op(nullptr)
{
	m_repairModels.clear();
	m_shortcut = "Ctrl+R";
    m_actionname = tr("Model Repair");
    m_actionNameWithout = "Model Repair";
	m_strMessageText = tr("Please import model");
	m_eParentMenu = eMenuType_Tool;//eMenuType_Repair;

	addUIVisualTracer(this,this);
}

RepairCommand::~RepairCommand()
{
	bool ret = disconnect(m_op, SIGNAL(repairSuccess()), this, SLOT(slotRepairSuccess()));
}

void RepairCommand::slotRepair()
{
	execute();
}

void RepairCommand::onThemeChanged(creative_kernel::ThemeCategory category)
{

}

void RepairCommand::onLanguageChanged(creative_kernel::MultiLanguage language)
{
	m_actionname = tr("Model Repair") + "        " + m_shortcut;
	m_strMessageText = tr("Please import model");
}

void RepairCommand::execute()
{
	m_nverticessizeAll = 0;
	m_facesizeAll = 0;
	m_errorNormalsAll = 0;
	m_errorEdgesAll = 0;
	m_shellsAll = 0;
	m_holesAll = 0;
	m_intersectsAll = 0;
	if (!haveModelN())
	{
		requestQmlDialog(this, "messageSingleBtnDlg");
		return;
	}

	QList<ModelN*> selections = getRepairableModels();
	if (selections.size() < 1)
	{
		requestQmlDialog(this, "messageSingleBtnDlg");
		return;
	}

	foreach(creative_kernel::ModelN * model, selections)
	{
		trimesh::TriMesh* mesh = model->mesh();
		int verticessize = mesh->vertices.size();
		int facesize = mesh->faces.size();

		cmesh::ErrorInfo info;
		cmesh::getErrorInfo(model->mesh(), info);

		//model->setErrorEdges(info.edgeNum);
		//model->setErrorNormals(info.normalNum);
		//model->setErrorHoles(info.holeNum);
		//model->setErrorIntersects(info.intersectNum);

		int errorNormals = info.normalNum;
		int errorEdges = info.edgeNum;
		int errorHoles = info.holeNum;
		int errorIntersect = info.intersectNum;

		if (errorEdges > 0 || errorNormals > 0 || errorHoles > 0 || errorIntersect > 0)
		{
			//m_nverticessizeAll += verticessize;
			//m_facesizeAll += facesize;
			m_errorNormalsAll += errorNormals;
			m_errorEdgesAll += errorEdges;
			m_holesAll += errorHoles;
			m_intersectsAll += errorIntersect;
			//errorCnt++;
		}
	}

	requestQmlDialog(this, "repaircmdDlg");

}

void RepairCommand::slotRepairSuccess()
{
	m_strMessageText = tr("Repair Model Finish!!!");
	requestQmlDialog(this, "messageSingleBtnDlg");
	
	bool ret = disconnect(m_op, SIGNAL(repairSuccess()), this, SLOT(slotRepairSuccess()));
	//getKernelUI()->requestMenuDialog(this, "messageSingleBtnDlg");
	//QMetaObject::invokeMethod(getKernelUI()->footer(), "showJobFinishMessage", Q_ARG(QVariant, QVariant::fromValue(this)), Q_ARG(QVariant, QVariant::fromValue(str)));
	delete m_op;
	m_op = nullptr;
}
QString RepairCommand::getMessageText()
{
	return m_strMessageText;
}

void RepairCommand::acceptRepair()
{
	QList<ModelN*> selections = getRepairableModels();

	if (!m_op)
	{
		m_op = new RepairOp(this);
	}
	m_op->repairModel(selections);

	bool ret = disconnect(m_op, SIGNAL(repairSuccess()), this, SLOT(slotRepairSuccess()));
	connect(m_op, SIGNAL(repairSuccess()), this, SLOT(slotRepairSuccess()));
}
void RepairCommand::accept()
{
	

	//update info
	QObject* pinfoshowObj = getKernelUI()->getUI("UIRoot")->findChild<QObject*>("infoshowObj");
	QMetaObject::invokeMethod(pinfoshowObj, "updateInfo");
}

void RepairCommand::slotsDetectModel()
{
	qDebug() << "detect model";
	QList<ModelN*> selections = selectionms();

	m_repairModels.clear();
	for (int i=0; i<selections.size(); ++i)
	{
		ModelN* m = selections.at(i);
		trimesh::TriMesh* choosedMesh = m->mesh();
		//if (judgeModelHasBoundaries(choosedMesh))
		{
			m_repairModels.push_back(m);
		}
	}
	if(m_repairModels.size() > 0)
		requestQmlDialog(this, "modelRepairMessageDlg");
}

QList<creative_kernel::ModelN*> RepairCommand::getRepairableModels()
{
    QList<ModelN*> selections;
    QList<ModelGroup*> group_selections = selectedGroups();
    if (!group_selections.isEmpty())
    {
        for (int i = 0; i < group_selections.size(); ++i)
        {
            QList<ModelN*> models = group_selections[i]->models();
            selections.append(models);
        }
    }
    else
    {
        selections = selectionms();
    }

    for (int i = selections.size() - 1; i >= 0; --i)
    {
        ModelN* m = selections[i];
        if (m->modelNType() != ModelNType::normal_part)
        {
            selections.removeAt(i);
        }
    }
    return selections;
}

void RepairCommand::repairModel()
{
	if (m_repairModels.size() < 1)
	{
		return;
	}

	if (!m_op)
	{
		m_op = new RepairOp(this);
		m_op->repairModel(m_repairModels);
	}

	bool ret = disconnect(m_op, SIGNAL(repairSuccess()), this, SLOT(slotRepairSuccess()));
	connect(m_op, SIGNAL(repairSuccess()), this, SLOT(slotRepairSuccess()));
	setVisOperationMode(m_op);
}

void RepairCommand::delSelectedModel()
{
	removeSelections();
}
