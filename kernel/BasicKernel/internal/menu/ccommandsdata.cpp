#include "ccommandsdata.h"
#include <QVariant>
#include "menu/ccommandlist.h"
#include <QVariantList>
#include <QMap>
#include <QString>
#include <QDebug>

#include "us/settingdef.h"

#include "openfilecommand.h"
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




#include "submenustandardmodel.h"
#include "../menu_view/modelshowcommand.h"
#include "../menu_view/mirroractioncommand.h"
#include "../menu/saveascommand.h"
#include "../menu_tool/submenulanguage.h"
#include "../menu_view/submenuviewsshow.h"
#include "../menu_view/resetactioncommand.h"
#include "../menu_view/mergemodelcommand.h"
#include "../menu_tool/manageprinter.h"
#include "../menu_tool/submenuThemeColor.h"
#include "../menu_tool/logviewcommand.h"
#include "../menu_tool/crealitygroupcommand.h"
#include"../menu_help/usecoursecommand.h"
#include "../menu_help/userfeedbackcommand.h"
#include"internal/rclick_menu/deletemodelaction.h"
#include"internal/rclick_menu/clearallaction.h"
#include"internal/rclick_menu/mergemodelaction.h"
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
    createMenuNameMap();

    addUIVisualTracer(this);
}

CCommandsData::~CCommandsData()
{
}

void CCommandsData::onThemeChanged(ThemeCategory category)
{
}

void CCommandsData::onLanguageChanged(MultiLanguage language)
{
    m_mapMenuName.clear();
    createMenuNameMap();
}

void CCommandsData:: createMenuNameMap()
{
    //  int index = eMenuType_File;
    m_mapMenuName.clear();
    m_mapMenuName.insert(QString::number(eMenuType_File),tr("File(&F)"));
    m_mapMenuName.insert(QString::number(eMenuType_Edit),tr("Edit(&E)"));
    m_mapMenuName.insert(QString::number(eMenuType_View),tr("View(&V)"));
    //m_mapMenuName.insert(QString::number(eMenuType_Repair),tr("Repair(&R)"));
    //    m_mapMenuName.insert(QString::number(eMenuType_Slice),tr("Slice(&R)"));
    m_mapMenuName.insert(QString::number(eMenuType_Tool),tr("Tool(&T)"));
    m_mapMenuName.insert(QString::number(eMenuType_CrealityGroup),tr("Models(&M)"));
    m_mapMenuName.insert(QString::number(eMenuType_PrinterControl), tr("PrinterControl(&C)"));
    m_mapMenuName.insert(QString::number(eMenuType_Calibration), tr("Calibration(&C)"));
    m_mapMenuName.insert(QString::number(eMenuType_Help),tr("Help(&H)"));
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

QString CCommandsData::getMenuNameFromKey(QString key)
{
    //m_mapMenuName.value(key);
    return m_mapMenuName.value(key);
}
void CCommandsData::initMapData(QVariantMap &map)
{
    QVariantList fileList;
    QVariantList editList;
    QVariantList viewList;
    QVariantList repairList;
    QVariantList sliceList;
    QVariantList toolList;
    QVariantList groupList;
    QVariantList printerControlList;
    QVariantList helpList;
    QVariantList calibrationList;
    map.insert(QString::number(eMenuType_File),fileList);
    map.insert(QString::number(eMenuType_Edit),editList);
    map.insert(QString::number(eMenuType_View),viewList);
    //map.insert(QString::number(eMenuType_Repair),repairList);
    //    map.insert(QString::number(eMenuType_Slice),sliceList);
    map.insert(QString::number(eMenuType_Tool),toolList);
    map.insert(QString::number(eMenuType_CrealityGroup),groupList);
    map.insert(QString::number(eMenuType_PrinterControl), printerControlList);
    map.insert(QString::number(eMenuType_Help),helpList);
    map.insert(QString::number(eMenuType_Calibration), calibrationList);

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

    SaveAsCommand* saveAs = new SaveAsCommand();
    saveAs->setParent(this);
    saveAs->setBSeparator(true);
    CCommandList::getInstance()->addActionCommad(saveAs);

    SubMenuStandardModel* ssm = new SubMenuStandardModel();
    ssm->setParent(this);
    ssm->setBSeparator(true);
    CCommandList::getInstance()->addActionCommad(ssm);
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

    MergeModelCommand* mergeModel = new MergeModelCommand(this);
    CCommandList::getInstance()->addActionCommad(mergeModel);

    SelectAllAction* selectAll = new SelectAllAction(this);
    CCommandList::getInstance()->addActionCommad(selectAll);

    //ResetAllAction* resetall = new ResetAllAction(this);
    //CCommandList::getInstance()->addActionCommad(resetall);

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

    //ResetActionCommand* resetAll = new ResetActionCommand();
    //resetAll->setParent(this);
    //CCommandList::getInstance()->addActionCommad(resetAll);

    //MergeModelCommand* mergeModel = new MergeModelCommand();
    //mergeModel->setParent(this);
    //CCommandList::getInstance()->addActionCommad(mergeModel);
    //resetAll->setBSeparator(true);

    //Tool
    SubMenuLanguage* lang = new SubMenuLanguage();
    lang->setParent(this);
    CCommandList::getInstance()->addActionCommad(lang);

    ManagePrinter* printer = new ManagePrinter();
    printer->setParent(this);
    CCommandList::getInstance()->addActionCommad(printer);
    SubMenuThemeColor* themecolor = new SubMenuThemeColor();
    themecolor->setParent(this);
    CCommandList::getInstance()->addActionCommad(themecolor);
    LogViewCommand* logview = new LogViewCommand(this);
    CCommandList::getInstance()->addActionCommad(logview);

    //group
    CrealityGroupCommand* group = new CrealityGroupCommand();
    group->setParent(this);
    CCommandList::getInstance()->addActionCommad(group);

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
}

QVariantMap CCommandsData::getCommandsMap(void)
{
    QVariantMap dataMap;

    initMapData(dataMap);
    //get actionCommand Data


    m_varCommandList = CCommandList::getInstance()->getActionCommandList();
    if(m_varCommandList.isEmpty())
    {
        return dataMap;
    }
    //data write into QVariantMap
    //QVariantMap tmpMap = dataMap;
    foreach (auto var, m_varCommandList)
    {
        QVariantList  list;
        int index = var->parentMenu();
        if(index > eMenuType_Help)continue;
        if(dataMap.isEmpty())
        {
            return dataMap;
        }
        if(dataMap.find(QString::number(index))->isNull())
        {
            list.append(QVariant::fromValue(var));
        }
        else
        {
            list = dataMap.find(QString::number(index)).value().value<QVariantList>();
            list.append(QVariant::fromValue(var));
        }
        dataMap.insert(QString::number(index),list);
    }
    return dataMap;
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
