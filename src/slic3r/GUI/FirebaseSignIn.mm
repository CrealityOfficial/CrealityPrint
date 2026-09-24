// FIRApp / FIRAuth live in the plain headers, but FIROAuthProvider and the
// credential factories are Swift-generated and only declared in FirebaseAuth-Swift.h
// (which itself references the FIRAuthInterop protocol from the interop framework).
#import <FirebaseAuthInterop/FIRAuthInterop.h>
#import <FirebaseAuth/FirebaseAuth.h>
#import <FirebaseAuth/FirebaseAuth-Swift.h>
#import <Foundation/Foundation.h>

#include "FirebaseSignIn.hpp"

// FirebaseAuth ships as static archives: the FIRAuthComponent class (which
// registers the Auth component with FIRApp.configure) is never referenced by
// the code we call, so the linker drops its object file and Auth.auth() traps
// on a failed "as! Auth" cast. A linker-level -u flag cannot be used because
// the "$" in the mangled ObjC symbol gets eaten by the build system; this
// extern reference forces the archive member in portably instead.
// On macOS the compiler prepends one underscore to C identifiers, so the
// ObjC class symbol "_OBJC_CLASS_$_FIRAuthComponent" is spelled without the
// leading underscore here.
extern "C" {
extern char OBJC_CLASS_$_FIRAuthComponent[];
}
__attribute__((used)) static void *const kForceFIRAuthComponentLink[] = {
    (void *) &OBJC_CLASS_$_FIRAuthComponent[0],
};

namespace Slic3r {
namespace GUI {

// Unbuffered logging for the crash investigation: NSLog survives crashes via
// the unified log, unlike the buffered boost file log.
extern "C" void cp_apple_debug_log(const char *message)
{
    NSLog(@"CrealityPrintApple: %s", message);
}

bool firebase_auth_available()
{
    @try {
        if ([FIRApp defaultApp] == nil) {
            if ([[NSBundle mainBundle] pathForResource:@"GoogleService-Info" ofType:@"plist"] == nil)
                return false;
            [FIRApp configure];
        }
        return [FIRApp defaultApp] != nil;
    } @catch (NSException *exception) {
        NSLog(@"CrealityPrint FIRApp configure failed: %@ %@", exception.name, exception.reason);
        return false;
    }
}

void firebase_sign_in_with_apple(const std::string &apple_identity_token,
                                 const std::string &apple_raw_nonce,
                                 const std::string &apple_authorization_code,
                                 FirebaseSignInCallback callback)
{
    cp_apple_debug_log("firebase_sign_in_with_apple: enter");
    if (!firebase_auth_available()) {
        cp_apple_debug_log("firebase_sign_in_with_apple: firebase not configured");
        if (callback)
            callback({false, {}, {}, "Firebase is not configured (GoogleService-Info.plist missing)"});
        return;
    }
    cp_apple_debug_log("firebase_sign_in_with_apple: configured, checking token");
    if (apple_identity_token.empty()) {
        if (callback)
            callback({false, {}, {}, "Apple identity token is empty"});
        return;
    }

    NSString *idToken = [NSString stringWithUTF8String:apple_identity_token.c_str()];
    NSString *rawNonce = [NSString stringWithUTF8String:apple_raw_nonce.c_str()];
    NSString *accessToken = apple_authorization_code.empty() ? nil : [NSString stringWithUTF8String:apple_authorization_code.c_str()];

    FIROAuthCredential *credential = nil;
    if (accessToken != nil) {
        credential = [FIROAuthProvider credentialWithProviderID:@"apple.com"
                                                         IDToken:idToken
                                                        rawNonce:rawNonce
                                                     accessToken:accessToken];
    } else {
        credential = [FIROAuthProvider credentialWithProviderID:@"apple.com"
                                                         IDToken:idToken
                                                        rawNonce:rawNonce];
    }
    cp_apple_debug_log("firebase_sign_in_with_apple: credential created, calling signIn");

    NSLog(@"CrealityPrint Firebase Apple sign-in: id_token_len=%lu nonce_present=%d code_len=%lu",
          (unsigned long) idToken.length, rawNonce != nil, (unsigned long) (accessToken ? accessToken.length : 0));

    [[FIRAuth auth] signInWithCredential:credential
                              completion:^(FIRAuthDataResult *result, NSError *error) {
        cp_apple_debug_log("firebase_sign_in_with_apple: signIn callback entered");
        if (error != nil || result == nil) {
            NSString *domain = error.domain ?: @"unknown";
            NSString *description = error.localizedDescription ?: @"Firebase returned no result";
            const long code = error ? (long) error.code : 0;
            NSLog(@"CrealityPrint Firebase Apple sign-in failed: domain=%@ code=%ld description=%@",
                  domain, code, description);
            std::string message = "Firebase sign-in failed (";
            message += domain.UTF8String;
            message += " " + std::to_string(code) + "): ";
            message += description.UTF8String;
            if (callback)
                callback({false, {}, {}, message});
            return;
        }
        NSString *uid = result.user.uid ?: @"";
        cp_apple_debug_log("firebase_sign_in_with_apple: signIn ok, getting ID token");
        [result.user getIDTokenWithCompletion:^(NSString *token, NSError *tokenError) {
            if (tokenError != nil || token.length == 0) {
                NSString *description = tokenError.localizedDescription ?: @"Firebase returned an empty ID token";
                NSLog(@"CrealityPrint Firebase ID token failed: %@", description);
                std::string message = "Firebase ID token error: ";
                message += description.UTF8String;
                if (callback)
                    callback({false, {}, uid.UTF8String ? std::string(uid.UTF8String) : "", message});
                return;
            }
            NSLog(@"CrealityPrint Firebase Apple sign-in ok: uid_len=%lu id_token_len=%lu",
                  (unsigned long) uid.length, (unsigned long) token.length);
            if (callback)
                callback({true, std::string(token.UTF8String), std::string(uid.UTF8String), {}});
        }];
    }];
}

} // namespace GUI
} // namespace Slic3r
