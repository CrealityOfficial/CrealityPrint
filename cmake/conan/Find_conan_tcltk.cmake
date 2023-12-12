# nestplacer target
__cc_find(zlib)

set(TCLTK_COMPONETS
				tcl
				tk)
				
if(TCLTK_STATIC)
	__conan_import(tcltk lib COMPONENT ${TCLTK_COMPONETS})
else()
	__conan_import(tcltk dll COMPONENT ${TCLTK_COMPONETS})
endif()
