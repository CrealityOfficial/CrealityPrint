#ifndef SRC_TRANSLATION_VECTOR_HPP_
#define SRC_TRANSLATION_VECTOR_HPP_

#include "geometry.hpp"

namespace libnfporb {
/**
 * TranslationVector is the struct used to store information about translations (including the segment it was derived from)
 */
struct TranslationVector {
	point_t vector_;
	segment_t edge_;
	bool fromA_ = false;
	string name_;

	bool operator<(const TranslationVector& other) const {
		return this->vector_ < other.vector_ || (equals(this->vector_, other.vector_) && (this->edge_ < other.edge_));
	}
};

std::ostream& operator<<(std::ostream& os, const TranslationVector& tv) {
	os << "{" << tv.edge_ << " -> " << tv.vector_ << "} = " << tv.name_;
	return os;
}

bool operator==(const TranslationVector& lhs, const TranslationVector& rhs) {
	return equals(lhs.vector_, rhs.vector_) && equals(lhs.edge_, rhs.edge_);
}

bool operator!=(const TranslationVector& lhs, const TranslationVector& rhs) {
	return !operator==(lhs,rhs);
}
}

#endif /* SRC_TRANSLATION_VECTOR_HPP_ */
