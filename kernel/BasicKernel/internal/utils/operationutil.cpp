#include "operationutil.h"
#include "interface/camerainterface.h"
#include "data/modeln.h"
#include "qtuser3d/math/space3d.h"

double EPSILON = 1e-3;

namespace creative_kernel
{
	bool checkLayerHeightEqual(const std::vector<std::vector<double>>& objectsLayers)
	{
		/* ȫ���̶���� */
		if (std::all_of(objectsLayers.begin(), objectsLayers.end(),
			[](const std::vector<double>& layer) {
				return layer.empty();
			})) {
			return true;
		}

		/* ����Ӧ��ߺ͹̶����ͬʱ���ڣ�����false */
		if (std::any_of(objectsLayers.begin(), objectsLayers.end(), [](const std::vector<double>& layer) {
			return layer.empty();
			})) {
			return false;
		}

		auto areClose = [](double a, double b) {
			return std::abs(a - b) <= EPSILON;
		};

		/* ��ȡ���Ĳ��� */
		auto maxIt = std::max_element(objectsLayers.begin(), objectsLayers.end(),
			[](const std::vector<double>& a, const std::vector<double>& b) {
				return a.size() < b.size();
			});
		size_t maxLayerSize = (*maxIt).size();
		size_t maxLayerIndex = std::distance(objectsLayers.begin(), maxIt);

		/* ���Ƚϲ��, ���������򷵻�false */
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


