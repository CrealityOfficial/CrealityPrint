#include "onesideasbottomface.h"

#include "interface/selectorinterface.h"
#include "interface/commandinterface.h"
#include "kernel/kernelui.h"

namespace creative_kernel
{
    OneSideAsBottomFace::OneSideAsBottomFace(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("OneSide As Bottomface");
        m_actionNameWithout = "OneSide As Bottomface";

        addUIVisualTracer(this,this);
    }

    OneSideAsBottomFace::~OneSideAsBottomFace()
    {
    }

    void OneSideAsBottomFace::execute()
    {
        QObject* lefttoolbar = getKernelUI()->getUI("lefttoolbar");
        QMetaObject::invokeMethod(lefttoolbar, "pButtomBtnClick");
    }

    bool OneSideAsBottomFace::enabled()
    {
        return selectionms().size() > 0;
    }

    void OneSideAsBottomFace::onThemeChanged(ThemeCategory category)
    {

    }

    void OneSideAsBottomFace::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("OneSide As Bottomface");
    }
}
