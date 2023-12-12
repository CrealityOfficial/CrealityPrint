#include "modelpositioninitializer.h"

#include "external/data/modelspace.h"
#include "qtuser3d/math/space3d.h"
#include "qtuser3d/math/layoutalg.h"
#include <QtCore/QRectF>

#include "interface/spaceinterface.h"
#include "interface/modelinterface.h"
#include "interface/machineinterface.h"

#include "qtuser3d/trimesh2/conv.h"

#define FLT_MAX 3.402823466e+38F
namespace creative_kernel
{
	ModelPositionInitializer::ModelPositionInitializer(QObject* parent)
		:QObject(parent)
	{
	}

	ModelPositionInitializer::~ModelPositionInitializer()
	{
	}

	void ModelPositionInitializer::layout(ModelN* model, qtuser_core::Progressor* progressor, bool bAdaption)
	{
		using namespace qtuser_3d;

		qtuser_3d::Box3D baseBox = baseBoundingBox();
		S3DPrtRectF ptPlatform;

		ptPlatform.fXMin = baseBox.min.x();
		ptPlatform.fYMin = baseBox.min.y();
		ptPlatform.fXMax = baseBox.max.x();
		ptPlatform.fYMax = baseBox.max.y();

		
        if (bAdaption)
        {
            QVector3D newscale= QVector3D(1.0f, 1.0f, 1.0f);
			qtuser_3d::Box3D modelbox = model->globalSpaceBox();
            QVector3D oldscale = model->localScale();
            while (modelbox.size().x() > baseBox.size().x()||
                modelbox.size().y() > baseBox.size().y()||
                modelbox.size().z() > baseBox.size().z())
            {
                newscale = newscale * 0.9f;
                model->setLocalScale(newscale, true);
                modelbox = model->globalSpaceBox();

                qtuser_3d::Box3D box = model->globalSpaceBox();
                QVector3D zoffset = QVector3D(0.0f, 0.0f, -box.min.z());
                QVector3D oldLocalPosition = model->localPosition();
                QVector3D newLocalPosition = oldLocalPosition + zoffset;
                mixTSModel(model, oldLocalPosition, newLocalPosition, oldscale, newscale, false);
            }
        }



		auto InitializeModelInfor = [](ModelN* pModel, SModelPolygon& mdlInfor)
		{
			qtuser_3d::Box3D boxCur = pModel->globalSpaceBox();
			QVector3D ptGrobalCenter;

			ptGrobalCenter.setX((boxCur.min.x() + boxCur.max.x()) / 2);
			ptGrobalCenter.setY((boxCur.min.y() + boxCur.max.y()) / 2);
			std::vector<trimesh::vec3> lines;
			pModel->convex(lines);
			mdlInfor.ptPolygon = qtuser_3d::vecs2qvectors(lines);
			mdlInfor.ptGrobalCenter = ptGrobalCenter;

			qtuser_3d::Box3D baseGrobalBox = pModel->globalSpaceBox();
			mdlInfor.rcGrobalDst.fXMin = baseGrobalBox.min.x();
			mdlInfor.rcGrobalDst.fYMin = baseGrobalBox.min.y();
			mdlInfor.rcGrobalDst.fXMax = baseGrobalBox.max.x();
			mdlInfor.rcGrobalDst.fYMax = baseGrobalBox.max.y();
		};

		//initialize the source model information
		SModelPolygon modelInsert;
		InitializeModelInfor(model, modelInsert);

		//initialize all models that are in the platform
		QVector<SModelPolygon> plgGroup;
		QList<ModelN*> models = modelns();
		for (ModelN* m : models)
		{
			if (m != model)
			{
				SModelPolygon itModelInfor;
				InitializeModelInfor(m, itModelInfor);

				plgGroup.push_back(itModelInfor);
			}
		}

		S3DPrtPointF ptDst;

		CLayoutAlg layAlg;
		layAlg.InsertOnePolygon(ptPlatform, plgGroup, modelInsert, ptDst, 10);
		qDebug() << "load>>> layout";

		QVector3D ptValid;//
		ptValid.setX(ptDst.fX);
		ptValid.setY(ptDst.fY);

		QVector3D translate = ptValid - modelInsert.ptGrobalCenter + model->localPosition();
		model->SetInitPosition(translate);
		model->setLocalPosition(translate);

		checkModelRange();
		checkBedRange();
	}

	void ModelPositionInitializer::layoutBelt(ModelN* model, qtuser_core::Progressor* progressor, bool bAdaption)
	{
		qtuser_3d::Box3D baseBox = baseBoundingBox();

		QList<ModelN*> models = modelns();
		if (models.size())
		{
			QVector3D ptGrobalCenter;
			ptGrobalCenter.setX((baseBox.min.x() + baseBox.max.x()) / 2);
			float maxY = 0.0;
			for (ModelN* amodel : models)
			{
				if (maxY < amodel->globalSpaceBox().max.y())
				{
					maxY = amodel->globalSpaceBox().max.y();
				}
			}

			qtuser_3d::Box3D box = model->localBox();
			float localY = maxY+ 50+(box.max.y() - box.min.y()) * 0.5;
			ptGrobalCenter.setY(localY);
			QVector3D translate = ptGrobalCenter + model->localPosition();
			model->setLocalPosition(translate);
		}
		else
		{
			qtuser_3d::Box3D boxCur = model->globalSpaceBox();
			QVector3D ptGrobalCenter;
			ptGrobalCenter.setX((baseBox.min.x() + baseBox.max.x()) / 2);
			qtuser_3d::Box3D box = model->localBox();
			float localY = 10 + (box.max.y() - box.min.y()) * 0.5;
			ptGrobalCenter.setY(localY);
			QVector3D translate = ptGrobalCenter + model->localPosition();
			model->setLocalPosition(translate);
		}
		model->SetInitPosition(model->localPosition());
	}
}
