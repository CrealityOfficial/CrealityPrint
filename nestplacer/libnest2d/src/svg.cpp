#include "libnest2d/nfp/svg.hpp"

namespace libnfporb {
	/**
	 * @brief Because the boost svg mapper can't handle our custom types we have to convert polygon_t::ring_type to polygonf_t::ring_type (which is using "long double" as coordinate type).
	 * @param r The ring to convert.
	 * @return A converted copy of r.
	 */
	polygonf_t::ring_type convert(const polygon_t::ring_type& r) {
		polygonf_t::ring_type rf;
		for (const auto& pt : r) {
			rf.push_back(pointf_t(toLongDouble(pt.x_), toLongDouble(pt.y_)));
		}
		return rf;
	}

	/**
	 * @brief Because the boost svg mapper can't handle our custom types we have to convert polygon_t to polygonf_t (which is using "long double" as coordinate type).
	 * @param p The polygon to convert.
	 *  * @return A converted copy of p.
	 */
	polygonf_t convert(const polygon_t& p) {
		polygonf_t pf;
		pf.outer() = convert(p.outer());

		for (const auto& r : p.inners()) {
			pf.inners().push_back(convert(r));
		}

		return pf;
	}

	/**
	 * @brief Writes segments to a svg file.
	 * @param filename The filename to write to.
	 * @param segments The segments
	 */
	void write_svg(std::string const& filename, const std::vector<segment_t>& segments) {
		std::ofstream svg(filename.c_str());

		boost::geometry::svg_mapper<pointf_t> mapper(svg, 100, 100, "width=\"200mm\" height=\"200mm\" viewBox=\"-250 -250 500 500\" style=\"background-color:white\"");
		for (const auto& seg : segments) {
			segmentf_t segf({ toLongDouble(seg.first.x_), toLongDouble(seg.first.y_) }, { toLongDouble(seg.second.x_), toLongDouble(seg.second.y_) });
			mapper.add(segf);
			mapper.map(segf, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:0.25");
		}
	}

	/**
	 * @brief Writes a polygon (A) and a ring (B) to a svg file.
	 * @param filename The filename to write to.
	 * @param p The polygon
	 * @param ring The ring
	 */
	void write_svg(std::string const& filename, const polygon_t& p, const polygon_t::ring_type& ring) {
		std::ofstream svg(filename.c_str());

		boost::geometry::svg_mapper<pointf_t> mapper(svg, 100, 100, "width=\"200mm\" height=\"200mm\" viewBox=\"-250 -250 500 500\" style=\"background-color:white\"");
		auto pf = convert(p);
		auto rf = convert(ring);

		mapper.add(pf);
		mapper.map(pf, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:0");
		mapper.add(rf);
		mapper.map(rf, "fill-opacity:0.5;fill:rgb(204,153,0);stroke:rgb(153,204,0);stroke-width:0");
	}

	/**
	 * @brief Writes polygons a svg file.
	 * @param filename The filename to write to.
	 * @param polygons The polygons
	 */
	void write_svg(std::string const& filename, typename std::vector<polygon_t> const& polygons) {
		std::ofstream svg(filename.c_str());

		boost::geometry::svg_mapper<pointf_t> mapper(svg, 100, 100, "width=\"200mm\" height=\"200mm\" viewBox=\"-250 -250 500 500\" style=\"background-color:white\"");
		for (auto p : polygons) {
			auto pf = convert(p);
			mapper.add(pf);
			mapper.map(pf, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:0.25");
		}
	}

	/**
	 * @brief Writes pA, pB and the resulting NFP to a svg file.
	 * @param filename The filename to write to.
	 * @param pA polygon A
	 * @param pB polygon B
	 * @param nfp The resulting NFP
	 */
	void write_svg(std::string const& filename, const polygon_t& pA, const polygon_t& pB, const nfp_t& nfp) {
		polygon_t nfppoly;
		for (const auto& pt : nfp.front()) {
			nfppoly.outer().push_back(pt);
		}

		for (size_t i = 1; i < nfp.size(); ++i) {
			nfppoly.inners().push_back({ });
			for (const auto& pt : nfp[i]) {
				nfppoly.inners().back().push_back(pt);
			}
		}
		std::ofstream svg(filename.c_str());

		boost::geometry::svg_mapper<pointf_t> mapper(svg, 100, 100, "width=\"200mm\" height=\"200mm\" viewBox=\"-250 -250 500 500\" style=\"background-color:white\"");
		auto pAf = convert(pA);
		mapper.add(pAf);
		mapper.map(pAf, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:0");

		auto pBf = convert(pB);
		mapper.add(pBf);
		mapper.map(pBf, "fill-opacity:0.5;fill:rgb(204,153,0);stroke:rgb(204,153,0);stroke-width:0");

		bg::correct(nfppoly);
		auto nfpf = convert(nfppoly);
		mapper.add(nfpf);
		mapper.map(nfpf, "fill-opacity:0.5;fill:rgb(0,153,204);stroke:rgb(0,38,50);stroke-width:0.5");

		for (auto& r : nfpf.inners()) {
			if (r.size() == 1) {
				mapper.add(r.front());
				mapper.map(r.front(), "fill-opacity:0.5;fill:rgb(0,153,204);stroke:rgb(0,38,50);stroke-width:0.5");
			}
			else if (r.size() == 2) {
				segmentf_t seg(r.front(), *(r.begin() + 1));
				mapper.add(seg);
				mapper.map(seg, "fill-opacity:0.5;fill:rgb(0,153,204);stroke:rgb(0,38,50);stroke-width:0.5");
			}
		}
	}

	void write_svg(std::string const& filename, const polygon_t& pA, const polygon_t& pB, const polygon_t& pC) {

		std::ofstream svg(filename.c_str());

		boost::geometry::svg_mapper<pointf_t> mapper(svg, 100, 100, "width=\"200mm\" height=\"200mm\" viewBox=\"-250 -250 500 500\" style=\"background-color:white\"");

		auto pAf = convert(pA);
		mapper.add(pAf);
		mapper.map(pAf, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:0");

		auto pBf = convert(pB);
		mapper.add(pBf);
		mapper.map(pBf, "fill-opacity:0.5;fill:rgb(204,153,0);stroke:rgb(204,153,0);stroke-width:0");

		auto pCf = convert(pC);
		mapper.add(pCf);
		mapper.map(pCf, "fill-opacity:0.5;fill:rgb(0,153,204);stroke:rgb(0,153,204);stroke-width:0");
	}
}

