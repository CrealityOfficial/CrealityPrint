#ifndef SRC_SVG_HPP_
#define SRC_SVG_HPP_


#include <iostream>
#include <string>
#include <fstream>
#include <vector>

#include <boost/geometry/io/svg/svg_mapper.hpp>

#include "geometry.hpp"

namespace libnfporb {
	/**
	 * @brief Because the boost svg mapper can't handle our custom types we have to convert polygon_t::ring_type to polygonf_t::ring_type (which is using "long double" as coordinate type).
	 * @param r The ring to convert.
	 * @return A converted copy of r.
	 */
	polygonf_t::ring_type convert(const polygon_t::ring_type& r);

	/**
	 * @brief Because the boost svg mapper can't handle our custom types we have to convert polygon_t to polygonf_t (which is using "long double" as coordinate type).
	 * @param p The polygon to convert.
	 *  * @return A converted copy of p.
	 */
	polygonf_t convert(const polygon_t& p);

	/**
	 * @brief Writes segments to a svg file.
	 * @param filename The filename to write to.
	 * @param segments The segments
	 */
	void write_svg(std::string const& filename, const std::vector<segment_t>& segments);

	/**
	 * @brief Writes a polygon (A) and a ring (B) to a svg file.
	 * @param filename The filename to write to.
	 * @param p The polygon
	 * @param ring The ring
	 */
	void write_svg(std::string const& filename, const polygon_t& p, const polygon_t::ring_type& ring);

	/**
	 * @brief Writes polygons a svg file.
	 * @param filename The filename to write to.
	 * @param polygons The polygons
	 */
	void write_svg(std::string const& filename, typename std::vector<polygon_t> const& polygons);

	/**
	 * @brief Writes pA, pB and the resulting NFP to a svg file.
	 * @param filename The filename to write to.
	 * @param pA polygon A
	 * @param pB polygon B
	 * @param nfp The resulting NFP
	 */
	void write_svg(std::string const& filename, const polygon_t& pA, const polygon_t& pB, const nfp_t& nfp);

	void write_svg(std::string const& filename, const polygon_t& pA, const polygon_t& pB = polygon_t(), const polygon_t& pC = polygon_t());
}

#endif /* SRC_SVG_HPP_ */

