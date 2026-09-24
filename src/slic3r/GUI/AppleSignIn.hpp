#ifndef SLIC3R_GUI_APPLE_SIGN_IN_HPP
#define SLIC3R_GUI_APPLE_SIGN_IN_HPP

#include <functional>
#include <string>

namespace Slic3r {
namespace GUI {

struct AppleSignInResult {
    bool success { false };
    std::string authorization_code;
    std::string identity_token;
    // Raw nonce used when starting the ASAuthorization request. Firebase needs
    // it to verify the identity token signature (Apple returns sha256(nonce)).
    std::string nonce;
    std::string user;
    std::string email;
    std::string given_name;
    std::string family_name;
    std::string error;
};

using AppleSignInCallback = std::function<void(AppleSignInResult)>;

// True on macOS 10.15+ where AuthenticationServices is available.
bool apple_sign_in_available();

// Runs the native Sign in with Apple sheet and reports the credentials on the
// callback (from an arbitrary internal queue).
void start_apple_sign_in(AppleSignInCallback callback);

} // namespace GUI
} // namespace Slic3r

#endif // SLIC3R_GUI_APPLE_SIGN_IN_HPP
