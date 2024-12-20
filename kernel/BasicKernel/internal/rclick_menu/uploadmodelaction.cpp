#include "uploadmodelaction.h"

#include "interface/selectorinterface.h"
#include "interface/commandinterface.h"
#include "interface/cloudinterface.h"
#include "interface/uiinterface.h"
#include "uploadmodelsubaction.h"

namespace creative_kernel
{
    UploadModelAction::UploadModelAction(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Upload Models");
        m_actionNameWithout = "Upload Models";
        m_bSubMenu = true;

        if (nullptr == m_actionModelList)
        {
            m_actionModelList = new ActionCommandModel(this);
        }

        m_uploadLoacalAction = new UploadModelSubAction(UploadModelSubAction::UPLOADLOCAL, tr("Upload Local Model"), this);
        m_uploadSelectAction = new UploadModelSubAction(UploadModelSubAction::UPLOADSELECT, tr("Upload Selected Model"), this);
        m_actionModelList->addCommand(m_uploadLoacalAction);
        m_actionModelList->addCommand(m_uploadSelectAction);
        addUIVisualTracer(this,this);
    }

    UploadModelAction::~UploadModelAction()
    {

    }

    QAbstractListModel* UploadModelAction::subMenuActionmodel()
    {
        return m_actionModelList;
    }

    void UploadModelAction::execute()
    {
        /*  if (IsCloudLogined())
          {
              uploadSelectModels();
          }
          else
          {
              requestQmlDialog("idWarningLoginDlg");
          }*/
    }

    bool UploadModelAction::enabled()
    {
        return true;// selectionms().size() > 0;
    }

    void UploadModelAction::onThemeChanged(ThemeCategory category)
    {
    }

    void UploadModelAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Upload Models");
        m_uploadLoacalAction->setName(tr("Upload Local Model"));
        m_uploadSelectAction->setName(tr("Upload Selected Model"));
    }
}
