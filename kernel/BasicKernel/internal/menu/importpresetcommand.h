#ifndef IMPORTPRESETCOMMAND_H
#define IMPORTPRESETCOMMAND_H
#include "menu/actioncommand.h"
#include "data/interface.h"
#include "qtusercore/module/cxhandlerbase.h"
#include <QJsonObject>
#include "qtusercore/module/quazipfile.h"
#include "data/kernelenum.h"
namespace creative_kernel
{
    class ImportPresetCommand : public ActionCommand
        , public UIVisualTracer
        , public qtuser_core::CXHandleBase
    {
        Q_OBJECT
    public:
        ImportPresetCommand();
        virtual ~ImportPresetCommand();

        void setModelType(ModelNType type);
        Q_INVOKABLE void execute();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
        QString filter() override;
        void handle(const QString& fileName) override;
        void processOrcaPrinter(QString filename);
        bool process3MFStructFile(QIODevice& device);
        bool processStructFile(QIODevice& device);
        bool processOrcaProfile(QString type,QString path,QString preset_name);
        bool copyOrcaProfle(QString type,QString preset_name);
        bool save2json(QString preset_name,QString path,QString type,QJsonObject json);
        QJsonObject loadFromKeys(QString inherits);
        bool process3MFPrinter(QString filename);
    private:
        qtuser_core::QuazipFile* m_zipFile;
        MachineData m_machineMetaData;
        ModelNType m_partType { ModelNType::normal_part };
    };
}

#endif // OPENFILECOMMAND_H
