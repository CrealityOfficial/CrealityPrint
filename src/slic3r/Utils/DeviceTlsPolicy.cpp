#include "DeviceTlsPolicy.hpp"

#include <openssl/x509_vfy.h>

#include <curl/curl.h>

#include <cstring>

namespace Slic3r {
namespace DeviceTlsPolicy {

namespace {

bool apply_no_check_time(X509_VERIFY_PARAM* param, std::string& error)
{
    if (param == nullptr) {
        error = "TLS verification parameters are unavailable";
        return false;
    }

    if (X509_VERIFY_PARAM_set_flags(param, X509_V_FLAG_NO_CHECK_TIME) != 1) {
        error = "Failed to disable certificate time verification";
        return false;
    }

    return true;
}

} // namespace

bool ignore_certificate_time(SSL* ssl, std::string& error)
{
    if (ssl == nullptr) {
        error = "Invalid SSL handle";
        return false;
    }

    return apply_no_check_time(SSL_get0_param(ssl), error);
}

bool ignore_certificate_time(SSL_CTX* context, std::string& error)
{
    if (context == nullptr) {
        error = "Invalid SSL context";
        return false;
    }

    return apply_no_check_time(SSL_CTX_get0_param(context), error);
}

bool ignore_certificate_time_from_curl(void* ssl_context, std::string& error)
{
    const curl_version_info_data* version = curl_version_info(CURLVERSION_NOW);
    if (version == nullptr || version->ssl_version == nullptr ||
        std::strncmp(version->ssl_version, "OpenSSL/", 8) != 0) {
        error = "The libcurl TLS backend is not OpenSSL";
        return false;
    }

    return ignore_certificate_time(static_cast<SSL_CTX*>(ssl_context), error);
}

} // namespace DeviceTlsPolicy
} // namespace Slic3r
