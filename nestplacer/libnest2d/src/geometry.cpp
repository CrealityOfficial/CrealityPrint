#include "libnest2d/nfp/geometry.hpp"

namespace libnfporb {
	std::ostream& operator<<(std::ostream& os, const LongDouble& c) {
		os << toLongDouble(c);
		return os;
	}

	std::istream& operator>>(std::istream& is, LongDouble& c) {
		long double val;
		is >> val;
		c.setVal(val);
		return is;
	}

	std::ostream& operator<<(std::ostream& os, const point_t& p) {
		os << "{" << toLongDouble(p.x_) << "," << toLongDouble(p.y_) << "}";
		return os;
	}
}

namespace libnfporb {
	bool equals(const LongDouble& lhs, const LongDouble& rhs) {
		if (lhs.val() == rhs.val())
			return true;

		return bg::math::detail::abs<libnfporb::LongDouble>::apply(lhs.val() - rhs.val()) <= libnfporb::NFP_EPSILON * std::fmax(
			bg::math::detail::abs<libnfporb::LongDouble>::apply(lhs.val()).val(),
			bg::math::detail::abs<libnfporb::LongDouble>::apply(rhs.val()).val());
	}

	bool equals(const point_t& lhs, const point_t& rhs) {
		return equals(lhs.x_, rhs.x_) && equals(lhs.y_, rhs.y_);
	}

	bool equals(const segment_t& lhs, const segment_t& rhs) {
		return equals(lhs.first, rhs.first) && equals(lhs.second, rhs.second);
	}
	std::ostream& operator<<(std::ostream& os, const segment_t& seg) {
		os << "{" << seg.first << "," << seg.second << "}";
		return os;
	}

	bool operator<(const segment_t& lhs, const segment_t& rhs) {
		return lhs.first < rhs.first || (equals(lhs.first, rhs.first) && (lhs.second < rhs.second));
	}

	point_t normalize(const point_t& vec) {
		point_t norm = vec;
		coord_t len = bg::length(segment_t{ { 0, 0 }, vec });

		if (len == 0.0L)
			return { 0,0 };

		norm.x_ /= len;
		norm.y_ /= len;

		return norm;
	}

	point_t flip(const point_t& vec) {
		point_t flipped = vec;

		flipped.x_ *= -1;
		flipped.y_ *= -1;

		return flipped;
	}

	bool is_parallel(const segment_t& s1, const segment_t& s2) {
		coord_t dx1 = s1.first.x_ - s2.first.x_;
		coord_t dy1 = s1.first.y_ - s2.first.y_;
		coord_t dx2 = s1.second.x_ - s2.second.x_;
		coord_t dy2 = s1.second.y_ - s2.second.y_;

		return dx1 == dx2 && dy2 == dy1;
	}

	Alignment get_alignment(const segment_t& seg, const point_t& pt) {
		coord_t res = ((seg.second.x_ - seg.first.x_) * (pt.y_ - seg.first.y_)
			- (seg.second.y_ - seg.first.y_) * (pt.x_ - seg.first.x_));

		if (equals(res, 0)) {
			return ON;
		}
		else if (larger(res, 0)) {
			return LEFT;
		}
		else {
			return RIGHT;
		}
	}

	long double get_inner_angle(const point_t& joint, const point_t& end1, const point_t& end2) {
		coord_t dx21 = end1.x_ - joint.x_;
		coord_t dx31 = end2.x_ - joint.x_;
		coord_t dy21 = end1.y_ - joint.y_;
		coord_t dy31 = end2.y_ - joint.y_;
		coord_t m12 = sqrt((dx21 * dx21 + dy21 * dy21));
		coord_t m13 = sqrt((dx31 * dx31 + dy31 * dy31));
		if (m12 == 0.0L || m13 == 0.0L)
			return 0;
		return acos((dx21 * dx31 + dy21 * dy31) / (m12 * m13));
	}

	std::vector<psize_t> find_minimum_x(const polygon_t& p) {
		std::vector<psize_t> result;
		coord_t min = MAX_COORD;
		auto& po = p.outer();
		for (psize_t i = 0; i < p.outer().size() - 1; ++i) {
			if (smaller(po[i].x_, min)) {
				result.clear();
				min = po[i].x_;
				result.push_back(i);
			}
			else if (equals(po[i].x_, min)) {
				result.push_back(i);
			}
		}
		return result;
	}

	std::vector<psize_t> find_maximum_x(const polygon_t& p) {
		std::vector<psize_t> result;
		coord_t max = MIN_COORD;
		auto& po = p.outer();
		for (psize_t i = 0; i < p.outer().size() - 1; ++i) {
			if (larger(po[i].x_, max)) {
				result.clear();
				max = po[i].x_;
				result.push_back(i);
			}
			else if (equals(po[i].x_, max)) {
				result.push_back(i);
			}
		}
		return result;
	}

	std::vector<psize_t> find_minimum_y(const polygon_t& p) {
		std::vector<psize_t> result;
		coord_t min = MAX_COORD;
		auto& po = p.outer();
		for (psize_t i = 0; i < p.outer().size() - 1; ++i) {
			if (smaller(po[i].y_, min)) {
				result.clear();
				min = po[i].y_;
				result.push_back(i);
			}
			else if (equals(po[i].y_, min)) {
				result.push_back(i);
			}
		}
		return result;
	}

	std::vector<psize_t> find_maximum_y(const polygon_t& p) {
		std::vector<psize_t> result;
		coord_t max = MIN_COORD;
		auto& po = p.outer();
		for (psize_t i = 0; i < p.outer().size() - 1; ++i) {
			if (larger(po[i].y_, max)) {
				result.clear();
				max = po[i].y_;
				result.push_back(i);
			}
			else if (equals(po[i].y_, max)) {
				result.push_back(i);
			}
		}
		return result;
	}

	psize_t find_point(const polygon_t::ring_type& ring, const point_t& pt) {
		for (psize_t i = 0; i < ring.size(); ++i) {
			if (equals(ring[i], pt))
				return i;
		}
		return std::numeric_limits<psize_t>::max();
	}

	/**
	 * @brief Checks if a point exists in a NFP
	 * @param pt The point to look for
	 * @param nfp The NFP to search
	 * @return true if the point was found
	 */
	bool in_nfp(const point_t& pt, const nfp_t& nfp) {
		for (const auto& r : nfp) {
			if (bg::touches(pt, r))
				return true;
		}

		return false;
	}
}
