#ifndef STANDARDMODELIMPORT_H
#define STANDARDMODELIMPORT_H
#include "menu/actioncommand.h"
#include "menu/actioncommandmodel.h"
#include "data/interface.h"
#include <QtCore/QSettings>

namespace creative_kernel
{
    class BASIC_KERNEL_API StandardModelImport : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
        Q_PROPERTY(QAbstractListModel* subMenuActionmodel READ subMenuActionmodel CONSTANT)
    public:
        enum ShapeType
        {
            ST_CUBOID, //³¤·½Ìå
            ST_SPHERICAL, //ÇòÐÎ
            ST_CYLINDER,//Ô²ÖùÌå
            ST_SPHERICALSHELL,//Çò¿Ç
            ST_CONE,//Ô²×¶Ìå
            ST_TRUNCATEDCONE,//½ØÔ²×¶
            ST_RING,//Ô²»·
            ST_PYRAMID,//Àâ×¶
            ST_PRISM,//ÀâÖù
            //ST_RING,//Ring
        };

        explicit StandardModelImport(ShapeType shapeType);
        virtual ~StandardModelImport();

        Q_INVOKABLE void execute();
        void initActionModel();
        QAbstractListModel* subMenuActionmodel();

    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    private:
        void setActionNameByType(ShapeType type);
        ActionCommandModel* m_actionModelList = nullptr;
        ShapeType m_ShapeType;
    };
}

#endif // STANDARDMODELIMPORT_H
