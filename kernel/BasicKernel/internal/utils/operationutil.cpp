#include "operationutil.h"
#include "interface/camerainterface.h"
#include "data/modeln.h"
#include "qtuser3d/math/space3d.h"

double EPSILON = 1e-3;

namespace creative_kernel
{
	bool checkLayerHeightEqual(const std::vector<std::vector<double>>& objectsLayers)
	{
		/* 全部固定层高 */
		if (std::all_of(objectsLayers.begin(), objectsLayers.end(),
			[](const std::vector<double>& layer) {
				return layer.empty();
			})) {
			return true;
		}

		/* 自适应层高和固定层高同时存在，返回false */
		if (std::any_of(objectsLayers.begin(), objectsLayers.end(), [](const std::vector<double>& layer) {
			return layer.empty();
			})) {
			return false;
		}

		auto areClose = [](double a, double b) {
			return std::abs(a - b) <= EPSILON;
		};

		/* 获取最大的层数 */
		auto maxIt = std::max_element(objectsLayers.begin(), objectsLayers.end(),
			[](const std::vector<double>& a, const std::vector<double>& b) {
				return a.size() < b.size();
			});
		size_t maxLayerSize = (*maxIt).size();
		size_t maxLayerIndex = std::distance(objectsLayers.begin(), maxIt);

		/* 逐层比较层高, 如果不相等则返回false */
		for (size_t heightIdx = 0; heightIdx < maxLayerSize; heightIdx++) {
			const double layerHeight = (*maxIt)[heightIdx];
			for (size_t layerIdx = 0; layerIdx < objectsLayers.size(); layerIdx++) {
				if (layerIdx != maxLayerIndex && heightIdx < objectsLayers[layerIdx].size() &&
					!areClose(layerHeight, objectsLayers[layerIdx][heightIdx])) {
					return false;
				}
			}
		}
		return true;
	}
}


