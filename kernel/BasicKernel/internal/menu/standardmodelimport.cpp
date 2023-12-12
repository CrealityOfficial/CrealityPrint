#include <QCoreApplication>
#include "standardmodelimport.h"
#include "menu/actioncommand.h"
#include "interface/commandinterface.h"
#include "cxkernel/interface/modelninterface.h"
#include "cxkernel/data/modelndata.h"

#include "msbase/primitive/primitive.h"

namespace creative_kernel
{
    StandardModelImport::StandardModelImport(ShapeType shapeType)
    {
        setActionNameByType(shapeType);
        m_ShapeType = shapeType;
        m_shortcut = "";      //不能有空格
        m_source = "";
        m_bSubMenu = true;
        m_insertKey = "subMenuFile";
        initActionModel();
        addUIVisualTracer(this);
    }

    StandardModelImport::~StandardModelImport()
    {

    
    }

    Q_INVOKABLE void StandardModelImport::execute()
    {
        trimesh::TriMesh* triMesh  = nullptr;
        switch (m_ShapeType)
        {
        case creative_kernel::StandardModelImport::ST_CUBOID:
            triMesh = msbase::createCuboid(20, 20, 20);
            break;
        case creative_kernel::StandardModelImport::ST_SPHERICAL:
            triMesh = msbase::createSphere(20, 100);
            break;
        case creative_kernel::StandardModelImport::ST_CYLINDER:
            triMesh = msbase::createCylinder(20, 30, 300);
            break;
        case creative_kernel::StandardModelImport::ST_SPHERICALSHELL:
            triMesh = msbase::createSphere(10, 10);
            break;
        case creative_kernel::StandardModelImport::ST_CONE:
            triMesh = msbase::createCone(300, 20, 30);
            break;
        case creative_kernel::StandardModelImport::ST_TRUNCATEDCONE:
            triMesh = msbase::createCCylinder(30, 20, 40, 300);
            break;
        case creative_kernel::StandardModelImport::ST_RING:
            triMesh = msbase::createTorusMesh(20, 5, 300, 300);
            break;
        case creative_kernel::StandardModelImport::ST_PYRAMID:
            triMesh = msbase::createCone(4, 20, 30);
            break;
        case creative_kernel::StandardModelImport::ST_PRISM:
            triMesh = msbase::createCylinder(20, 30, 3);
            break;
        default:
            break;
        }

        std::shared_ptr<trimesh::TriMesh> ptr2(triMesh);
        cxkernel::ModelCreateInput input;
        input.mesh = ptr2;
        input.fileName = m_actionname;
        input.name = m_actionname;
        input.type = cxkernel::ModelNDataType::mdt_algrithm;
        cxkernel::addModelFromCreateInput(input);
    }

    QAbstractListModel* StandardModelImport::subMenuActionmodel()
    {
        return m_actionModelList;
    }

    void StandardModelImport::initActionModel()
    {

    }

    void StandardModelImport::onThemeChanged(ThemeCategory category)
    {

    }

    void StandardModelImport::onLanguageChanged(MultiLanguage language)
    {
        setActionNameByType(m_ShapeType);
    }

    void StandardModelImport::setActionNameByType(ShapeType type)
    {
        switch (type)
        {
        case creative_kernel::StandardModelImport::ST_CUBOID:
            m_actionname = QCoreApplication::translate("StandardModel", "Cuboid");
            m_actionNameWithout = "Cuboid";
            m_source = "qrc:/UI/photo/menuImg/cuboid.svg";
            m_icon = "qrc:/UI/photo/menuImg/cuboid.svg";
            break;
        case creative_kernel::StandardModelImport::ST_SPHERICAL:
            m_actionname = QCoreApplication::translate("StandardModel", "Spherical");
            m_actionNameWithout = "Spherical";
            m_source = "qrc:/UI/photo/menuImg/spherical.svg";
            m_icon = "qrc:/UI/photo/menuImg/spherical.svg";
            break;
        case creative_kernel::StandardModelImport::ST_CYLINDER:
            m_actionname = QCoreApplication::translate("StandardModel", "Cylinder");
            m_actionNameWithout = "Cylinder";
            m_source = "qrc:/UI/photo/menuImg/cylinder.svg";
            m_icon = "qrc:/UI/photo/menuImg/cylinder.svg";
            break;
        case creative_kernel::StandardModelImport::ST_SPHERICALSHELL:
            m_actionname = QCoreApplication::translate("StandardModel", "Spherical Shell");
            m_actionNameWithout = "Spherical Shell";
            m_source = "qrc:/UI/photo/menuImg/sphericalShell.svg";
            m_icon = "qrc:/UI/photo/menuImg/sphericalShell.svg";
            break;
        case creative_kernel::StandardModelImport::ST_CONE:
            m_actionname = QCoreApplication::translate("StandardModel", "Cone");
            m_actionNameWithout = "Cone";
            m_source = "qrc:/UI/photo/menuImg/cone.svg";
            m_icon = "qrc:/UI/photo/menuImg/cone.svg";
            break;
        case creative_kernel::StandardModelImport::ST_TRUNCATEDCONE:
            m_actionname = QCoreApplication::translate("StandardModel", "Truncated Cone");
            m_actionNameWithout = "Truncated Cone";
            m_source = "qrc:/UI/photo/menuImg/truncatedCone.svg";
            m_icon = "qrc:/UI/photo/menuImg/truncatedCone.svg";
            break;
        case creative_kernel::StandardModelImport::ST_RING:
            m_actionname = QCoreApplication::translate("StandardModel", "Ring");
            m_actionNameWithout = "Ring";
            m_source = "qrc:/UI/photo/menuImg/ring.svg";
            m_icon = "qrc:/UI/photo/menuImg/ring.svg";
            break;
        case creative_kernel::StandardModelImport::ST_PYRAMID:
            m_actionname = QCoreApplication::translate("StandardModel", "Pyramid");
            m_actionNameWithout = "Pyramid";
            m_source = "qrc:/UI/photo/menuImg/pyramid.svg";
            m_icon = "qrc:/UI/photo/menuImg/pyramid.svg";
            break;
        case creative_kernel::StandardModelImport::ST_PRISM:
            m_actionname = QCoreApplication::translate("StandardModel", "Prism");
            m_actionNameWithout = "Prism";
            m_source = "qrc:/UI/photo/menuImg/prism.svg";
            m_icon = "qrc:/UI/photo/menuImg/prism.svg";
            break;
        default:
            break;
        }
    }
}
