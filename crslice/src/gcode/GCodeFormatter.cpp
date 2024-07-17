#include "GCodeFormatter.h"

#ifdef __APPLE__
#include <boost/spirit/include/karma.hpp>
#endif

namespace Slic3r
{
    void GCodeFormatter::emit_axis(const char axis, const double v, size_t digits) {
        assert(digits <= 9);
        static constexpr const std::array<int, 10> pow_10{ 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };
        *ptr_err.ptr++ = ' '; *ptr_err.ptr++ = axis;

        char* base_ptr = this->ptr_err.ptr;
        auto  v_int = int64_t(std::round(v * pow_10[digits]));
        // Older stdlib on macOS doesn't support std::from_chars at all, so it is used boost::spirit::karma::generate instead of it.
        // That is a little bit slower than std::to_chars but not much.
#ifdef __APPLE__
        boost::spirit::karma::generate(this->ptr_err.ptr, boost::spirit::karma::int_generator<int64_t>(), v_int);
#else
    // this->buf_end minus 1 because we need space for adding the extra decimal point.
        this->ptr_err = std::to_chars(this->ptr_err.ptr, this->buf_end - 1, v_int);
#endif
        size_t writen_digits = (this->ptr_err.ptr - base_ptr) - (v_int < 0 ? 1 : 0);
        if (writen_digits < digits) {
            // Number is smaller than 10^digits, so that we will pad it with zeros.
            size_t remaining_digits = digits - writen_digits;
            // Move all newly inserted chars by remaining_digits to allocate space for padding with zeros.
            for (char* from_ptr = this->ptr_err.ptr - 1, *to_ptr = from_ptr + remaining_digits; from_ptr >= this->ptr_err.ptr - writen_digits; --to_ptr, --from_ptr)
                *to_ptr = *from_ptr;

            memset(this->ptr_err.ptr - writen_digits, '0', remaining_digits);
            this->ptr_err.ptr += remaining_digits;
        }

        // Move all newly inserted chars by one to allocate space for a decimal point.
        for (char* to_ptr = this->ptr_err.ptr, *from_ptr = to_ptr - 1; from_ptr >= this->ptr_err.ptr - digits; --to_ptr, --from_ptr)
            *to_ptr = *from_ptr;

        *(this->ptr_err.ptr - digits) = '.';
        for (size_t i = 0; i < digits; ++i) {
            if (*this->ptr_err.ptr != '0')
                break;
            this->ptr_err.ptr--;
        }
        if (*this->ptr_err.ptr == '.')
            this->ptr_err.ptr--;
        if ((this->ptr_err.ptr + 1) == base_ptr || *this->ptr_err.ptr == '-')
            *(++this->ptr_err.ptr) = '0';
        this->ptr_err.ptr++;

#if 0 // #ifndef NDEBUG
        {
            // Verify that the optimized formatter produces the same result as the standard sprintf().
            double v1 = atof(std::string(base_ptr, this->ptr_err.ptr).c_str());
            char buf[2048];
            sprintf(buf, "%.*lf", int(digits), v);
            double v2 = atof(buf);
            // Numbers may differ when rounding at exactly or very close to 0.5 due to numerical issues when scaling the double to an integer.
            // Thus the complex assert.
    //        assert(v1 == v2);
            assert(std::abs(v1 - v) * pow_10[digits] < 0.50001);
            assert(std::abs(v2 - v) * pow_10[digits] < 0.50001);
        }
#endif // NDEBUG
    }
}
