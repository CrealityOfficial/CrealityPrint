# freeimage_static target

__cc_find(png_static)
__cc_find(open_jpeg)
__conan_import(freeimage_static lib ILIB png_static open_jpeg
									INTERFACE_DEF FREEIMAGE_LIB
									)
