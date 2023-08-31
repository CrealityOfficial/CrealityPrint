#ifndef CCOMMANDSDATA_H
#define CCOMMANDSDATA_H
#include "basickernelexport.h"
#include "menu/actioncommand.h"
#include <QVariantList>
#include <QList>
#include "data/interface.h"

namespace creative_kernel
{
    class BASIC_KERNEL_API CCommandsData : public QObject
        , public UIVisualTracer
    {
        Q_OBJECT

    public:
        explicit CCommandsData(QObject* parent = nullptr);
        virtual ~CCommandsData();
        void createMenuNameMap();
        Q_INVOKABLE void addCommads(ActionCommand* pAction);
        //    QString getMenuNameFromEType(eMenuType eType);
        Q_INVOKABLE QString getMenuNameFromKey(QString key);
        Q_INVOKABLE QVariantList  getCommandsList(void);
        Q_INVOKABLE QVariantMap getCommandsMap(void);

        Q_INVOKABLE QVariant getFileOpenOpt();
        Q_INVOKABLE QVariant getOpt(const QString& optName);

        void initMapData(QVariantMap& map);

    private:
        void addCommonCommand();

    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    private:
        QList<ActionCommand*>  m_varCommandList;
        QMap<QString, QString>  m_mapMenuName;
        ActionCommand* m_FileOpenOpt;

    };
}

#endif // CCOMMANDSDATA_H
