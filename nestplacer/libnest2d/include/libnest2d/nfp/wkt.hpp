#ifndef SRC_WKT_HPP_
#define SRC_WKT_HPP_

#include "geometry.hpp"

namespace libnfporb {
/**
 * @brief Read a wkt polygon from a file.
 * @param filename The name of the file to read from.
 * @param p The polygon reference to store the resulting polygon in.
 */
void read_wkt_polygon(const string& filename, polygon_t& p) {
	std::ifstream t(filename);

	std::string str;
	t.seekg(0, std::ios::end);
	str.reserve(t.tellg());
	t.seekg(0, std::ios::beg);

	str.assign((std::istreambuf_iterator<char>(t)),
			std::istreambuf_iterator<char>());

	str.pop_back();
	bg::read_wkt(str, p);
	bg::correct(p);
}
}


#endif /* SRC_WKT_HPP_ */
