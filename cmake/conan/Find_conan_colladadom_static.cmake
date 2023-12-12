# colladadom_static target
__cc_find(pcre)
__cc_find(minizip_static)
__cc_find(libxml_static)
__cc_find(boost_header)

__conan_import(colladadom_static lib COMPONENT colladadom141 ILIB colladadom_static)
__conan_import(colladadom_static lib COMPONENT colladadom_static ILIB colladadom141 boost_header 
												libxml_static minizip_static
									 			pcrecpp_local pcre_local pcreposix_local  
									 			)

