#include "byobjecteventhandler.h"
#include "internal/multi_printer/printer.h"
#include "internal/multi_printer/printermanager.h"
#include "interface/printerinterface.h"
#include "interface/selectorinterface.h"
#include "data/modeln.h"
#include "data/modelgroup.h"
#include "nestplacer/printpiece.h"
#include "kernel/kernelui.h"
#include "external/message/modelcollisionmessage.h"
#include "external/message/modeltootallmessage.h"
#include "external/kernel/kernel.h"
#include "internal/parameter/parametermanager.h"
#include "external/kernel/visualscene.h"

#include "topomesh/interface/utils.h"
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <functional>
#include <chrono>
#include <qtuser3d/trimesh2/conv.h>

namespace creative_kernel {

	ByObjectEventHandler::ByObjectEventHandler(QObject* parent)
		:PMCollisionCheckHandler(parent)
		, m_enable(false)
		, m_processing(false)
	{
		ParameterManager* parameterManager = getKernel()->parameterManager();
		connect(parameterManager, &ParameterManager::extruderClearanceHeightToRodChanged,
			this, &ByObjectEventHandler::onExtruderClearanceHeightToLodChanged);
		
		getKernel()->visScene()->heightReferEntity()->setEnabled(true);
	}

	ByObjectEventHandler::~ByObjectEventHandler()
	{
		ParameterManager* parameterManager = getKernel()->parameterManager();
		disconnect(parameterManager, &ParameterManager::extruderClearanceHeightToLidChanged,
			this, &ByObjectEventHandler::onExtruderClearanceHeightToLodChanged);

		clearCollisionFlags();
		getKernelUI()->destroyMessage(MessageType::ModelCollision);
		getKernelUI()->destroyMessage(MessageType::ModelTooTall);

		getKernel()->visScene()->heightReferEntity()->setEnabled(false);
	}

	QList<ModelGroup*> ByObjectEventHandler::collisionCheckAndUpdateState(const QList<ModelGroup*>& models)
	{
		clearCollisionFlags(m_modelPtrs);
		QList<ModelGroup*> list = _collisionCheck(models);
		setStateCollide(list);


		QList<ModelGroup*> tallList = _modelHeightCheck(models);
		setStateTooTall(tallList);

		return list + tallList;
	}

	/*void ByObjectEventHandler::initialize()
	{
		connect(getPrinterManager(), &PrinterMangager::signalDidSelectPrinter, this, &ByObjectEventHandler::slotDidSelectPrinter);
	}*/

	void ByObjectEventHandler::setStateNone(ModelGroup* m)
	{
		m->setOuterLinesVisibility(false);
		m->setNozzleRegionVisibility(false);
	}

	void ByObjectEventHandler::setStateCollide(ModelGroup* m)
	{
		m->setNozzleRegionVisibility(true);
		m->setOuterLinesVisibility(true);
		m->setOuterLinesColor(QVector4D(1.0f, 0.0f, 0.0f, 1.0f));
	}

	void ByObjectEventHandler::setStateCollide(const QList<ModelGroup*>& models)
	{
		for (auto m : models)
		{
			setStateCollide(m);
		}

		if (models.size() > 0)
		{
			/*ModelGroup* first = collideModels.first();
			QString str = first->objectName();*/

			ModelNCollisionMessage* msg = new ModelNCollisionMessage(models);
			msg->setLevel(2);
			getKernelUI()->requestMessage(msg);
		}
		else {

			getKernelUI()->destroyMessage(MessageType::ModelCollision);
		}
	}

	void ByObjectEventHandler::setStateTooTall(const QList<ModelGroup*>& tallList)
	{
		if (tallList.size() > 0)
		{
			ModelTooTallMessage* msg = new ModelTooTallMessage(tallList);
			getKernelUI()->requestMessage(msg);
		}
		else {
			getKernelUI()->destroyMessage(MessageType::ModelTooTall);
		}
	}


	void ByObjectEventHandler::setStateMoving(ModelGroup* m)
	{
		m->setOuterLinesVisibility(true);
		m->setOuterLinesColor(QVector4D(0.6f, 0.6f, 0.6f, 1.0f));
		m->setNozzleRegionVisibility(false);
	}

	void ByObjectEventHandler::clearCollisionFlags()
	{
		clearCollisionFlags(m_modelPtrs);
	}

	QList<ModelGroup*> ByObjectEventHandler::checkModelGroups(const QList<ModelGroup*>& models)
	{
		updateHeightReferEntitySize();

		return collisionCheckAndUpdateState(models);
	}

	void ByObjectEventHandler::clearCollisionFlags(const QList<QPointer<ModelGroup>>& modelPtrs)
	{
		for (ModelGroup* m : modelPtrs)
		{
			if (m == nullptr)
			{
				continue;
			}
			setStateNone(m);
		}
	}

