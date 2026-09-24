#include "Hasher.hpp"

#include <algorithm>
#include <array>
#include <iomanip>
#include <locale>
#include <sstream>
#include <utility>
#include <vector>

namespace Slic3r::Diagnostics {
namespace {

using Coordinate = std::pair<coord_t, coord_t>;

std::size_t minimal_rotation(const std::vector<Coordinate> &points)
{
    const std::size_t size = points.size();
    if (size < 2)
        return 0;

    std::size_t lhs = 0;
    std::size_t rhs = 1;
    std::size_t offset = 0;
    while (lhs < size && rhs < size && offset < size) {
        const Coordinate &a = points[(lhs + offset) % size];
        const Coordinate &b = points[(rhs + offset) % size];
        if (a == b) {
            ++offset;
            continue;
        }
        if (a > b) {
            lhs += offset + 1;
            if (lhs == rhs)
                ++lhs;
        } else {
            rhs += offset + 1;
            if (lhs == rhs)
                ++rhs;
        }
        offset = 0;
    }
    return std::min(lhs, rhs) % size;
}

std::vector<Coordinate> normalized_ring(const Polygon &polygon)
{
    std::vector<Coordinate> forward;
    forward.reserve(polygon.points.size());
    for (const Point &point : polygon.points)
        forward.emplace_back(point.x(), point.y());

    if (forward.size() > 1 && forward.front() == forward.back())
        forward.pop_back();
    if (forward.empty())
        return forward;

    std::vector<Coordinate> reverse(forward.rbegin(), forward.rend());
    const std::size_t forward_begin = minimal_rotation(forward);
    const std::size_t reverse_begin = minimal_rotation(reverse);

    auto rotation_less = [](const std::vector<Coordinate> &lhs, std::size_t lhs_begin,
                            const std::vector<Coordinate> &rhs, std::size_t rhs_begin) {
        for (std::size_t offset = 0; offset < lhs.size(); ++offset) {
            const Coordinate &a = lhs[(lhs_begin + offset) % lhs.size()];
            const Coordinate &b = rhs[(rhs_begin + offset) % rhs.size()];
            if (a != b)
                return a < b;
        }
        return false;
    };

    const bool use_reverse = rotation_less(reverse, reverse_begin, forward, forward_begin);
    const std::vector<Coordinate> &source = use_reverse ? reverse : forward;
    const std::size_t begin = use_reverse ? reverse_begin : forward_begin;

    std::vector<Coordinate> normalized;
    normalized.reserve(source.size());
    for (std::size_t offset = 0; offset < source.size(); ++offset)
        normalized.emplace_back(source[(begin + offset) % source.size()]);
    return normalized;
}

} // namespace

void Hasher::mix_byte(unsigned char value)
{
    m_hash_high ^= value;
    m_hash_high *= 1099511628211ull;
    m_hash_low ^= static_cast<std::uint64_t>(value) + 0x9e3779b97f4a7c15ull +
                  (m_hash_low << 6u) + (m_hash_low >> 2u);
}

void Hasher::add_bytes(const void *data, std::size_t size)
{
    add_size(size);
    const auto *bytes = static_cast<const unsigned char *>(data);
    for (std::size_t index = 0; index < size; ++index)
        mix_byte(bytes[index]);
}

void Hasher::add_size(std::size_t value)
{
    std::array<unsigned char, sizeof(std::uint64_t)> bytes{};
    std::uint64_t normalized = static_cast<std::uint64_t>(value);
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[bytes.size() - index - 1] = static_cast<unsigned char>(normalized & 0xffu);
        normalized >>= 8u;
    }
    for (unsigned char byte : bytes)
        mix_byte(byte);
}

void Hasher::add_string(std::string_view value)
{
    add_bytes(value.data(), value.size());
}

void Hasher::add_integer(std::int64_t value)
{
    add_string(std::to_string(value));
}

void Hasher::add_unsigned(std::uint64_t value)
{
    add_string(std::to_string(value));
}

void Hasher::add_boolean(bool value)
{
    add_string(value ? "true" : "false");
}

void Hasher::add_floating_point(double value)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::setprecision(17) << value;
    add_string(stream.str());
}

void Hasher::add_point(const Point &point)
{
    add_integer(point.x());
    add_integer(point.y());
}

void Hasher::add_polyline(const Polyline &polyline)
{
    add_unsigned(polyline.points.size());
    for (const Point &point : polyline.points)
        add_point(point);
}

void Hasher::add_polygon(const Polygon &polygon)
{
    const std::vector<Coordinate> points = normalized_ring(polygon);
    add_unsigned(points.size());
    for (const Coordinate &point : points) {
        add_integer(point.first);
        add_integer(point.second);
    }
}

void Hasher::add_expolygon(const ExPolygon &expolygon)
{
    Hasher contour_hasher;
    contour_hasher.add_polygon(expolygon.contour);
    add_string(contour_hasher.digest());

    std::vector<std::string> holes;
    holes.reserve(expolygon.holes.size());
    for (const Polygon &hole : expolygon.holes) {
        Hasher hole_hasher;
        hole_hasher.add_polygon(hole);
        holes.emplace_back(hole_hasher.digest());
    }
    std::sort(holes.begin(), holes.end());
    add_unsigned(holes.size());
    for (const std::string &hole : holes)
        add_string(hole);
}

void Hasher::add_expolygons(const ExPolygons &expolygons)
{
    std::vector<std::string> fingerprints;
    fingerprints.reserve(expolygons.size());
    for (const ExPolygon &expolygon : expolygons) {
        Hasher item;
        item.add_expolygon(expolygon);
        fingerprints.emplace_back(item.digest());
    }
    std::sort(fingerprints.begin(), fingerprints.end());

    add_unsigned(fingerprints.size());
    for (const std::string &fingerprint : fingerprints)
        add_string(fingerprint);
}

std::string Hasher::digest() const
{
    std::ostringstream stream;
    stream << std::hex << std::setfill('0')
           << std::setw(16) << m_hash_high
           << std::setw(16) << m_hash_low;
    return stream.str();
}

} // namespace Slic3r::Diagnostics
