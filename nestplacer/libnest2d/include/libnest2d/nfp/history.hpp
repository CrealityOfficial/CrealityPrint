#ifndef SRC_HISTORY_HPP_
#define SRC_HISTORY_HPP_

#include "geometry.hpp"
#include "translation_vector.hpp"

namespace libnfporb {
typedef std::vector<TranslationVector> History;

/**
 * Find a translation vector in the history using a custom predicate
 * @param h The history.
 * @param tv The translation vector.
 * @param predicate The custom predicate
 * @param offset The index offset to start searching from
 * @return returns The index of the translation vector or -1 if it isn't found.
 */
off_t find(const History& h, const TranslationVector& tv, std::function<bool(const TranslationVector&, const TranslationVector&)> predicate, const off_t& offset = 0) {
	if(offset < 0)
		return -1;

	for(size_t i = offset; i < h.size(); ++i) {
		if (predicate(h[i], tv)) {
			return i;
		}
	}

	return -1;
}

/**
 * Find a translation vector in the history by comparing segment and vector.
 * @param h The history.
 * @param tv The translation vector.
 * @param offset The index offset to start searching from
 * @return returns The index of the translation vector or -1 if it isn't found.
 */
off_t find(const History& h, const TranslationVector& tv, const off_t& offset = 0) {
	if(offset < 0)
		return -1;

	for(size_t i = offset; i < h.size(); ++i) {
		if (find(h, tv, [](const TranslationVector& lhs, const TranslationVector& rhs){ return lhs == rhs; }, offset)) {
			return i;
		}
	}

	return -1;
}

/**
 * Count the occurences of a translation vector in the history with a custom predicate.
 * @param h The history.
 * @param tv The translation vector.
 * @return The count of occurrences in the history.
 */size_t count(const History& h, const TranslationVector& tv, std::function<bool(const TranslationVector&, const TranslationVector&)> predicate) {
	size_t cnt = 0;
	off_t offset = 0;
	while((offset = find(h, tv, predicate, offset + 1)) != -1)
		++cnt;

	return cnt;
}

/**
 * Count the occurences of a translation vector in the history.
 * @param h The history.
 * @param tv The translation vector.
 * @return The count of occurrences in the history.
 */size_t count(const History& h, const TranslationVector& tv) {
	return count(h,tv, [](const TranslationVector& lhs, const TranslationVector& rhs){ return lhs == rhs; });
}




}

#endif /* SRC_HISTORY_HPP_ */