	QList<ModelGroup*> ByObjectEventHandler::_collisionCheck(const QList<ModelGroup*>& models)
	{
		clearCollisionFlags(m_modelPtrs);

		m_modelPtrs.clear();
		for (auto m : models)
		{
			m_modelPtrs.push_back(QPointer<ModelGroup>(m));
		}
		return _collisionCheck(m_modelPtrs);
	}

	QList<ModelGroup*> ByObjectEventHandler::_collisionCheck(const QList<QPointer<ModelGroup>>& modelPtrs)
	{
		QList<ModelGroup*> models;
		for (auto m : modelPtrs)
		{
			if (m.isNull())
			{
				continue;
			}
			models.push_back(m.data());
		}

		if (models.isEmpty())
		{
			return QList<ModelGroup*>();
		}

		return ByObjectEventHandler::collisionCheck(models);
	}

	void ByObjectEventHandler::onLeftMouseButtonPress(QMouseEvent* event)
	{
		m_processing = false;

		qtuser_3d::Pickable* pickable = checkPickable(event->pos(), nullptr);
		if (pickable == nullptr/* || pickable->selected() == false*/)
			return;

		ModelN* pick = qobject_cast<ModelN*>(pickable);
		if (!pick)
		{
			return;
		}

		Printer* sel = getPrinterManager()->getSelectedPrinter();
		if (!sel)
		{
			return;
		}

		clearCollisionFlags(m_modelPtrs);
		m_modelPtrs.clear();

		ModelGroup* pickGroup = pick->getModelGroup();
		QList<ModelGroup*> groups = sel->modelGroupsInside();
		/*if (pickGroup && !groups.contains(pickGroup))
		{
			groups.append(pickGroup);
		}*/
		
		for (auto m : groups)
		{
			QPointer<ModelGroup> p = m;
			m_modelPtrs.push_back(p);
			setStateMoving(m);
		}
		//m_processing = true;
	}

	void ByObjectEventHandler::onLeftMouseButtonRelease(QMouseEvent* event)
	{
		if (m_processing == false)
			return;

		/*clearCollisionFlags(m_modelPtrs);
		QList<ModelGroup*> list = _collisionCheck(m_modelPtrs);
		setStateCollide(list);

		_modelHeightCheckForSelectedPrinter();*/


		m_processing = false;
	}
	
	bool ByObjectEventHandler::processing()
	{
		return m_processing;
	}

	QList<ModelGroup*> ByObjectEventHandler::_modelHeightCheck(const QList<ModelGroup*>& inputModels)
	{
		ParameterManager* parameterManager = getKernel()->parameterManager();
		float maxH = parameterManager->extruderClearanceHeightToRod();
		return ByObjectEventHandler::heightCheck(inputModels, maxH);
	}


	QList<ModelGroup*> ByObjectEventHandler::_modelHeightCheckForSelectedPrinter()
	{
		Printer* sel = getPrinterManager()->getSelectedPrinter();
		if (!sel)
		{
			return QList<ModelGroup*>();
		}
		QList<ModelGroup*> models = sel->modelGroupsInside();
		QList<ModelGroup*> tallList = _modelHeightCheck(models);
		setStateTooTall(tallList);
		return tallList;

	}

	void ByObjectEventHandler::onExtruderClearanceHeightToLodChanged(float h)
	{
		updateHeightReferEntitySize();
		_modelHeightCheckForSelectedPrinter();
	}

