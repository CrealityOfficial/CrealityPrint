#pragma once

#include "GEOM/Point.hpp"
#include "trimesh2/TriMesh.h"
#include <memory>
#include <vector>

namespace MESHTOOL {

	using TriMeshPtr = std::shared_ptr<trimesh::TriMesh>;

	class UnitConverter {
	public:

		UnitConverter();

		float getFactor() const;
		// 将内部单位转换为外部单位
		float toOutside(const GEOM::GeomNumType& insideValue) const;
		std::array<float, 3> toOutside(const GEOM::Point& point) const;
		// 将外部单位转换为内部单位
		GEOM::GeomNumType toInside(const float& outsideValue) const;
		GEOM::Point toInside(const trimesh::point& point) const;

	private:
		// 这里是缩放因子，目前3D打印使用的数据格式是TriMesh，TriMesh的基本数值类型是float
		// 3D打印的尺度是毫米，float的有效位数是7位，因此使用TriMesh最低能够表示0.1纳米。
		// 为解决float转成整型零误差，这里设置缩放系数为1000000000，表示几何体的基本单位是0.001纳米，
		float factor = 1000000000;

	};

}  // namespace MESHTOOL
