#ifndef NESTPLACER_PLACER_1702004306712_H
#define NESTPLACER_PLACER_1702004306712_H
#include "nestplacer/export.h"
#include "ccglobal/tracer.h"

#include "trimesh2/Box.h"
#include <functional>
#include <vector>

namespace nestplacer
{
	struct  PlacerResultRT 
	{
		trimesh::vec3 rt; // x, y translation  z rotation angle
		int binIndex = -1;
	};

	struct PlacerItemGeometry
	{
		int binIndex = -1;
		std::vector<trimesh::vec3> outline;
		std::vector<std::vector<trimesh::vec3>> holes;
	};

	class PlacerItem
	{
	public:
		virtual ~PlacerItem() {}
		virtual void polygon(PlacerItemGeometry& geometry) = 0;  // loop polygon
	};

	struct PlacerParameter
	{
		float itemGap = 1.0f;
		float binItemGap = 1.0f;
		bool rotate = false;
		float rotateAngle = 30.0f;
        int startPoint = 0;
        /*
        @param align_mode:
        0 - CENER, 1 - BOTTOM_LEFT, 2 - BOTTOM_RIGHT,
        3 - TOP_LEFT, 4 - TOP_RIGHT, 5 -  DONT_ALIGN;
        if is DONT_ALIGN, is same with no needAlign.
        */
        bool needAlign = false;
        int alignMode = 0;
        bool concaveCal = false;
        trimesh::box3 box;
		ccglobal::Tracer* tracer = nullptr;

		//paramter for mutiPlates.
		std::vector<trimesh::box3> multiBins;
		float binDist = 10.0f;
		int curBinId = 0;

		//debug
		std::string fileName;
	};

	class BinExtendStrategy
	{
	public:
		virtual ~BinExtendStrategy() {}

		virtual trimesh::box3 bounding(int index) const = 0;
		virtual trimesh::box3 fixBounding(int index) const = 0;
	};
	NESTPLACER_API void loadDataFromFile(const std::string& fileName, std::vector<PlacerItem*>& fixed, std::vector<PlacerItem*>& actives, PlacerParameter& parameter);
	NESTPLACER_API void placeFromFile(const std::string& fileName, std::vector<PlacerResultRT>& results, const BinExtendStrategy& binExtendStrategy, ccglobal::Tracer* tracer);
	NESTPLACER_API void extendFillFromFile(const std::string& fileName, std::vector<PlacerResultRT>& results, const BinExtendStrategy& binExtendStrategy, ccglobal::Tracer* tracer);
	/// <summary>
	/// 此接口作为libnest2d库的包装接口，主要进行输入的二维轮廓的预处理，
	/// 以及排料算法库libnest2d的配置函数及回调函数的设置。
	/// </summary>
	/// <param name="fixed">相对固定不动的输入的二维轮廓</param>
	/// <param name="actives">可以进行旋转平移挪动的二维轮廓</param>
	/// <param name="parameter">自动布局配置参数</param>
	/// <param name="results">返回所有轮廓对应的旋转平移量</param>   result rt, same size with actives
	/// <param name="binExtendStrategy">输入的箱子/盘延展策略</param>  bin extend strategy
	/// <returns></returns>
    NESTPLACER_API void place(const std::vector<PlacerItem*>& fixed, const std::vector<PlacerItem*>& actives,
        const PlacerParameter& parameter, std::vector<PlacerResultRT>& results, const BinExtendStrategy& binExtendStrategy);

	/// <summary>
	/// 满铺打印算法：同样也是libnest2d的包装接口，只不过传入的二维轮廓略微不同。
	/// </summary>
	/// <param name="fixed">相对固定不动的输入的二维轮廓</param>
	/// <param name="active">待满铺打印对象的二维轮廓</param>
	/// <param name="parameter">自动布局配置参数</param>
	/// <param name="results">返回所有个体对应的旋转平移量</param>  result clone positions
	/// <returns></returns>
	NESTPLACER_API void extendFill(const std::vector<PlacerItem*>& fixed, PlacerItem* active,
		const PlacerParameter& parameter, std::vector<PlacerResultRT>& results);

	class NESTPLACER_API YDefaultBinExtendStrategy : public BinExtendStrategy
	{
	public:
		YDefaultBinExtendStrategy(const trimesh::box3& box, float dy);
		virtual ~YDefaultBinExtendStrategy();

		trimesh::box3 bounding(int index) const override;
		trimesh::box3 fixBounding(int index) const override { return m_box; }
	protected:
		trimesh::box3 m_box;
		float m_dy;
	};

    class NESTPLACER_API DiagonalBinExtendStrategy : public BinExtendStrategy {
    public:
		DiagonalBinExtendStrategy(const trimesh::box3& box, float ratio);
        virtual ~DiagonalBinExtendStrategy();

        trimesh::box3 bounding(int index) const override;
		trimesh::box3 fixBounding(int index) const override { return m_box; }
    protected:
        trimesh::box3 m_box;
        float m_ratio;
    };

}

#endif // NESTPLACER_PLACER_1702004306712_H