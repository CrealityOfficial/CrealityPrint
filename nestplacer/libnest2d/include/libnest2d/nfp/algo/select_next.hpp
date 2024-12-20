#ifndef SRC_ALGO_SELECT_NEXT_HPP_
#define SRC_ALGO_SELECT_NEXT_HPP_

#include "../geometry.hpp"
#include "../translation_vector.hpp"
#include "../history.hpp"

namespace libnfporb {
/**
 * @brief Find the longest translation vector.
 * @param tvs The translation vectors.
 * @return The longest translation vector.
 */
TranslationVector find_longest(const std::vector<TranslationVector>& tvs) {
	coord_t len;
	coord_t maxLen = MIN_COORD;
	TranslationVector longest;
	longest.vector_ = INVALID_POINT;

	for (auto& tv : tvs) {
		len = bg::length(segment_t { { 0, 0 }, tv.vector_ });
		if (larger(len, maxLen)) {
			maxLen = len;
			longest = tv;
		}
	}
	return longest;
}

/**
 * @brief Sort a std::vector of translation vectors by length.
 * @param tvs The std::vector to sort.
 */
void sort_by_length(std::vector<TranslationVector>& tvs) {
	std::sort( tvs.begin( ), tvs.end( ), [ ]( const TranslationVector& lhs, const TranslationVector& rhs ) {
		coord_t llen = bg::length(segment_t { { 0, 0 }, lhs.vector_ });
		coord_t rlen = bg::length(segment_t { { 0, 0 }, rhs.vector_ });
		return llen < rlen;
	});
}

/**
 * @brief Sort a std::vector of translation vectors by history count.
 * @param history The history of used translation vectors.
 * @param tvs The std::vector to sort.
 */
void sort_by_history_count(const History& history, std::vector<TranslationVector>& tvs) {
	std::sort( tvs.begin( ), tvs.end( ), [&]( const TranslationVector& lhs, const TranslationVector& rhs ) {
		size_t cl = count(history, lhs);
		size_t cr = count(history, rhs);
		return cl < cr;
	});
}

/**
 * @brief Select the next translation vector from all viable translations.
 * @param nfp The NFP so far.
 * @param pA Polygon A.
 * @param rA The pertaining ring of polygon A.
 * @param rB Ring of B.
 * @param feasibleVectors All viable translations (= all translations that lead to a valid slide, including already traversed ones)
 * @param history The history of all performed translations
 * @return The translation vector used for the next traversal step (slide)
 */
TranslationVector select_next_translation_vector(const nfp_t& nfp, const polygon_t& pA, const polygon_t::ring_type& rA, const polygon_t::ring_type& rB, std::vector<TranslationVector> feasibleVectors, const History& history) {
	if(feasibleVectors.size() == 1) {
		return feasibleVectors.front();
	}

	if (history.size() > 1) {
#ifdef NFP_DEBUG
		if(history.size() > 5) {
			DEBUG_VAL("last 6 from history:");
			for (size_t i = 0; i < 6; ++i) {
				DEBUG_VAL(history[history.size() - 1 - i]);
			}
			DEBUG_VAL("");
		}
#endif

		size_t histCnt = 0;
		size_t minHistCnt = history.size() + 1;
		TranslationVector least_used;

		sort_by_history_count(history, feasibleVectors);

		for(auto& candidate : feasibleVectors) {
			point_t translated;
			TranslationVector trimmed = trim_vector(rA, rB, candidate);
			boost::geometry::transform(rB.front(), translated, trans::translate_transformer<coord_t, 2, 2>(trimmed.vector_.x_, trimmed.vector_.y_));
			if(!in_nfp(translated, nfp)) {
				DEBUG_MSG("least unused, not in nfp", candidate);
				return candidate;
			}
		}

		sort_by_length(feasibleVectors);

		for (auto& candidate : feasibleVectors) {
			if (count(history, candidate) == 0) {
				DEBUG_MSG("longest unused", candidate);
				return candidate;
			}
		}

		for (const auto& candidate : feasibleVectors) {
			histCnt = count(history, candidate);

			if(histCnt < minHistCnt) {
				minHistCnt = histCnt;
				least_used = candidate;
			}
		}

		DEBUG_MSG("longest least used", least_used);
		return least_used;

		TranslationVector tv;
		tv.vector_ = INVALID_POINT;
		return tv;
	} else {
		auto longest = find_longest(feasibleVectors);
		DEBUG_MSG("longest", longest);
		return longest;
	}
}
}


#endif /* SRC_ALGO_SELECT_NEXT_HPP_ */
