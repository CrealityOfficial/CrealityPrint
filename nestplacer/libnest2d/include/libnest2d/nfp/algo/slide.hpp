#ifndef SRC_ALGO_SLIDE_HPP_
#define SRC_ALGO_SLIDE_HPP_

#include "../geometry.hpp"
#include "../translation_vector.hpp"

namespace libnfporb {

/**
 * Indicates the result of the slide function
 */
enum SlideResult {
	LOOP,         //!< we were able to successfully create a NFP-loop
	NO_LOOP,      //!< we were not because we ran out of feasible translations
	NO_TRANSLATION//!< we were unable to find a valid next position
};

/**
 * The function actually performs the orbiting by sliding one ring around the other.
 * @param pA Polygon A
 * @param rA Ring of A
 * @param rB Ring of B
 * @param nfp The NFP reference to write the traversal to.
 * @param startTrans The initial translation to perform on rB.
 * @param inside We are sliding on the inside, which means we previously determined that we are in a hole.
 * @throws runtime_error on internal errors
 * @return A SlideResult, which indicates:
 * we were able to successfully create a NFP-loop (LOOP) or,
 * we were not because we ran out of feasible translations (NO_LOOP) or
 * we were unable to find a valid next position (NO_TRANSLATION)
 */
SlideResult slide(polygon_t& pA, polygon_t::ring_type& rA, polygon_t::ring_type& rB, nfp_t& nfp, const point_t& startTrans, bool inside) {
	polygon_t::ring_type rifsB;
	boost::geometry::transform(rB, rifsB, trans::translate_transformer<coord_t, 2, 2>(startTrans.x_, startTrans.y_));
	rB = std::move(rifsB);

#ifdef NFP_DEBUG
	write_svg("ifs.svg", pA, rB);
#endif

	bool startAvailable = true;
	psize_t cnt = 0;
	point_t referenceStart = rB.front();
	History history;

	//generate the nfp for the ring
	while (startAvailable) {
		DEBUG_VAL("");
		DEBUG_VAL("");
		DEBUG_VAL("#### iteration: " + std::to_string(cnt) + " ####");

		//use first point of rB as reference
		nfp.back().push_back(rB.front());
		if (cnt == 15)
			std::cerr << "";

		std::vector<TouchingPoint> touchers = find_touching_points(rA, rB);

		DEBUG_MSG("touchers", touchers.size());

		if (touchers.empty()) {
			throw std::runtime_error("Internal error: No touching points found");
		}
		std::vector<TranslationVector> feasibleVectors = find_feasible_translation_vectors(rA, rB, touchers);


#ifdef NFP_DEBUG
		DEBUG_MSG("feasible vectors", feasibleVectors.size());
		for(auto pt : feasibleVectors) {
			DEBUG_VAL(pt);
		}
#endif

		if (feasibleVectors.empty()) {
			return NO_LOOP;
		}

		TranslationVector next = select_next_translation_vector(nfp, pA, rA, rB, feasibleVectors, history);

		if (equals(next.vector_, INVALID_POINT))
			return NO_TRANSLATION;

		TranslationVector trimmed = trim_vector(rA, rB, next);
		DEBUG_MSG("trimmed", trimmed);

		DEBUG_MSG("next", next);
		history.push_back(next);

		polygon_t::ring_type nextRB;
		boost::geometry::transform(rB, nextRB, trans::translate_transformer<coord_t, 2, 2>(trimmed.vector_.x_, trimmed.vector_.y_));
		rB = std::move(nextRB);

#ifdef NFP_DEBUG
		write_svg("next" + std::to_string(cnt) + ".svg", pA,rB);
#endif
		if(bg::overlaps(pA, rB))
			DEBUG_VAL("Internal error: Slide resulted in overlap");
		//	throw std::runtime_error("Internal error: Slide resulted in overlap");

		++cnt;
		if (equals(referenceStart, rB.front()) || (inside && bg::touches(rB.front(), nfp.front()))) {
			startAvailable = false;
		}
	}
	return LOOP;
}
}


#endif /* SRC_ALGO_SLIDE_HPP_ */
