#ifndef SLIC3R_GUI_FIREBASE_SIGN_IN_HPP
#define SLIC3R_GUI_FIREBASE_SIGN_IN_HPP

#include <functional>
#include <string>

namespace Slic3r {
namespace GUI {

struct FirebaseSignInResult {
    bool success { false };
    // Firebase ID token: what loginV2 (type 23) expects in "accessToken",
    // exactly like the Creality Cloud web login does.
    std::string firebase_id_token;
    std::string firebase_uid;
    std::string error;
};

using FirebaseSignInCallback = std::function<void(FirebaseSignInResult)>;

// True when GoogleService-Info.plist is bundled and FIRApp could be configured
// against the Creality Cloud Firebase project.
bool firebase_auth_available();

// Exchanges the native Sign in with Apple credentials for a Firebase session
// and reports the Firebase ID token on the callback (main thread is not
// guaranteed).
void firebase_sign_in_with_apple(const std::string &apple_identity_token,
                                 const std::string &apple_raw_nonce,
                                 const std::string &apple_authorization_code,
                                 FirebaseSignInCallback callback);

} // namespace GUI
} // namespace Slic3r

#endif // SLIC3R_GUI_FIREBASE_SIGN_IN_HPP
