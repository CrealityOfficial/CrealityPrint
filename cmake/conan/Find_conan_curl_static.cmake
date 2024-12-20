__cc_find(openssl)

set(ILIBS ssl crypto)
if(CC_BC_WIN)
	list(APPEND ILIBS Ws2_32.lib Crypt32.lib Wldap32.lib)
endif()
__conan_import(curl_static lib ILIB ${ILIBS} INTERFACE_DEF CURL_STATICLIB)
