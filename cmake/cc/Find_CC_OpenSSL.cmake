# This sets the following variables:
# ssl_INCLUDE_DIRS
# ssl_LIBRARIES_DEBUG ssl_LIBRARIES_RELEASE
# ssl target
# crypto_INCLUDE_DIRS
# crypto_LIBRARIES_DEBUG crypto_LIBRARIES_RELEASE
# crypto target

if(THIRD0_INSTALL_ROOT)
elseif(CXOPENSSL_INSTALL_ROOT)
	set(ssl_INCLUDE_ROOT ${CXQHULL_INSTALL_ROOT}/include/openssl/)
	set(ssl_LIB_ROOT ${CXQHULL_INSTALL_ROOT}/lib/)
							   
	set(crypto_INCLUDE_ROOT ${CXQHULL_INSTALL_ROOT}/include/openssl/)
	set(crypto_LIB_ROOT ${CXQHULL_INSTALL_ROOT}/lib/)
endif()

__search_target_components(ssl
						   INC openssl/aes.h
						   DLIB ssl
						   LIB ssl
						   PRE openssl
						   )
							   
__search_target_components(crypto
						   INC openssl/aes.h
						   DLIB crypto
						   LIB crypto
						   PRE openssl
						   )
							   
if( WIN32 AND NOT CYGWIN )
	set(crypto_LIBRARIES_DEBUG ${crypto_LIBRARIES_DEBUG} ws2_32 crypt32)
	set(crypto_LIBRARIES_RELEASE ${crypto_LIBRARIES_RELEASE} ws2_32 crypt32)
endif()
	
__test_import(ssl lib)
__test_import(crypto lib)
set(openssl ssl crypto)

set(OPENSSL_INCLUDE_DIR ${ssl_INCLUDE_DIRS})