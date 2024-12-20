#include "autosavejob.h"
#include "qtusercore/module/progressor.h"
#include "internal/project_3mf/save3mf.h"
AutoSaveJob::AutoSaveJob(QString projectname,QList<creative_kernel::ModelGroup*> groups,SaveType type)
{
    m_fileName = projectname;
    m_modelGroups = groups;
    m_type = type;
}
 void AutoSaveJob::Save()
 {
    //qtuser_core::Progressor progressor;
    creative_kernel::save_current_scene_2_3mf_with_group(m_fileName, m_modelGroups,nullptr);
 }