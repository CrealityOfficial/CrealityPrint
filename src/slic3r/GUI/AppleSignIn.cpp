#include "AppleSignIn.hpp"

namespace Slic3r {
namespace GUI {

#if !defined(__APPLE__)
bool apple_sign_in_available()
{
    return false;
}

void start_apple_sign_in(AppleSignInCallback callback)
{
    if (callback)
        callback({false, {}, {}, {}, {}, {}, {}, "Apple Sign in is only available on macOS"});
}
#endif

} // namespace GUI
} // namespace Slic3r
