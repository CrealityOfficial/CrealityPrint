# nlopt target

__conan_import(nlopt dll)

 if(CONAN_NLOPT_ROOT_RELEASE)
 	set(NLOPT_INCLUDE_DIRS ${CONAN_NLOPT_ROOT_RELEASE}/include/ ${CONAN_NLOPT_ROOT_RELEASE}/include/nlopt/)
 endif()
