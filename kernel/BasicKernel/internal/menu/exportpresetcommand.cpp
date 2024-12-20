#include "exportpresetcommand.h"
#include "interface/commandinterface.h"
#include "cxkernel/interface/iointerface.h"
#include <QCoreApplication>
#include "qtusercore/string/resourcesfinder.h"
#include "external/interface/machineinterface.h"
#include "qtusercore/module/systemutil.h"
#include "qtusercore/module/quazipfile.h"
#include "QTemporaryDir"
namespace creative_kernel
{
    ExportPresetCommand::ExportPresetCommand()
        :ActionCommand()
    {
        m_shortcut = "Ctrl+E";
        m_actionname = QCoreApplication::translate("MenuCmdObj","Preset Config");
        m_actionNameWithout = "Export Preset";
        m_icon = "qrc:/kernel/images/save.png";
        m_eParentMenu = eMenuType_File;

        addUIVisualTracer(this,this);
    }

    ExportPresetCommand::~ExportPresetCommand()
    {
    }

    void ExportPresetCommand::execute()
    {
        cxkernel::saveFile(this);
    }

    void ExportPresetCommand::onThemeChanged(ThemeCategory category)
    {
    }

    void ExportPresetCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = QCoreApplication::translate("MenuCmdObj", "Preset Config");// +"        " + m_shortcut;
    }
    QString ExportPresetCommand::filter()
    {
        return QString("Preset File (*.zip)");
    }
    void ExportPresetCommand::handle(const QString& fileName)
    {
        QTemporaryDir temp_dir;
        auto params_dir = temp_dir.filePath("param");
        QDir().mkpath(params_dir);
        creative_kernel::exportAllCurrentSettings(params_dir);
        qtuser_core::compressDir(fileName, params_dir);
        clearPath(params_dir);
    }
}
