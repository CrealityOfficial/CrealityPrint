#ifndef slic3r_Diagnostics_Hasher_hpp_
#define slic3r_Diagnostics_Hasher_hpp_

#include "../ExPolygon.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace Slic3r::Diagnostics {

class Hasher
{
public:
    void add_bytes(const void *data, std::size_t size);
    void add_string(std::string_view value);
    void add_integer(std::int64_t value);
    void add_unsigned(std::uint64_t value);
    void add_boolean(bool value);
    void add_floating_point(double value);
    void add_point(const Point &point);
    void add_polyline(const Polyline &polyline);
    void add_polygon(const Polygon &polygon);
    void add_expolygon(const ExPolygon &expolygon);
    void add_expolygons(const ExPolygons &expolygons);
    std::string digest() const;

private:
    void add_size(std::size_t value);
    void mix_byte(unsigned char value);

    std::uint64_t m_hash_high {14695981039346656037ull};
    std::uint64_t m_hash_low {1099511628211ull};
};

} // namespace Slic3r::Diagnostics

#endif
