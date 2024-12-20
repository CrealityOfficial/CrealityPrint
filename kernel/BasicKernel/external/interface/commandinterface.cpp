#include "commandinterface.h"

#include <QFileInfo>
#include "data/modeln.h"

#include "cxkernel/interface/jobsinterface.h"

#include "qtuser3d/trimesh2/conv.h"

#include "operation/rotateop.h"
#include "interface/spaceinterface.h"
#include "interface/machineinterface.h"
#include "interface/layoutinterface.h"
#include "interface/printerinterface.h"

#include "spaceinterface.h"
#include "selectorinterface.h"
#include "visualsceneinterface.h"

#include "qtuser3d/math/angles.h"
#include "cxkernel/wrapper/eventtracking.h"

#include "kernel/kernel.h"
#include "kernel/kernelui.h"
#include "kernel/commandcenter.h"
#include "kernel/translator.h"
#include "msbase/mesh/merge.h"

#include "internal/render/modelnentity.h"

namespace creative_kernel
{
    int cmdSaveAs(const QString& fileName)
    {
        std::vector<TriMeshPtr> meshs;
        QList<ModelN*> models = selectionms();
        size_t size = models.size();

        if (size > 0)
        {
            for (size_t i = 0; i < models.size(); i++)
            {
                TriMeshPtr newMesh = models.at(i)->createGlobalMeshWithNormal();
                if (newMesh.get())
                {
                    meshs.push_back(newMesh);
                }

                meshs.push_back(newMesh);
            }

            trimesh::TriMesh* newmodel = msbase::mergeMeshes(meshs);

            char buff[128];
            QByteArray qbyteTemp = fileName.toLocal8Bit();
            strcpy(buff, qbyteTemp.data());

            newmodel->write(buff);
            newmodel->clear();
        }

        return size;
    }

    void saveStl(const QString& fileName, ModelN* model)
    {
        std::vector<trimesh::TriMesh*> meshs;
        //QList<ModelN*> models = selectionms();
        //size_t size = models.size();

        //if (size > 0)
        {
            //for (size_t i = 0; i < models.size(); i++)
            {
                bool isfanzhuan = model->getFanZhuanState();
                QMatrix4x4 matrix = model->globalMatrix();
                trimesh::TriMesh* meshTemp = model->mesh();
                trimesh::TriMesh* newMesh = new trimesh::TriMesh();
                *newMesh = *meshTemp;

                trimesh::fxform xf = qtuser_3d::qMatrix2Xform(matrix);
                int size = meshTemp->vertices.size();
                for (int n = 0; n < size; ++n)
                {
                    trimesh::vec3 v = meshTemp->vertices.at(n);
                    newMesh->vertices.at(n) = xf * v;
                }
                if (isfanzhuan)
                {
                    size = newMesh->faces.size();
                    for (int n = 0; n < size; ++n)
                    {
                        newMesh->faces.at(n)[1] = meshTemp->faces.at(n)[2];
                        newMesh->faces.at(n)[2] = meshTemp->faces.at(n)[1];
                    }
                }
                meshs.push_back(newMesh);
            }

            trimesh::TriMesh* newmodel = msbase::mergeMeshes(meshs);

            char buff[128];
            QByteArray qbyteTemp = fileName.toLocal8Bit();
            strcpy(buff, qbyteTemp.data());

            newmodel->write(buff);
            newmodel->clear();
        }

        qDeleteAll(meshs);
    }

    void cmdSaveSelectStl(const QString& filePath, QList<QString>& saveNames)
    {
        QList<ModelN*> models = selectionms();
        size_t size = models.size();
        saveNames.clear();
        if (size > 0)
        {
            for (size_t i = 0; i < models.size(); i++)
            {
                QString name = models[i]->objectName();
                QString fullName = name.section(".", 0, 0) + ".stl";
                saveNames.append(fullName);
                saveStl(filePath + "/" + fullName, models[i]);
            }
        }
    }

    void sensorAnlyticsTrace(const QString& eventType, const QString& eventAction)
    {
        getKernel()->sensorAnalytics()->trace(eventType, eventAction);
    }

    void openUserCourseLocation()
    {
        getKernel()->commandCenter()->openUserCourseLocation();
    }

    void openLogLocation()
    {
        getKernel()->commandCenter()->openLogLocation();
    }

    void openModelLocation()
    {
        getKernel()->commandCenter()->openModelLocation();
    }

    void openUserFeedbackWebsite()
    {
        getKernel()->commandCenter()->openUserFeedbackWebsite();
    }

    void openCalibrationTutorialWebsite()
    {
        getKernel()->commandCenter()->openCalibrationTutorialWebsite();
    }
    void openUseTutorialWebsite()
    {
        getKernel()->commandCenter()->openUseTutorialWebsite();
    }

    void openFileLocation(const QString& file)
    {
        getKernel()->commandCenter()->openFileLocation(file);
    }

    void openUrl(const QString& web)
    {
        getKernel()->commandCenter()->openUrl(web);
    }

    void addUIVisualTracer(UIVisualTracer* tracer, QObject* base)
    {
        QQmlEngine::setObjectOwnership(base, QQmlEngine::ObjectOwnership::CppOwnership);
        getKernelUI()->translator()->addUIVisualTracer(tracer);
    }

    void removeUIVisualTracer(UIVisualTracer* tracer)
    {
        getKernelUI()->translator()->removeUIVisualTracer(tracer);
    }

    void changeTheme(ThemeCategory theme)
    {
        getKernelUI()->translator()->changeTheme(theme);
    }

    void changeLanguage(MultiLanguage language)
    {
        getKernelUI()->translator()->changeLanguage(language);
    }

    MultiLanguage currentLanguage()
    {
        return getKernelUI()->translator()->currentLanguage();
    }

    ThemeCategory currentTheme()
    {
        return getKernelUI()->translator()->currentTheme();
    }

    QString generateSceneName()
    {
        return getKernel()->commandCenter()->generateSceneName();
    }

    void setKernelPhase(KernelPhaseType phase)
    {
        return getKernel()->setKernelPhase(phase);
    }
}
