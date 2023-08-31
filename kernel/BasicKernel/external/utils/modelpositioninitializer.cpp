#include "modelpositioninitializer.h"

#include "external/data/modelspace.h"
#include "qtuser3d/math/space3d.h"
#include "qtuser3d/math/layoutalg.h"
#include <QtCore/QRectF>

#include "interface/spaceinterface.h"
#include "interface/modelinterface.h"
#include "interface/machineinterface.h"

#include <nestplacer/nestplacer.h>
#include "qcxutil/trimesh2/conv.h"
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
			mdlInfor.ptPolygon = qcxutil::vecs2qvectors(lines);
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
		layAlg.InsertOnePolygon(ptPlatform, plgGroup, modelInsert, ptDst, 10, progressor);
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

	std::vector<trimesh::vec3> outLine(ModelN* model, trimesh::vec3& toffset)
	{
		QVector3D offset = model->localPosition();	
		std::vector<trimesh::vec3> lines;
		model->convex(lines);
		if (offset == QVector3D())
		{
			float max_x = 0, min_x = FLT_MAX, max_y = 0, min_y = FLT_MAX;
			for (trimesh::vec3& v : lines)
			{
				if (max_x < v.x) max_x = v.x;
				if (min_x > v.x) min_x = v.x;
				if (max_y < v.y) max_y = v.y;
				if (min_y > v.y) min_y = v.y;
			}
			offset.setX((max_x + min_x) / 2);
			offset.setY((max_y + min_y) / 2);
		}
		toffset = qcxutil::qVector3D2Vec3(offset);
		for (trimesh::vec3& v : lines)
			v -= toffset;
		return lines;
	}

	bool ModelPositionInitializer::nestLayout(ModelN* model)
	{
		qtuser_3d::Box3D box = baseBoundingBox();
		QVector3D boxMinMax = box.size();
		QList <ModelN*> oldModels = modelns();
		bool can_pack = false;

		std::vector < std::vector<trimesh::vec3>> modelsData;
		std::vector<trimesh::vec3> transData;
		trimesh::vec3 RTdata;
		for (int m = 0; m < oldModels.size(); m++)
		{		
			std::vector<trimesh::vec3> oItem = outLine(oldModels[m], RTdata);
			modelsData.push_back(oItem);
			transData.push_back(RTdata);
		}
		std::vector<trimesh::vec3> NewItem = outLine(model, RTdata);
		trimesh::box3 workspaceBox(trimesh::vec3(box.min.x(), box.min.y(), box.min.z()), trimesh::vec3(box.max.x(), box.max.y(), box.max.z()));
		trimesh::vec3 offset(0, 0, 0);

		if (currentMachineCenterIsZero())
		{
			offset = trimesh::vec3(boxMinMax.x() / 2, boxMinMax.y() / 2, 0);
		}

		std::function<void(trimesh::vec3)> modelPositionUpdateFunc_nest = [model, offset](trimesh::vec3 newBoxCenter) {
			QQuaternion dq = QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 0.0f, 1.0f), newBoxCenter.z);
			QQuaternion q = model->localQuaternion();
			QVector3D translate = QVector3D(newBoxCenter.x - offset.x, newBoxCenter.y - offset.y, 0.0f);
			translate.setZ(model->localPosition().z());
			model->SetInitPosition(translate);
			model->setLocalPosition(translate);
			setModelRotation(model, dq * q, true);
		};

		nestplacer::NestParaFloat para = nestplacer::NestParaFloat(workspaceBox, 10.0f, nestplacer::PlaceType::NULLTYPE, true);
		can_pack = nestplacer::NestPlacer::layout_new_item(modelsData, transData, NewItem, para, modelPositionUpdateFunc_nest);

		checkModelRange();
		checkBedRange();
		return can_pack;
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
