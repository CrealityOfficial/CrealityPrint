#include "changemodeltypeaction.h"
#include "interface/selectorinterface.h"
#include "interface/commandinterface.h"
#include "data/modeln.h"
#include "interface/spaceinterface.h"

namespace creative_kernel
{
    ChangeModelTypeAction::ChangeModelTypeAction(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Change Type");
        m_actionNameWithout = "Change Type";

        addUIVisualTracer(this,this);
    }

    ChangeModelTypeAction::~ChangeModelTypeAction()
    {
    }

    void ChangeModelTypeAction::setModelType(int type)
    {
        m_type = type;
    }

    void ChangeModelTypeAction::execute()
    {
        auto models = selectionms();
        if (models.isEmpty())
            return;
        
        //for (ModelN* model : models)
        //{
        //    model->setModelNType((ModelNType)m_type);
        //}

        QList<ModelNPropertyMeshUndo> changes;
        for (ModelN* model : models)
        {
            int64_t model_id;

            QString start_name;
            QString end_name;

            SharedDataID start_data_id;
            SharedDataID end_data_id;

            ModelNPropertyMeshUndo undo;
            undo.model_id = model->getObjectId();
            undo.start_name = model->name();
            undo.end_name = model->name();

            SharedDataID id = model->sharedDataID();
            undo.start_data_id = id;

            id(ModelType) = m_type;
            undo.end_data_id = id;

            changes << undo;
        }

        replaceModelsProperty(changes, true);
    }

    bool ChangeModelTypeAction::enabled()
    {
        //return selectedGroups().size() >= 2;
        return true;
    }

    void ChangeModelTypeAction::onThemeChanged(ThemeCategory category)
    {
    }

    void ChangeModelTypeAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Change Type");
    }
}
