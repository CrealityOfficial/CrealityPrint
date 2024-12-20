#include "standardmodelimport.h"
#include "interface/commandinterface.h"
#include "interface/spaceinterface.h"
#include "interface/selectorinterface.h"

#include <QtCore/QCoreApplication>
#include "msbase/primitive/primitive.h"

namespace creative_kernel
{
    StandardModelImport::StandardModelImport(ShapeType shapeType)
        :ActionCommand()
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

    StandardModelImport::~StandardModelImport()
    {

    
    }

    void StandardModelImport::setModelType(ModelNType type)
    {
        m_type = type;
    }

    void StandardModelImport::setAddPartFlag(bool bFlag)
    {
        m_addPartFlag = bFlag;
    }

    void StandardModelImport::execute()
    {
        trimesh::TriMesh* triMesh  = nullptr;
        switch (m_ShapeType)
        {
        case creative_kernel::StandardModelImport::ST_CUBOID:
            triMesh = msbase::createCuboid(20.0f, 20.0f, 20.0f);
            break;
        case creative_kernel::StandardModelImport::ST_SPHERICAL:
            triMesh = msbase::createSphere(20.0f, 100);
            break;
        case creative_kernel::StandardModelImport::ST_CYLINDER:
            triMesh = msbase::createCylinder(20.0f, 30.0f, 300);
            break;
        case creative_kernel::StandardModelImport::ST_SPHERICALSHELL:
            triMesh = msbase::createSphere(10.0f, 10);
            break;
        case creative_kernel::StandardModelImport::ST_CONE:
            triMesh = msbase::createCone(300, 20.0f, 30.0f);
            break;
        case creative_kernel::StandardModelImport::ST_TRUNCATEDCONE:
            triMesh = msbase::createCCylinder(30.0f, 20.0f, 40.0f, 300);
            break;
        case creative_kernel::StandardModelImport::ST_RING:
            triMesh = msbase::createTorusMesh(20.0f, 5.0f, 300, 300);
            break;
        case creative_kernel::StandardModelImport::ST_PYRAMID:
            triMesh = msbase::createCone(4, 20.0f, 30.0f);
            break;
        case creative_kernel::StandardModelImport::ST_PRISM:
            triMesh = msbase::createCylinder(20.0f, 30.0f, 3);
            break;
        case creative_kernel::StandardModelImport::ST_DISC:
            triMesh = msbase::createCylinder(5.0f, 0.3f, 300);
            break;
        default:
            break;
        }

        TriMeshPtr mesh_ptr(triMesh);
        MeshCreateInput input;
        input.mesh = mesh_ptr;
        input.fileName = m_actionname;
        input.name = m_actionname;
        input.type = m_type;

        if ( m_addPartFlag && 1 == selectedGroups().size())
        {
            input.group = selectedGroups()[0]->getObjectId();
        }

        createFromInput(input);
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
        case creative_kernel::StandardModelImport::ST_DISC:
            m_actionname = QCoreApplication::translate("StandardModel", "Disc");
            m_actionNameWithout = "Disc";
            m_source = "qrc:/UI/photo/menuImg/disc.svg";
            m_icon = "qrc:/UI/photo/menuImg/disc.svg";
            break;
        default:
            break;
        }
    }
}
