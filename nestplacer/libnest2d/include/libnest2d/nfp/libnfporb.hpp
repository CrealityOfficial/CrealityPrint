#ifndef NFP_HPP_
#define NFP_HPP_

#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include <limits>

#include <boost/geometry/algorithms/intersects.hpp>

#include "geometry.hpp"

namespace libnfporb {

	/**
	 * Delete oscillations and loops from the given ring.
	 * @param ring A reference to the ring to be shortened.
	 * @return true if the ring has changed.
	 */
	bool delete_consecutive_repeating_point_patterns(polygon_t::ring_type& ring);

	/**
	 * Remove co-linear points from a ring.
	 * @param r A reference to a ring.
	 */
	void remove_co_linear(polygon_t::ring_type& r);
	/**
	 * Remove co-linear points from a polygon.
	 * @param p A reference to a polygon.
	 */
	void remove_co_linear(polygon_t& p);

	/**
	 * Generate the NFP for the given polygons pA and pB. Optionally check the input polygons for validity.
	 * @param pA polygon A (the stationary polygon).
	 * @param pB polygon B (the orbiting polygon).
	 *  * @param checkValidity Check the input polygons for validity if true. Defaults to true.
	 * @return The generated NFP.
	 */
	nfp_t generate_nfp(polygon_t& pA, polygon_t& pB, const bool checkValidity = true);
}
#endif