	QList<ModelGroup*> ByObjectEventHandler::collisionCheck(const QList<ModelGroup*>& models)
	{
        //parallel one
        if (1)
        {
            auto start_time = std::chrono::high_resolution_clock::now();

            //will not do collision checking when less than one model group
            if (models.size() <= 1)
            {
                return QList<ModelGroup*>();
            }

            std::vector<topomesh::collision_check_info_t> infos;

            //1: gathering xy contour for model group which will involve in collision checking
            {
                //TODO: can using tbb nesting parallel_for 
                for (size_t i = 0; i < models.size(); ++i)
                {
                    std::vector<trimesh::vec3> contour = models[i]->rsPath();

                    //this model group will not going to checking if it doesn't have an xy contour
                    size_t point_num = contour.size();
                    if (0 == point_num)
                    {
                        continue;
                    }

                    topomesh::collision_check_info_t info;
                    info.contour.resize(point_num);

                    //get the model group id
                    info.model_group_index = i;
					//info.trans = ModelGroup

                    auto point_map = [&](tbb::blocked_range<size_t> r)
                    {
                        for (size_t i = r.begin(); i < r.end(); ++i)
                        {
                            info.contour[i][0] = contour[i][0];
                            info.contour[i][1] = contour[i][1];
                        }
                    };
                    tbb::parallel_for(tbb::blocked_range<size_t>(0, point_num), point_map);

                    infos.emplace_back(info);
                }
            }

            //2: offsetting the contour with clearance distance to get a clearance area
            {
                float radius = getKernel()->parameterManager()->extruderClearanceRadius() / 2.0;

                auto offset_map = [&](tbb::blocked_range<size_t> r)
                {
                    for (size_t i = r.begin(); i < r.end(); ++i)
                    {
                        //using negative value if contour is cw, which comply with clipper
                        bool is_cw = topomesh::contour_is_cw(infos[i].contour);
                        if (is_cw)
                        {
                            radius = -radius;
                        }

                        topomesh::contour_2d_t contour;
                        bool ret = topomesh::offset_contour(radius, infos[i].contour, contour);
                        if (ret)
                        {
                            infos[i].contour = std::move(contour);
                        }
                    }
                };

                tbb::parallel_for(tbb::blocked_range<size_t>(0, infos.size()), offset_map);
            }

            //3: update the bounding box
            {
                //TODO:using reduce to improve performance
                for (auto& info : infos)
                {
                    for (auto& point : info.contour)
                    {
                        info.bounding_box += point;
                    }
                }
            }

#ifdef DEBUG
            if (0)
            {
                topomesh::contour_to_svg(infos, "D://test//all_offsetting_contour.svg");
            }
#endif  DEBUG
            //4: do the collision checking
            {
                size_t info_size = infos.size();
                for (size_t i = 0; i < info_size - 1; ++i)
                {
                    auto intersect_map = [&](tbb::blocked_range<size_t> r)
                    {
                        for (size_t j = r.begin(); j < r.end(); ++j)
                        {
                            if (!infos[i].bounding_box.intersects(infos[j].bounding_box))
                            {
                                continue;
                            }

                            topomesh::contour_2d_t& contour0(infos[i].contour);
                            topomesh::contour_2d_t& contour1(infos[j].contour);

                            bool is_intersect = topomesh::contour_is_intersect(contour0, contour1);

                            if (is_intersect)
                            {
                                infos[i].intersected = true;
                                infos[j].intersected = true;
                            }
                        }
                    };
                    tbb::parallel_for(tbb::blocked_range<size_t>(i + 1, infos.size()), intersect_map);
                }
            }

            QList<ModelGroup*> return_value;
            //5: collect the intersecting model groups
            {
                for (auto& info : infos)
                {
                    if (info.intersected)
                    {
                        return_value.push_back(models[info.model_group_index]);
                    }
                }
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

            qDebug() << "new" << ms;

            return return_value;
        }

        if (0)
        {
            auto start_time = std::chrono::high_resolution_clock::now();

            std::vector<nestplacer::PR_Polygon> polys;
            std::vector<nestplacer::PR_RESULT> results;

            for (ModelGroup* m : models)
            {
				m->updateSweepAreaPath();

				std::vector<trimesh::vec3> paths = m->sweepAreaPath();
                polys.emplace_back(paths);
            }

            //check collide
            //std::string str("D:\\result.out");
            nestplacer::collisionCheck(polys, results, true, false, nullptr);

            QSet<int> collideIndexs;

            for (size_t i = 0; i < results.size(); i++)
            {
                const nestplacer::PR_RESULT& res = results.at(i);
                if (res.state == nestplacer::ContactState::INTERSECT)
                {
                    if (models.size() > res.first && models.size() > res.second)
                    {
                        collideIndexs.insert(res.first);
                        collideIndexs.insert(res.second);
                    }
                }
            }

            QList<int> indexList = collideIndexs.toList();
            qSort(indexList.begin(), indexList.end());

            QList<ModelGroup*> collideModels;

            for (QList<int>::iterator it = indexList.begin(); it != indexList.end(); ++it)
            {
                int idx = *it;
                if (0 <= idx && idx < models.size())
                {
                    auto m = models.at(idx);
                    collideModels.push_back(m);
                }
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
            qDebug() << "old" << ms;

            return collideModels;
        }
	}

	QList<ModelGroup*> ByObjectEventHandler::heightCheck(const QList<ModelGroup*>& inputModels, float heightToRod)
	{
		QList<ModelGroup*> models;
		for (auto m : inputModels)
		{
			qtuser_3d::Box3D modelBox = m->globalSpaceBox();
			if (heightToRod > 0 && modelBox.max.z() > heightToRod)
			{
				models.push_back(m);
			}
		}

		return models;
	}

	void ByObjectEventHandler::updateHeightReferEntitySize()
	{
		float maxH = extruderClearanceHeightToRod();

		qtuser_3d::Box3D box = getSelectedPrinter()->globalBox();

		QMatrix4x4 m;
		m.translate(box.min);

		QVector3D size = box.size();
		size.setZ(maxH);

		m.scale(size);

		getKernel()->visScene()->heightReferEntity()->setPose(m);
	}

	float extruderClearanceHeightToRod()
	{
		ParameterManager* parameterManager = getKernel()->parameterManager();
		float maxH = parameterManager->extruderClearanceHeightToRod();
		return maxH;
	}
}

