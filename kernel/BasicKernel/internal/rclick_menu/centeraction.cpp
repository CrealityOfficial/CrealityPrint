#include "centeraction.h"

#include "interface/selectorinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/commandinterface.h"

#include "interface/selectorinterface.h"
#include "interface/printerinterface.h"
#include "internal/multi_printer/printer.h"
#include "interface/spaceinterface.h"

#include "qtuser3d/trimesh2/conv.h"

namespace creative_kernel
{
    CenterAction::CenterAction(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Center");
        m_actionNameWithout = "Center";
        m_shortcut = "F1";

        addUIVisualTracer(this,this);
    }

    CenterAction::~CenterAction()
    {
    }

    void CenterAction::execute()
    {
        Printer* printer = getSelectedPrinter();
        qtuser_3d::Box3D box = printer->globalBox();
        QVector3D pcenter = box.center();
        QList<ModelGroup*> groups = selectedGroups();
        QList<ModelN*> parts = selectedParts();

        if (!groups.isEmpty())
        {
            beginNodeSnap(groups, parts);

            for (ModelGroup* g : groups)
            {
                float x = pcenter.x();
                float y = pcenter.y();
                float z = g->globalBox().size().z() / 2.0;
                trimesh::xform xf = g->getMatrix();
                QVector3D groupCenter = g->globalBox().center();
                float dx = xf[12] - groupCenter.x();
                float dy = xf[13] - groupCenter.y();
                float dz = xf[14] - groupCenter.z();

                xf[12] = x + dx;
                xf[13] = y + dy;
                xf[14] = z + dz;
                g->setMatrix(xf);
                g->dirty();
            }
		    endNodeSnap();
		    updateSpaceNodeRender(groups, true);
        }

        if (!parts.isEmpty())
        {
            beginNodeSnap(groups, parts);
            for (ModelN* model : parts)
            {
                float x = pcenter.x();
                float y = pcenter.y();
                float z = model->globalSpaceBox().size().z() / 2.0;

                TriMeshPtr mesh = model->data()->mesh;
                mesh->need_bbox();
                trimesh::vec3 center = mesh->bbox.center();

                ModelGroup* group = model->getModelGroup();
                trimesh::xform gxf = group->getMatrix();
                trimesh::xform mxf = model->getMatrix();

                trimesh::xform globalXf = gxf * mxf;
                globalXf[12] = x;
                globalXf[13] = y;
                globalXf[14] = z;

                trimesh::xform igxf = trimesh::inv(gxf);
                trimesh::xform xf = globalXf * igxf;
                model->setMatrix(xf);
                model->dirty();
            }
            
		    endNodeSnap();
		    updateSpaceNodeRender(parts, true);
        }

    }

    bool CenterAction::enabled()
    {
        return selectedGroupsNum() > 0 || selectedPartsNum() > 0;
    }
    void CenterAction::onThemeChanged(ThemeCategory category)
    {
    }

    void CenterAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Center");
    }
}