#ifndef AUTOSAVEJOB_H
#define AUTOSAVEJOB_H
#include "common_3mf.h"
#include <QObject>
#include "data/modelgroup.h"
class AutoSaveJob
{
    public:
        enum SaveType
        {
            ALL=0,
            MESH,
            COLOR,
            SEAM,
            SUPPORT,
            MAT,
            PLATE,
            MODELSETTINGS,
            PRINTSETTINGS
        };
        AutoSaveJob(QString projectname,QList<creative_kernel::ModelGroup*> groups,SaveType type=SaveType::ALL);
        void Save();
    private:
      QString m_fileName;
      SaveType m_type;
      QList<creative_kernel::ModelGroup*>  m_modelGroups;
};
#endif // CX3DAUTOSAVEPROJECT_H