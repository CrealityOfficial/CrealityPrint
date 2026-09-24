#ifndef KLSIM_TEST_BOOST_LOG_CORE_HPP
#define KLSIM_TEST_BOOST_LOG_CORE_HPP

namespace boost { namespace log {
class core
{
public:
    static core* get() { return nullptr; }
    void flush() {}
};
}}

#endif
