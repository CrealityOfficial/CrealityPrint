#ifndef TESTMODELIMPORT_H
#define TESTMODELIMPORT_H
#include "menu/actioncommand.h"
#include "menu/actioncommandmodel.h"
#include "data/interface.h"
#include <QtCore/QSettings>

namespace creative_kernel
{
    class BASIC_KERNEL_API TestModelImport : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
        Q_PROPERTY(QAbstractListModel* subMenuActionmodel READ subMenuActionmodel CONSTANT)
    public:
        enum ShapeType
        {
            BLOCK20XY,
            BOAT, 
            COMPLEX,
            OVERHANG,
            SQUARE_COLUMNS_Z_AXIS,
            SQUARE_PRISM_Z_AXIS,
        };

        explicit TestModelImport(ShapeType shapeType);
        virtual ~TestModelImport();

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
