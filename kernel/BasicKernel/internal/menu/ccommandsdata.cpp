#include "ccommandsdata.h"
#include <QVariant>
#include "menu/ccommandlist.h"
#include <QVariantList>
#include <QMap>
#include <QString>
#include <QDebug>

#include "us/settingdef.h"

#include "openfilecommand.h"
#include "importpresetcommand.h"
#include "exportpresetcommand.h"
#include "submenurecentfiles.h"

#include "undoactioncmd.h"
#include "redoactioncmd.h"

#include "../menu_help/aboutuscommand.h"
#include "../menu_help/updatecommand.h"

#include "../menu_calibration/temperaturecommand.h"
#include "../menu_calibration/pacommand.h"
#include "../menu_calibration/maxflowvolumecommand.h"
#include "../menu_calibration/vfacommand.h"
#include "../menu_calibration/flowcoarsetuningcommand.h"
#include "../menu_calibration/flowfinetuningcommand.h"
#include "../menu_calibration/tutorialcommand.h"
#include "../menu_calibration/retractioncommand.h"
#include "../menu_calibration/retractiondistancecommand.h"
#include "../menu_calibration/retractionspeedcommand.h"
#include "../menu_calibration/fanspeedcommand.h"
#include "../menu_calibration/jitterspeedcommand.h"
#include "../menu_calibration/maxspeedcommand.h"
#include "../menu_calibration/speedtowercommand.h"
#include "../menu_calibration/slowaccelerationcommand.h"
#include "../menu_calibration/maxaccelerationcommand.h"
#include "../menu_calibration/accelerationtowercommand.h"
#include "../menu_calibration/arcfittingcommand.h"

#include "submenuexportmodel.h"
#include "submenuimportmodel.h"
#include "submenustandardmodel.h"
#include "submenutestmodel.h"
#include "../menu_view/modelshowcommand.h"
#include "../menu_view/mirroractioncommand.h"
#include "../menu/saveascommand.h"
#include "../project_3mf/save3mf.h"
#include "../project_3mf/load3mf.h"
#include "../menu_tool/submenulanguage.h"
#include "../menu_view/submenuviewsshow.h"
#include "../menu_view/resetactioncommand.h"
#include "../menu_view/mergemodelcommand.h"
#include "../menu_tool/submenuThemeColor.h"
#include "../menu_tool/logviewcommand.h"
#include "../menu_tool/crealitygroupcommand.h"
#include "../menu_tool/preferencescommand.h"
#include"../menu_help/usecoursecommand.h"
#include "../menu_help/userfeedbackcommand.h"
#include "../project_cx3d/cx3dmanager.h"
#include"internal/rclick_menu/deletemodelaction.h"
#include"internal/rclick_menu/clearallaction.h"
#include"internal/rclick_menu/mergemodelaction.h"
#include"internal/rclick_menu/mergemodellocation.h"
#include"internal/rclick_menu/splitmodelaction.h"
#include"internal/rclick_menu/cloneaction.h"
#include"internal/rclick_menu/selectallaction.h"
#include"internal/rclick_menu/resetallaction.h"
#include <QStandardPaths>

#include "interface/commandinterface.h"
#include "qtusercore/string/resourcesfinder.h"

using namespace creative_kernel;
CCommandsData::CCommandsData(QObject *parent) : QObject(parent)
{
    addCommonCommand();

    addUIVisualTracer(this,this);
}

CCommandsData::~CCommandsData()
{
}

void CCommandsData::onThemeChanged(ThemeCategory category)
{
}

void CCommandsData::onLanguageChanged(MultiLanguage language)
{

}


void CCommandsData::addCommads(ActionCommand *pAction)
{
    if (pAction)
    {
        m_varCommandList.push_back(pAction);
    }
}
QVariantList  CCommandsData::getCommandsList(void)
{
    QVariantList varList;
    varList.clear();
    for (int i = 0; i < m_varCommandList.count(); ++i)
    {
        varList.push_back(QVariant::fromValue(m_varCommandList.at(i)));
    }
    return varList;
}


