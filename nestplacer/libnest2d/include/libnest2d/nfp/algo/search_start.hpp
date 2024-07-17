#ifndef SRC_ALGO_SEARCH_START_HPP_
#define SRC_ALGO_SEARCH_START_HPP_

#include "../geometry.hpp"
#include "../translation_vector.hpp"

namespace libnfporb {

/**
 * @brief Indicating the result of %search_start_translation
 */
enum SearchStartResult {
	FIT,     //!< FIT
	FOUND,   //!< FOUND
	NOT_FOUND//!< NOT_FOUND
};

/**
 * @brief At the start of traversal and after every completed nfp-ring this function is used to search for a new start position.
 * @param rA The ring of polygon A to search for a new start position.
 * @param rB The ring of polygon B to try to fit into a new start position.
 * @param nfp The NFP so far.
 * @param result The reference, the translation vector that would lead to a new start position, is written to.
 *
 * @return A %SearchStartResult that indicates if either a perfect fit, a new start position or new possible position was found
 */
SearchStartResult search_start_translation(polygon_t::ring_type& rA, const polygon_t::ring_type& rB, const nfp_t& nfp, const bool& inside, point_t& result) {
	for (psize_t i = 0; i < rA.size() - 1; i++) {
		psize_t index;
		if (i >= rA.size())
			index = i % rA.size() + 1;
		else
			index = i;

		auto& ptA = rA[index];

		if (ptA.marked_)
			continue;

		ptA.marked_ = true;

		for (const auto& ptB : rB) {
			point_t testTranslation = ptA - ptB;
			polygon_t::ring_type translated;
			boost::geometry::transform(rB, translated, trans::translate_transformer<coord_t, 2, 2>(testTranslation.x_, testTranslation.y_));

			//check if translated rB is part of rA
			bool identical = false;
			for (const auto& ptA : rA) {
				identical = false;
				for (const auto& ptT : translated) {
					if (equals(ptT, ptA)) {
						identical = true;
						break;
					} else if(identical) {
						identical = false;
						break;
					}
				}
				if (!identical)
					break;
			}

			if (identical) {
				result = testTranslation;
				return FIT;
			}

			bool bInside = false;
			for (const auto& ptT : translated) {
				if (bg::within(ptT, rA)) {
					bInside = true;
					break;
				} else if (!bg::touches(ptT, rA)) {
					bInside = false;
					break;
				}
			}

			if (((bInside && inside) || (!bInside && !inside)) && (!bg::overlaps(translated, rA) && !bg::covered_by(translated, rA) && !bg::covered_by(rA, translated)) && !in_nfp(translated.front(), nfp)) {
				result = testTranslation;
				return FOUND;
			}

			point_t nextPtA = rA[index + 1];
			TranslationVector slideVector;
			slideVector.vector_ = nextPtA - ptA;
			slideVector.edge_ = {ptA, nextPtA};
			slideVector.fromA_ = true;
			TranslationVector trimmed = trim_vector(rA, translated, slideVector);
			polygon_t::ring_type translated2;
			trans::translate_transformer<coord_t, 2, 2> trans(trimmed.vector_.x_, trimmed.vector_.y_);
			boost::geometry::transform(translated, translated2, trans);

			//check if translated rB is part of rA
			identical = false;
			for (const auto& ptA : rA) {
				identical = false;
				for (const auto& ptT : translated) {
					if (equals(ptT, ptA)) {
						identical = true;
						break;
					} else if(identical) {
						identical = false;
						break;
					}
				}
				if (!identical)
					break;
			}

			if (identical) {
				result = trimmed.vector_ + testTranslation;
				return FIT;
			}

			bInside = false;
			for (const auto& ptT : translated2) {
				if (bg::within(ptT, rA)) {
					bInside = true;
					break;
				} else if (!bg::touches(ptT, rA)) {
					bInside = false;
					break;
				}
			}

			if (((bInside && inside) || (!bInside && !inside)) && (!bg::overlaps(translated2, rA) && !bg::covered_by(translated2, rA) && !bg::covered_by(rA, translated2)) && !in_nfp(translated2.front(), nfp)) {
				result = trimmed.vector_ + testTranslation;
				return FOUND;
			}
		}
	}
	return NOT_FOUND;
}
}



#endif /* SRC_ALGO_SEARCH_START_HPP_ */
