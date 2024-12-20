#ifndef _NESTPLACER_H
#define _NESTPLACER_H
#include "nestplacer/export.h"
#include "ccglobal/tracer.h"

#include "trimesh2/Box.h"
#include <functional>
#include <vector>

namespace nestplacer
{
	enum class PlaceType {
		CENTER_TO_SIDE,
		ALIGNMENT,
		ONELINE,
		CONCAVE,
		TOP_TO_BOTTOM,
		BOTTOM_TO_TOP,
		LEFT_TO_RIGHT,
		RIGHT_TO_LEFT,
		CONCAVE_ALL,
		NULLTYPE
	};

	enum class StartPoint {
		CENTER,
		TOP_LEFT,
		TOP_RIGHT,
		BOTTOM_LEFT,
		BOTTOM_RIGHT,
		NULLTYPE
	};

	struct NestParaFloat
	{
		trimesh::box3 workspaceBox;
		float modelsDist;
		PlaceType packType;
		bool parallel;
		int rotationStep;
		ccglobal::Tracer* tracer = nullptr;

		NestParaFloat()
		{
			workspaceBox = trimesh::box3();
			packType = PlaceType::CENTER_TO_SIDE;
			modelsDist = 3.0f;
			parallel = true;
			rotationStep = 8;
		}

		NestParaFloat(trimesh::box3 workspace, float dist, PlaceType type, bool _parallel, int _rotationStep = 8)
		{
			workspaceBox = workspace;
			modelsDist = dist;
			packType = type;
			parallel = _parallel;
			rotationStep = _rotationStep;
		}
	};

    struct NestPara {
        NestPara() {}
        trimesh::box3 workspaceBox = trimesh::box3();
        float modelsDist = 3.0f;
        float edgeDist = 1.0f;
        PlaceType packType = PlaceType::CENTER_TO_SIDE;
        bool parallel = true;
        float rotationAngle = 20.0f;

        NestPara(trimesh::box3 workspace, float dist, float offset, PlaceType type, bool _parallel, float _rotationAngle = 20.0f)
        {
            workspaceBox = workspace;
            modelsDist = dist;
            edgeDist = offset;
            packType = type;
            parallel = _parallel;
            rotationAngle = _rotationAngle;
        }
    };

    class NestPlacerDebugger {
    public:
        virtual ~NestPlacerDebugger() {}

        virtual void onUpdate(int index, const trimesh::vec3& rt) = 0;
    };

	/*所有模型布局*/
	NESTPLACER_API void layout_all_nest(const std::vector < std::vector<trimesh::vec3>>& models, std::vector<int> modelIndices,
		NestParaFloat para, std::function<void(int, trimesh::vec3)> modelPositionUpdateFunc);
	/*新增单个模型布局*/
	NESTPLACER_API bool layout_new_item(const std::vector < std::vector<trimesh::vec3>>& models, const std::vector<trimesh::vec3>& transData,
		const std::vector<trimesh::vec3>& NewItem, NestParaFloat para, std::function<void(trimesh::vec3)> func);
	/*新增多个模型布局*/
	NESTPLACER_API void layout_new_items(const std::vector < std::vector<trimesh::vec3>>& models, const std::vector<trimesh::vec3>& transData,
		const std::vector < std::vector<trimesh::vec3>>& NewItems, NestParaFloat para, std::function<void(int, trimesh::vec3)> func);
    
    NESTPLACER_API void layout_all_nest2(const std::vector < std::vector<trimesh::vec3>>& models, std::vector<int> modelIndices,
        NestPara para, std::function<void(int, trimesh::vec3)> modelPositionUpdateFunc, NestPlacerDebugger* debugger = nullptr);


}



#endif