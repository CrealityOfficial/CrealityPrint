#import <AuthenticationServices/AuthenticationServices.h>
#import <Cocoa/Cocoa.h>
#import <CommonCrypto/CommonDigest.h>
#import <Foundation/Foundation.h>
#import <objc/runtime.h>
#import <Security/Security.h>

#include "AppleSignIn.hpp"

namespace {
std::string random_hex_nonce()
{
    uint8_t bytes[32];
    if (SecRandomCopyBytes(kSecRandomDefault, sizeof(bytes), bytes) != errSecSuccess)
        return std::string();
    static const char hex[] = "0123456789abcdef";
    std::string out;
    out.reserve(sizeof(bytes) * 2);
    for (uint8_t b : bytes) {
        out.push_back(hex[b >> 4]);
        out.push_back(hex[b & 0x0f]);
    }
    return out;
}

std::string sha256_hex(const std::string &input)
{
    uint8_t digest[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256(input.data(), (CC_LONG) input.size(), digest);
    static const char hex[] = "0123456789abcdef";
    std::string out;
    out.reserve(sizeof(digest) * 2);
    for (uint8_t b : digest) {
        out.push_back(hex[b >> 4]);
        out.push_back(hex[b & 0x0f]);
    }
    return out;
}
} // namespace

@interface Slic3rAppleSignInDelegate : NSObject <ASAuthorizationControllerDelegate, ASAuthorizationControllerPresentationContextProviding>
- (instancetype)initWithCallback:(Slic3r::GUI::AppleSignInCallback)callback nonce:(std::string)nonce;
@end

@implementation Slic3rAppleSignInDelegate {
    Slic3r::GUI::AppleSignInCallback _callback;
    std::string _nonce;
}

- (instancetype)initWithCallback:(Slic3r::GUI::AppleSignInCallback)callback nonce:(std::string)nonce
{
    self = [super init];
    if (self) {
        _callback = std::move(callback);
        _nonce = std::move(nonce);
    }
    return self;
}

- (void)authorizationController:(ASAuthorizationController *)controller
 didCompleteWithAuthorization:(ASAuthorization *)authorization
{
    NSLog(@"CrealityPrintApple: ASAuthorization didCompleteWithAuthorization enter");
    Slic3r::GUI::AppleSignInResult result;
    result.nonce = _nonce;
    ASAuthorizationAppleIDCredential *credential = nil;
    if ([authorization.credential isKindOfClass:[ASAuthorizationAppleIDCredential class]])
        credential = (ASAuthorizationAppleIDCredential *)authorization.credential;

    if (!credential) {
        result.error = "Apple returned an unsupported credential";
    } else {
        result.success = true;
        result.user = credential.user ? [credential.user UTF8String] : "";
        if (credential.authorizationCode)
            result.authorization_code = [[[NSString alloc] initWithData:credential.authorizationCode encoding:NSUTF8StringEncoding] UTF8String];
        if (credential.identityToken)
            result.identity_token = [[[NSString alloc] initWithData:credential.identityToken encoding:NSUTF8StringEncoding] UTF8String];
        result.email = credential.email ? [credential.email UTF8String] : "";
        result.given_name = credential.fullName.givenName ? [credential.fullName.givenName UTF8String] : "";
        result.family_name = credential.fullName.familyName ? [credential.fullName.familyName UTF8String] : "";
        if (result.identity_token.empty()) {
            result.success = false;
            result.error = "Apple did not return an identity token";
        }
    }
    if (_callback)
        _callback(std::move(result));
}

- (void)authorizationController:(ASAuthorizationController *)controller
 didCompleteWithError:(NSError *)error
{
    Slic3r::GUI::AppleSignInResult result;
    result.error = error ? [[error localizedDescription] UTF8String] : "Apple authorization failed";
    NSLog(@"CrealityPrint Apple Sign in failed: domain=%@ code=%ld description=%@ userInfo=%@",
          error.domain, (long) error.code, error.localizedDescription, error.userInfo);
    if (_callback)
        _callback(std::move(result));
}

- (ASPresentationAnchor)presentationAnchorForAuthorizationController:(ASAuthorizationController *)controller
{
    return NSApp.keyWindow ?: NSApp.mainWindow;
}
@end

namespace {
static char kAppleSignInDelegateKey;
}

namespace Slic3r {
namespace GUI {

bool apple_sign_in_available()
{
    if (@available(macOS 10.15, *))
        return true;
    return false;
}

void start_apple_sign_in(AppleSignInCallback callback)
{
    if (!callback)
        return;
    if (!apple_sign_in_available()) {
        callback({false, {}, {}, {}, {}, {}, {}, "Sign in with Apple requires macOS 10.15 or later"});
        return;
    }

    // Firebase verifies the identity token against sha256(nonce); Apple only
    // echoes the hash, so the raw value must be kept for the Firebase exchange.
    std::string nonce = random_hex_nonce();
    if (nonce.empty()) {
        callback({false, {}, {}, {}, {}, {}, {}, "Failed to generate login nonce"});
        return;
    }

    Slic3rAppleSignInDelegate *delegate = [[Slic3rAppleSignInDelegate alloc] initWithCallback:std::move(callback) nonce:nonce];
    ASAuthorizationAppleIDProvider *provider = [ASAuthorizationAppleIDProvider new];
    ASAuthorizationAppleIDRequest *request = [provider createRequest];
    request.requestedScopes = @[ASAuthorizationScopeEmail, ASAuthorizationScopeFullName];
    request.nonce = [NSString stringWithUTF8String:sha256_hex(nonce).c_str()];

    ASAuthorizationController *controller = [[ASAuthorizationController alloc] initWithAuthorizationRequests:@[request]];
    controller.delegate = delegate;
    controller.presentationContextProvider = delegate;
    objc_setAssociatedObject(controller, &kAppleSignInDelegateKey, delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [controller performRequests];
}

} // namespace GUI
} // namespace Slic3r
