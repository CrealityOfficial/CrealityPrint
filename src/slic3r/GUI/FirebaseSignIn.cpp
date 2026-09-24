#include "FirebaseSignIn.hpp"

// Non-Apple platforms keep the build green; the Apple login entry point is
// only created on macOS anyway.
#if !defined(__APPLE__)

namespace Slic3r {
namespace GUI {

bool firebase_auth_available() { return false; }

void firebase_sign_in_with_apple(const std::string &apple_identity_token,
                                 const std::string &apple_raw_nonce,
                                 const std::string &apple_authorization_code,
                                 FirebaseSignInCallback callback)
{
    if (callback)
        callback({false, {}, {}, "Firebase Apple sign in is only available on macOS"});
}

} // namespace GUI
} // namespace Slic3r

#endif // !defined(__APPLE__)
