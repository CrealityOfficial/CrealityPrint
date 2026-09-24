#ifndef slic3r_DeviceTlsPolicy_hpp_
#define slic3r_DeviceTlsPolicy_hpp_

#include <openssl/ssl.h>

#include <string>

namespace Slic3r {
namespace DeviceTlsPolicy {

// Ignore certificate validity dates while retaining chain and signature checks.
bool ignore_certificate_time(SSL* ssl, std::string& error);
bool ignore_certificate_time(SSL_CTX* context, std::string& error);

// The curl SSL context callback receives a backend-specific opaque pointer.
// This helper is only valid for the OpenSSL curl backend used by the project.
bool ignore_certificate_time_from_curl(void* ssl_context, std::string& error);

} // namespace DeviceTlsPolicy
} // namespace Slic3r

#endif /* slic3r_DeviceTlsPolicy_hpp_ */
