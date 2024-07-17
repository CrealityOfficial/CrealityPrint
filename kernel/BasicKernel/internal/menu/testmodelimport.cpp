#include <QCoreApplication>
#include "testmodelimport.h"
#include "menu/actioncommand.h"
#include "interface/commandinterface.h"
#include "cxkernel/interface/modelninterface.h"
#include "cxkernel/data/modelndata.h"
#include <buildinfo.h>
#include "msbase/primitive/primitive.h"
#include "cxkernel/interface/iointerface.h"
namespace creative_kernel
{
    TestModelImport::TestModelImport(ShapeType shapeType)
    {
        setActionNameByType(shapeType);
        m_ShapeType = shapeType;
        m_shortcut = "";      //不能有空格
        m_source = "";
        m_bSubMenu = true;
        m_insertKey = "subMenuFile";
        initActionModel();
        addUIVisualTracer(this,this);
    }

    TestModelImport::~TestModelImport()
    {

    
    }

    Q_INVOKABLE void TestModelImport::execute()
    {
        QString fileName = "";
        switch (m_ShapeType)
        {
        case creative_kernel::TestModelImport::BLOCK20XY:
            fileName = "Block20XY.stl";
            break;
        case creative_kernel::TestModelImport::BOAT:
            fileName = "3DBenchy.stl";
            break;
        case creative_kernel::TestModelImport::COMPLEX:
            fileName = "ksr_fdmtest_w4.stl";
            break;
        case creative_kernel::TestModelImport::OVERHANG: 
            fileName = "Overhang.stl";
            break;
        case creative_kernel::TestModelImport::SQUARE_COLUMNS_Z_AXIS:
            fileName = "Square columns Z axis.stl";
            break;
        case creative_kernel::TestModelImport::SQUARE_PRISM_Z_AXIS:
            fileName = "Square prism Z axis.stl";
            break;
        default:
            break;
        }
        #if defined(__APPLE__)
            const auto index = QCoreApplication::applicationDirPath().lastIndexOf(QStringLiteral("/"));
            QString name = QStringLiteral("%1%2%3").arg(QCoreApplication::applicationDirPath().left(index))
                                 .arg(QStringLiteral("/Resources/resources/mesh/testModel/")).arg(fileName);
        #else
            QString name = QCoreApplication::applicationDirPath() + "/resources/mesh/testModel/" + fileName;
        #endif
        cxkernel::openFileWithString(name);
    }

    QAbstractListModel* TestModelImport::subMenuActionmodel()
    {
        return m_actionModelList;
    }

    void TestModelImport::initActionModel()
    {

    }

    void TestModelImport::onThemeChanged(ThemeCategory category)
    {

    }

    void TestModelImport::onLanguageChanged(MultiLanguage language)
    {
        setActionNameByType(m_ShapeType);
    }

    void TestModelImport::setActionNameByType(ShapeType type)
    {
        switch (type)
        {
        case creative_kernel::TestModelImport::BLOCK20XY:
            m_actionname = QCoreApplication::translate("TestModel", "Block20XY");
            m_actionNameWithout = "Block20XY";
            m_source = "qrc:/UI/photo/menuImg/Block20XY.svg";
            m_icon = "qrc:/UI/photo/menuImg/Block20XY.svg";
            break;
        case creative_kernel::TestModelImport::BOAT:
            m_actionname = QCoreApplication::translate("TestModel", "Boat");
            m_actionNameWithout = "Boat";
            m_source = "qrc:/UI/photo/menuImg/Boat.png";
            m_icon = "qrc:/UI/photo/menuImg/Boat.png";
            break;
        case creative_kernel::TestModelImport::COMPLEX:
            m_actionname = QCoreApplication::translate("TestModel", "Complex");
            m_actionNameWithout = "Complex";
            m_source = "qrc:/UI/photo/menuImg/Complex.png";
            m_icon = "qrc:/UI/photo/menuImg/Complex.png";
            break;
        case creative_kernel::TestModelImport::OVERHANG:
            m_actionname = QCoreApplication::translate("TestModel", "Overhang");
            m_actionNameWithout = "Overhang";
            m_source = "qrc:/UI/photo/menuImg/Overhang.png";
            m_icon = "qrc:/UI/photo/menuImg/Overhang.png";
            break;
        case creative_kernel::TestModelImport::SQUARE_COLUMNS_Z_AXIS:
            m_actionname = QCoreApplication::translate("TestModel", "Square columns Z axis");
            m_actionNameWithout = "Square columns Z axis";
            m_source = "qrc:/UI/photo/menuImg/Square columns Z axis.svg";
            m_icon = "qrc:/UI/photo/menuImg/Square columns Z axis.svg";
            break;
        case creative_kernel::TestModelImport::SQUARE_PRISM_Z_AXIS:
            m_actionname = QCoreApplication::translate("TestModel", "Square prism Z axis");
            m_actionNameWithout = "Square prism Z axis";
            m_source = "qrc:/UI/photo/menuImg/Square prism Z axis.png";
            m_icon = "qrc:/UI/photo/menuImg/Square prism Z axis.png";
            break;
        default:
            break;
        }
    }
}
