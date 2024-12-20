
#ifndef SRC_ALGO_TOUCHING_POINT_HPP_
#define SRC_ALGO_TOUCHING_POINT_HPP_

#include "../geometry.hpp"

namespace libnfporb {
/**
 * Stores information about a touching point
 */
struct TouchingPoint {
	/**
	 * Indicates if the touching point is a result of:
	 * 1. Two vertices touching -> VERTEX
	 * 2. A vertex of A touching a segment of B -> A_ON_B
	 * 3. A vertex of B touching a segment of A -> B_ON_A
	 */
	enum Type {
		VERTEX,//!< VERTEX
		A_ON_B,//!< A_ON_B
		B_ON_A, //!< B_ON_A
		NULLTYPE
	};
	Type type_;
	/**
	 * The index of the touching point in ringA
	 */
	psize_t A_;
	/**
	 * The index of the touching point in ringB
	 */
	psize_t B_;
	TouchingPoint()
	{
		type_ = NULLTYPE;
		A_ = -1;
		B_ = -1;
	}

	TouchingPoint(Type type, psize_t a, psize_t b)
	{
		type_ = type;
		A_ = a;
		B_ = b;
	}
};

/**
 * @brief Search for touching points of two rings of A and B
 * @param ringA Ring of A
 * @param ringB Ring of B
 * @return A %vector of %TouchingPoint objects
 */
std::vector<TouchingPoint> find_touching_points(const polygon_t::ring_type& ringA, const polygon_t::ring_type& ringB) {
	std::vector<TouchingPoint> touchers(ringA.size());
#pragma omp parallel for
	for (int i = 0; i < ringA.size() - 1; i++) {
		psize_t idx_i = i;
		psize_t nextI = i + 1;
		for (psize_t j = 0; j < ringB.size() - 1; j++) {
			psize_t nextJ = j + 1;
			if (equals(ringA[i], ringB[j])) {
				DEBUG_MSG("vertex", segment_t(ringA[i],ringB[j]));
				touchers[i] = TouchingPoint(TouchingPoint::VERTEX, idx_i, j);
			} else if (!equals(ringA[nextI], ringB[j]) && bg::intersects(segment_t(ringA[i], ringA[nextI]), ringB[j])) {
				DEBUG_MSG("bona", segment_t(ringA[i],ringB[j]));
				touchers[i] = TouchingPoint(TouchingPoint::B_ON_A, nextI, j);
			} else if (!equals(ringB[nextJ], ringA[i]) && bg::intersects(segment_t(ringB[j], ringB[nextJ]), ringA[i])) {
				DEBUG_MSG("aonb", segment_t(ringA[i],ringB[j]));
				touchers[i] = TouchingPoint(TouchingPoint::A_ON_B, idx_i, nextJ);
			}
		}
	}
	std::vector<TouchingPoint> touchers_dst;
	for (int i = 0; i < touchers.size(); i++)
	{
		if (touchers[i].type_ != TouchingPoint::NULLTYPE)
			touchers_dst.push_back(touchers[i]);
	}

	return touchers_dst;
}
}


#endif /* SRC_ALGO_TOUCHING_POINT_HPP_ */
