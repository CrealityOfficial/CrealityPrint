#ifndef slic3r_FillQuarter_hpp_
#define slic3r_FillQuarter_hpp_

#include "FillBase.hpp"
#include "Fill/Quarter/ZigzagConnectorProcessor.h"

namespace Slic3r {

	class FillQuarter : public Fill
	{
	public:
		~FillQuarter() override = default;
        bool is_self_crossing() override { return false; }
		void setOrigin(const InfillPattern& _pattern,
			const Point& _infill_origin_internal,
			const Point& _offset_internal,
			const coord_t& _z_microns,
			const coord_t& _line_distance_microns,
			const coord_t& _infill_line_width_microns);
	protected:
		Fill* clone() const override { return new FillQuarter(*this); };
		void _fill_surface_single(
			const FillParams& params,
			unsigned int                     thickness_layers,
			const std::pair<float, Point>& direction,
			ExPolygon     		             expolygon,
			Polylines& polylines_out) override;

		void _fill_surface_single(const FillParams& params,
			unsigned int                   thickness_layers,
			const std::pair<float, Point>& direction,
			ExPolygon                      expolygon,
			ThickPolylines& thick_polylines_out) override;

		bool no_sort() const override { return true; }
	protected:
		void generateLinearBasedInfill(Polygons& result, const int line_distance, const PointMatrix& rotation_matrix, ZigzagConnectorProcessor& zigzag_connector_processor, const bool connected_zigzags, coord_t extra_shift);

		void generateLineInfill(Polygons& result, int line_distance, const double& infill_rotation, coord_t shift);

		coord_t getShiftOffsetFromInfillOriginAndRotation(const double& infill_rotation);

		void generateHalfTetrahedralInfill(float pattern_z_shift, int angle_shift, Polygons& result);

		void addLineInfill(Polygons& result, const PointMatrix& rotation_matrix, const int scanline_min_idx, const int line_distance, const BoundingBox boundary, std::vector<std::vector<coord_t>>& cut_list, coord_t shift);

	private:
		InfillPattern m_pattern;

		Polygons outer_contour;
		Polygons inner_contour;

		Point current_position{ Point(0,0) };

		// The imported Cura algorithm operates in integer micrometres. These
		// values, along with inner_contour, stay in that domain while generating.
		Point infill_origin; //!< origin of the infill pattern, in micrometres
		Point offset;        //!< local-to-model offset, in micrometres

		coord_t line_distance{ 5600 };  //!< micrometres
		coord_t infill_line_width{420}; //!< micrometres
		float fill_angle{45.0f};

		coord_t z; //!< micrometres
	};

} // namespace Slic3r

#endif // slic3r_FillQuarter_hpp_