void CCommandsData::addCommonCommand()
{
    OpenFileCommand* openFile = new OpenFileCommand();
    openFile->setParent(this);
    CCommandList::getInstance()->addActionCommad(openFile);
    m_FileOpenOpt = openFile;

    SubMenuRecentFiles* rectfile = SubMenuRecentFiles::getInstance();
    rectfile->setParent(this);
    CCommandList::getInstance()->addActionCommad(rectfile);
    rectfile->setNumOfRecentFiles(10);

    //Save3MFCommand* save3MF = new Save3MFCommand();
    //save3MF->setParent(this);
    //save3MF->setBSeparator(true);
    //CCommandList::getInstance()->addActionCommad(save3MF);

    Load3MFCommand* load3MF = new Load3MFCommand();
    load3MF->setParent(this);
    load3MF->setBSeparator(true);
    CCommandList::getInstance()->addActionCommad(load3MF);

    SaveAsCommand* saveAs = new SaveAsCommand();
    saveAs->setParent(this);
    saveAs->setBSeparator(true);
    CCommandList::getInstance()->addActionCommad(saveAs);

    SubMenuStandardModel* ssm = new SubMenuStandardModel();
    ssm->setParent(this);
    ssm->setBSeparator(true);
    CCommandList::getInstance()->addActionCommad(ssm);

    SubMenuTestModel* stm = SubMenuTestModel::getInstance();
    stm->setParent(this);
    stm->setBSeparator(true);
    CCommandList::getInstance()->addActionCommad(stm);


    SubMenuImportModel* subImport = SubMenuImportModel::getInstance();
    subImport->setParent(this);
    subImport->setBSeparator(true);
    CCommandList::getInstance()->addActionCommad(subImport);
    
    SubMenuExportModel* subExport = SubMenuExportModel::getInstance();
    subExport->setParent(this);
    subExport->setBSeparator(true);
    CCommandList::getInstance()->addActionCommad(subExport);

    //ImportPresetCommand* importPresetCmd = new ImportPresetCommand();
    //importPresetCmd->setParent(this);
    //importPresetCmd->setBSeparator(true);
    //CCommandList::getInstance()->addActionCommad(importPresetCmd);

    //ExportPresetCommand* exportPresetCmd = new ExportPresetCommand();
    //exportPresetCmd->setParent(this);
    //exportPresetCmd->setBSeparator(true);
    //CCommandList::getInstance()->addActionCommad(exportPresetCmd);
    //edit
    UndoActionCmd* undo = new UndoActionCmd();
    undo->setParent(this);
    CCommandList::getInstance()->addActionCommad(undo);

    RedoActionCmd* redo = new RedoActionCmd();
    redo->setParent(this);
    CCommandList::getInstance()->addActionCommad(redo);

    DeleteModelAction* dma = new DeleteModelAction(this);
    CCommandList::getInstance()->addActionCommad(dma);

    ClearAllAction* clearAll = new ClearAllAction(this);
    CCommandList::getInstance()->addActionCommad(clearAll);

    CloneAction* clone = new CloneAction(this);
    CCommandList::getInstance()->addActionCommad(clone);

    SplitModelAction* split = new SplitModelAction(this);
    CCommandList::getInstance()->addActionCommad(split);

    MergeModelAction* merge = new MergeModelAction(this);
    CCommandList::getInstance()->addActionCommad(merge);

    MergeModelLocation* mergeLocation = new MergeModelLocation(this);
    CCommandList::getInstance()->addActionCommad(mergeLocation);

    MergeModelCommand* mergeModel = new MergeModelCommand(this);
    CCommandList::getInstance()->addActionCommad(mergeModel);

    SelectAllAction* selectAll = new SelectAllAction(this);
    CCommandList::getInstance()->addActionCommad(selectAll);

    ResetAllAction* resetall = new ResetAllAction(this);
    CCommandList::getInstance()->addActionCommad(resetall);

    //view
    ModelShowCommand* lineShow = new ModelShowCommand(ModelVisualMode::mvm_line, this);
    CCommandList::getInstance()->addActionCommad(lineShow);
    lineShow->setBSeparator(true);
    ModelShowCommand* faceShow = new ModelShowCommand(ModelVisualMode::mvm_face, this);
    CCommandList::getInstance()->addActionCommad(faceShow);
    ModelShowCommand* faceLineShow = new ModelShowCommand(ModelVisualMode::mvm_line_and_face, this);
    CCommandList::getInstance()->addActionCommad(faceLineShow);

    MirrorActionCommand* mirror_x = new MirrorActionCommand(MirrorOperation::mo_x, this);
    CCommandList::getInstance()->addActionCommad(mirror_x);
    mirror_x->setBSeparator(true);

    MirrorActionCommand* mirror_y = new MirrorActionCommand(MirrorOperation::mo_y,this);
    CCommandList::getInstance()->addActionCommad(mirror_y);

    MirrorActionCommand* mirror_z = new MirrorActionCommand(MirrorOperation::mo_z, this);
    CCommandList::getInstance()->addActionCommad(mirror_z);

    MirrorActionCommand* mirror_reset = new MirrorActionCommand(MirrorOperation::mo_reset, this);
    CCommandList::getInstance()->addActionCommad(mirror_reset);

    SubMenuViewShow* viewshow = new SubMenuViewShow();
    viewshow->setParent(this);
    CCommandList::getInstance()->addActionCommad(viewshow);
    //    viewshow->setBSeparator(true);

    ResetActionCommand* resetAll = new ResetActionCommand();
    resetAll->setParent(this);
    CCommandList::getInstance()->addActionCommad(resetAll);

    //MergeModelCommand* mergeModel = new MergeModelCommand();
    //mergeModel->setParent(this);
    //CCommandList::getInstance()->addActionCommad(mergeModel);
    //resetAll->setBSeparator(true);

    //Tool
    SubMenuLanguage* lang = new SubMenuLanguage();
    lang->setParent(this);
    CCommandList::getInstance()->addActionCommad(lang);

    SubMenuThemeColor* themecolor = new SubMenuThemeColor();
    themecolor->setParent(this);
    CCommandList::getInstance()->addActionCommad(themecolor);
    LogViewCommand* logview = new LogViewCommand(this);
    CCommandList::getInstance()->addActionCommad(logview);

    //group
    CrealityGroupCommand* group = new CrealityGroupCommand();
    group->setParent(this);
    CCommandList::getInstance()->addActionCommad(group);

    //perferences
    PreferencesCommand* preferences = new PreferencesCommand();
    preferences->setParent(this);
    CCommandList::getInstance()->addActionCommad(preferences);

    //help
    AboutUsCommand* aboutus = new AboutUsCommand(this);
    CCommandList::getInstance()->addActionCommad(aboutus);
    UpdateCommand* update = new UpdateCommand(this);
    CCommandList::getInstance()->addActionCommad(update);
    UseCourseCommand* usecourse = new UseCourseCommand(this);
    CCommandList::getInstance()->addActionCommad(usecourse);
    UserFeedbackCommand* userfeedback = new UserFeedbackCommand(this);
    CCommandList::getInstance()->addActionCommad(userfeedback);

    //calibration
    TemperatureCommand* temperature = new TemperatureCommand(this);
    CCommandList::getInstance()->addActionCommad(temperature);
    PACommand* pa = new PACommand(this);
    CCommandList::getInstance()->addActionCommad(pa);
    MaxFlowVolumeCommand* mfv = new MaxFlowVolumeCommand(this);
    CCommandList::getInstance()->addActionCommad(mfv);
    VFACommand* vfa = new VFACommand(this);
    CCommandList::getInstance()->addActionCommad(vfa);
    FlowCoarseTuningCommand* fct = new FlowCoarseTuningCommand(this);
    CCommandList::getInstance()->addActionCommad(fct);
    FlowFineTuningCommand* fft = new FlowFineTuningCommand(this);
    CCommandList::getInstance()->addActionCommad(fft);
    TutorialCommand* tutorial = new TutorialCommand(this);
    CCommandList::getInstance()->addActionCommad(tutorial);
    RetractionCommand* rc = new RetractionCommand(this);
    CCommandList::getInstance()->addActionCommad(rc);
    RetractionDistanceCommand* rd = new RetractionDistanceCommand(this);
    CCommandList::getInstance()->addActionCommad(rd);
    RetractionSpeedCommand* rs = new RetractionSpeedCommand(this);
    CCommandList::getInstance()->addActionCommad(rs);
    FanSpeedCommand* fsc = new FanSpeedCommand(this);
    CCommandList::getInstance()->addActionCommad(fsc);
    JitterSpeedCommand* jsc = new JitterSpeedCommand(this);
    CCommandList::getInstance()->addActionCommad(jsc);
    MaxSpeedCommand* msc = new MaxSpeedCommand(this);
    CCommandList::getInstance()->addActionCommad(msc);
    SpeedTowerCommand* stc = new SpeedTowerCommand(this);
    CCommandList::getInstance()->addActionCommad(stc);
    AccelerationTowerCommand* atc = new AccelerationTowerCommand(this);
    CCommandList::getInstance()->addActionCommad(atc);
    MaxAccelerationCommand* mac = new MaxAccelerationCommand(this);
    CCommandList::getInstance()->addActionCommad(mac);
    SlowAccelerationCommand* sac = new SlowAccelerationCommand(this);
    CCommandList::getInstance()->addActionCommad(sac);
    ArcFittingCommand* afc = new ArcFittingCommand(this);
    CCommandList::getInstance()->addActionCommad(afc);



}

QVariant CCommandsData::getFileOpenOpt()
{
    return QVariant::fromValue(m_FileOpenOpt);
}

QVariant CCommandsData::getOpt(const QString& optName)
{
    QList<ActionCommand *> commandList = CCommandList::getInstance()->getActionCommandList();
    QVariant res;
    foreach(auto command, commandList)
    {
        if(command->nameWithout() == optName)
        {
            res = QVariant::fromValue(command);
            break;
        }
    }

    return res;
}
