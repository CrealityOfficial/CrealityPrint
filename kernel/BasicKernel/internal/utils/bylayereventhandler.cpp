#include "bylayereventhandler.h"
#include "data/modelgroup.h"
#include "nestplacer/printpiece.h"
#include "interface/printerinterface.h"
#include "internal/multi_printer/printermanager.h"
#include "kernel/kernelui.h"
#include "external/message/modelcollisionmessage.h"
#include "data/spaceutils.h"

#include "topomesh/interface/utils.h"
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <functional>
#include <chrono>
#include <qtuser3d/trimesh2/conv.h>

namespace creative_kernel
{
	ByLayerEventHandler::ByLayerEventHandler(QObject* parent)
		:PMCollisionCheckHandler(parent)
	{
		
	}
	
	ByLayerEventHandler::~ByLayerEventHandler()
	{
		clearCollisionFlags();
	}

	void ByLayerEventHandler::onLeftMouseButtonRelease(QMouseEvent* event)
	{

		/*Printer* sel = getPrinterManager()->getSelectedPrinter();
		if (!sel)
		{
			return;
		}
		QList<ModelGroup*> models = sel->modelGroupsInside();
		checkModelGroups(models);*/
	}

	void ByLayerEventHandler::clearCollisionFlags()
	{
		getKernelUI()->destroyMessage(MessageType::ModelCollision);
	}

	QList<ModelGroup*> ByLayerEventHandler::checkModelGroups(const QList<ModelGroup*>& groups)
	{
        QList<ModelGroup*> collideModels = ByLayerEventHandler::collisionCheck(groups);

        if (collideModels.size() > 0)
        {
            ModelNCollisionMessage* msg = new ModelNCollisionMessage(collideModels);
            msg->setLevel(1);
            getKernelUI()->requestMessage(msg);
        }
        else {
            getKernelUI()->destroyMessage(MessageType::ModelCollision);
        }
        return collideModels;

#if 0
		if (groups.isEmpty())
		{
			getKernelUI()->destroyMessage(MessageType::ModelCollision);
			return QList<ModelGroup*>();
		}

		std::vector<nestplacer::PR_Polygon> polys;
		std::vector<nestplacer::PR_RESULT> results;

		for (ModelGroup* m : groups)
		{
			std::vector<trimesh::vec3> paths = m->rsPath();
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
				if (groups.size() > res.first && groups.size() > res.second)
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
			if (0 <= idx && idx < groups.size())
			{
				auto m = groups.at(idx);
				collideModels.push_back(m);
			}
		}

		if (collideModels.size() > 0)
		{
			ModelNCollisionMessage* msg = new ModelNCollisionMessage(collideModels);
			msg->setLevel(1);
			getKernelUI()->requestMessage(msg);
		}
		else {
			getKernelUI()->destroyMessage(MessageType::ModelCollision);
		}

		return collideModels;

#endif // 0

	}

	QList<ModelGroup*> ByLayerEventHandler::collisionCheck(const QList<ModelGroup*>& models)
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
        //passing

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
}